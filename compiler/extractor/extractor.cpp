#include "extractor.h"

#include "../ast/declarations.h"

namespace ionsl
{
    Extractor::Extractor(const Module &module, Compiler &compiler, TypeTable &typeTable, const std::vector<DeclId> &entrypoints)
        : m_module(module), m_compiler(compiler), m_types(typeTable), m_decls(module), m_entrypoints(entrypoints)
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

    void Extractor::extractEntrypoint(FunctionDecl &decl, Module &newModule)
    {
        AstWalker walker{};
        walker.on([&](const TypeSyntax& type)
        {
            auto info = m_types.getInfo(type.resolvedType);

            if (const auto structType = info.as<StructType>())
                extractDecl(*m_decls.get(structType->declId), newModule);
            if (const auto interfaceType = info.as<InterfaceType>())
                extractDecl(*m_decls.get(interfaceType->declId), newModule);
            if (const auto enumType = info.as<EnumType>())
                extractDecl(*m_decls.get(enumType->declId), newModule);
        });

        walker.on([&](const Expression& expression)
        {
            if (const auto identifierExpr = expression.as<IdentifierExpr>())
            {
                if (m_topLevelDecls.contains(identifierExpr->decl))
                {
                    auto* identifierDecl = m_decls.get(identifierExpr->decl);
                    if (!identifierDecl) return;

                    extractDecl(*identifierDecl, newModule);
                }
            }
        });

        walker.walk(decl);

        newModule.declarations().emplace_back(decl.clone(newModule.arena()));
    }

    void Extractor::extractDecl(const Declaration &decl, Module &newModule)
    {
        if (m_extracted.contains(decl.id)) return;

        newModule.declarations().emplace_back(decl.clone(newModule.arena()));
    }

    void Extractor::extractType(const TypeId id, Module& out)
    {
        const TypeInfo info = m_types.getInfo(id);
        if(const auto* s = info.as<StructType>())           extractDecl(*m_decls.get(s->declId), out);
        else if(const auto* i = info.as<InterfaceType>()) extractDecl(*m_decls.get(i->declId), out);
        else if(const auto* e = info.as<EnumType>())        extractDecl(*m_decls.get(e->declId), out);
        else if(const auto* a = info.as<ArrayType>())       extractType(a->elementType, out);
    }
}
