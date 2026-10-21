#include "extractor.h"

#include "../compiler.h"
#include "../ast/declarations.h"
#include "../ast/statements.h"

namespace ionsl
{
    Extractor::Extractor(const Module &module, Compiler &compiler, const std::vector<DeclId> &entrypoints)
        : m_module(module), m_compiler(compiler), m_types(compiler.m_typeTable), m_typeSystem(compiler.m_typeSystem),
            m_symbols(compiler.m_symbolTable), m_declAllocator(compiler.m_declAllocator), m_decls(module), m_entrypoints(entrypoints)
    {

    }

    Module Extractor::extract()
    {
        Module newModule{m_compiler};

        for (auto* decl : m_module.declarations())
        {
            m_topLevelDecls.emplace(decl->id);
        }

        for (auto* decl : m_module.declarations())
        {
            if (auto* funcDecl = decl->as<FunctionDecl>())
            {
                for (const auto ep : m_entrypoints)
                {
                    if (ep == decl->id)
                        extractEntrypoint(*funcDecl, newModule);
                }
            }
        }

        return newModule;
    }

    void Extractor::extractEntrypoint(FunctionDecl &decl, Module &out)
    {
        extractResources(decl, out);

        AstWalker walker{};
        walker.on([&](const TypeId& type)
        {
            auto info = m_types.getInfo(type);

            if (const auto structType = info.as<StructType>())
                extractDecl(*m_decls.get(structType->declId), out);
            if (const auto interfaceType = info.as<InterfaceType>())
                extractDecl(*m_decls.get(interfaceType->declId), out);
            if (const auto enumType = info.as<EnumType>())
                extractDecl(*m_decls.get(enumType->declId), out);
        });

        walker.on([&](const Expression& expression)
        {
            if (const auto identifierExpr = expression.as<IdentifierExpr>())
            {
                if (m_topLevelDecls.contains(identifierExpr->decl))
                {
                    auto* identifierDecl = m_decls.get(identifierExpr->decl);
                    if (!identifierDecl)
                        return;

                    walker.walk(*identifierDecl);
                    extractDecl(*identifierDecl, out);
                }
            }
        });

        walker.walk(decl);

        out.declarations().emplace_back(decl.clone(out.arena()));
    }

    void Extractor::extractDecl(const Declaration &decl, Module &out)
    {
        if (m_extracted.contains(decl.id) || !m_topLevelDecls.contains(decl.id)) return;

        m_extracted.emplace(decl.id);
        out.declarations().emplace_back(decl.clone(out.arena()));

        if (const auto* structure = decl.as<StructDecl>())
        {
            for (const auto* field : structure->fields)
                extractDecl(*field, out);

            // TODO extract methods
        }
        else if (const auto* resource = decl.as<ResourceDecl>())
        {
            extractType(resource->type, out);
        }
    }

    void Extractor::extractType(const TypeId id, Module& out)
    {
        const TypeInfo info = m_types.getInfo(id);
        if(const auto* s = info.as<StructType>())           extractDecl(*m_decls.get(s->declId), out);
        else if(const auto* i = info.as<InterfaceType>()) extractDecl(*m_decls.get(i->declId), out);
        else if(const auto* e = info.as<EnumType>())        extractDecl(*m_decls.get(e->declId), out);
        else if(const auto* a = info.as<ArrayType>())       extractType(a->elementType, out);
        else if (const auto rt = info.as<ResourceType>())     extractType(rt->containedType, out);
    }

    void Extractor::extractResources(FunctionDecl &declaration, Module &out)
    {
        if (!declaration.genericParams.empty())
            return;

        std::unordered_map<DeclId, DeclId> resourceMap{};

        for (auto it = declaration.params.begin(); it != declaration.params.end();)
        {
            if (const auto resource = extractResourceDecl(*(*it), declaration, out))
            {
                resourceMap.emplace((*it)->id, resource->id);
                it = declaration.params.erase(it);
            }
            else
            {
                ++it;
            }
        }

        remapIdentifiers(*declaration.body, resourceMap);
    }



    ResourceDecl* Extractor::extractResourceDecl(const ValueDecl &param, const FunctionDecl &function, Module& out)
    {
        auto resourceKind = getResourceKind(param);
        if (!resourceKind)
            return nullptr;

        // if already extracted
        if (auto resource = findResourceDecl(param))
        {
            resource->usedBy.push_back(function.id);
            return nullptr;
        }

        uint32_t index = 0;
        uint32_t space = 0;
        const auto bindingAttrib = param.attributes->find("binding", m_symbols);
        if (!bindingAttrib)
        {
            index = nextBindingIndex(space);
        }
        else
        {
            const int64_t indexArg = static_cast<int64_t>(std::get<ConstantInt>(*bindingAttrib->getArg("index", m_symbols)->value).value);
            if (indexArg == -1)
            {
                index = nextBindingIndex(space);
            }
            else
            {
                index = indexArg;
                m_takenBindingIndices.emplace(index);
            }

            space = std::get<ConstantInt>(*bindingAttrib->getArg("space", m_symbols)->value).value;
        }

        auto access = ResourceAccess::ReadOnly;
        const auto accessAttrib = param.attributes->find("access", m_symbols);
        if (accessAttrib)
        {
            const std::string accessStr = std::get<std::string>(*accessAttrib->getArg("access", m_symbols)->value);
            if (accessStr == "read-write")
                access = ResourceAccess::ReadWrite;
        }

        TypeId type = param.type->resolvedType;

        // strip the array from the type
        if (param.attributes->find("storage", m_symbols))
        {
            if (const auto arrayType = m_typeSystem.types().getInfo(param.type->resolvedType).as<ArrayType>())
            {
                type = arrayType->elementType;
            }
        }

        auto* decl = m_module.arena().create<ResourceDecl>();
        decl->span = param.span;
        decl->attributes = param.attributes->clone(m_module.arena());
        decl->name = param.name;
        decl->type = type;
        decl->index = index;
        decl->space = space;
        decl->access = access;
        decl->kind = *resourceKind;
        decl->id = m_declAllocator.allocate();
        decl->usedBy.push_back(function.id);

        m_topLevelDecls.emplace(decl->id);
        extractDecl(*decl, out);

        return decl;
    }

    ResourceDecl* Extractor::findResourceDecl(const ValueDecl &param) const
    {
        // TODO just use a cache
        for(auto* decl : m_module.declarations())
        {
            if (auto* resourceDecl = dynamic_cast<ResourceDecl*>(decl))
            {
                // TODO check for storage and remove the array
                if (resourceDecl->name == param.name && resourceDecl->type == param.type->resolvedType)
                    return resourceDecl;
            }
        }

        return nullptr;
    }

    void Extractor::remapIdentifiers(BlockStmt& block, std::unordered_map<DeclId, DeclId> resourceMap)
    {
        AstWalker walker{};
        walker.on([&resourceMap](Expression& expr)
        {
            auto* identExpr = expr.as<IdentifierExpr>();
            if (!identExpr) return;

            if (const auto it = resourceMap.find(identExpr->decl); it != resourceMap.end())
            {
                identExpr->decl = it->second;
            }
        });
        walker.walk(block);
    }

    std::optional<ResourceKind> Extractor::getResourceKind(const ValueDecl &param)
    {
        if (param.attributes->find("constant", m_symbols))
            return ResourceKind::ConstantBuffer;
        if (param.attributes->find("storage", m_symbols))
            return ResourceKind::StorageBuffer;

        // TODO textures
        return std::nullopt;
    }

    uint32_t Extractor::nextBindingIndex(const uint32_t space)
    {
        // TODO diagnostics
        if (space >= 16) return 0;

        while (m_takenBindingIndices.contains(m_nextBindingIndices[space]))
            m_nextBindingIndices[space]++;

        return m_nextBindingIndices[space]++;
    }

}
