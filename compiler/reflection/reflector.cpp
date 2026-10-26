#include "reflector.h"

#include <iostream>

#include "../ast/module.h"

namespace ionsl
{
    Reflector::Reflector(const SymbolTable &symbols, const TypeTable &types, const Module &module)
        : m_symbols(symbols),
          m_types(types),
          m_decls(module),
          m_module(module)
    {
    }

    ShaderReflection Reflector::reflect()
    {
        ShaderReflection refl{};

        for (const auto* decl : m_module.declarations())
        {
            reflectDeclaration(*decl, refl);
        }

        return refl;
    }

    void Reflector::reflectDeclaration(const Declaration &declaration, ShaderReflection& data)
    {
        if(const auto funcDecl = declaration.as<FunctionDecl>())
            reflectFunctionDecl(*funcDecl, data);
        else if(const auto resourceDecl = declaration.as<ResourceDecl>())
            reflectResourceDecl(*resourceDecl, data);
    }

    void Reflector::reflectFunctionDecl(const FunctionDecl &declaration, ShaderReflection& data)
    {
        if (!declaration.genericParams.empty())
            return;

        ReflectedAttributes attribs = reflectAttributes(*declaration.attributes);

        const auto shaderAttrib = attribs.find("shader");
        if (!shaderAttrib)
            return;

        const auto shaderTypeArg = shaderAttrib->find("shaderType");

        if (!shaderTypeArg || !std::holds_alternative<std::string>(*shaderTypeArg))
            return;

        ReflectedEntryPoint ep;
        ep.name = m_symbols.get(declaration.name);
        ep.attributes = attribs;
        ep.stage = convertStage(std::get<std::string>(*shaderTypeArg));

        if (ep.stage == ShaderStage::Compute)
        {
            const auto numThreadsAttrib = attribs.find("numthreads");
            if (!numThreadsAttrib)
                return;

            const auto threadsXArg = numThreadsAttrib->find("x");
            const auto threadsYArg = numThreadsAttrib->find("y");
            const auto threadsZArg = numThreadsAttrib->find("z");

            if (!threadsXArg || !threadsYArg || !threadsZArg ||
                !std::holds_alternative<ConstantInt>(*threadsXArg) ||
                !std::holds_alternative<ConstantInt>(*threadsYArg) ||
                !std::holds_alternative<ConstantInt>(*threadsZArg))
            {
                // TODO should error since should not be possible after sema
                return;
            }

            ep.threadsX = std::get<ConstantInt>(*threadsXArg).value;
            ep.threadsY = std::get<ConstantInt>(*threadsYArg).value;
            ep.threadsZ = std::get<ConstantInt>(*threadsZArg).value;
        }

        data.entryPoints.push_back(ep);
    }

    void Reflector::reflectResourceDecl(const ResourceDecl& declaration, ShaderReflection &data)
    {
        const std::string name = m_symbols.get(declaration.name);
        const ReflectedType t = reflectType(declaration.type);

        const ReflectedAttributes attrs = reflectAttributes(*declaration.attributes);

        const ReflectedResource res{name, attrs, t, declaration.access, declaration.kind};
        data.resources.push_back(res);
    }

    ReflectedType Reflector::reflectType(const TypeId id)
    {
        TypeInfo info = m_types.getInfo(id);
        if (const auto primitive = info.as<PrimitiveType>())
            return reflectPrimitiveType(*primitive);
        if (const auto structure = info.as<StructType>())
            return reflectStructType(*structure);
        if (const auto array = info.as<ArrayType>())
            return reflectArrayType(*array);
        if (const auto matrix = info.as<MatrixType>())
            return reflectMatrixType(*matrix);
        if (const auto vector = info.as<VectorType>())
            return reflectVectorType(*vector);
        if (const auto enumeration = info.as<EnumType>())
            return reflectEnumType(*enumeration);

        // TODO fix
        throw std::runtime_error("Unknown type reached during reflection");
    }

    ReflectedType Reflector::reflectPrimitiveType(const PrimitiveType &type)
    {
        const uint32_t size = primitiveSize(type.kind);
        return ReflectedType{type.kind, size, size}; // TODO get alignment
    }

    ReflectedType Reflector::reflectStructType(const StructType &type)
    {
        uint32_t size = 0;

        const auto* decl = m_decls.get(type.declId)->as<StructDecl>();
        // TODO ensure decl

        std::vector<ReflectedField> fields;

        for (const auto* field : decl->fields)
        {
            ReflectedField f{};
            f.type = std::make_shared<ReflectedType>(reflectType(field->type->resolvedType));
            f.offset = size;

            fields.push_back(f);

            size += f.type->size(); // TODO field alignment
        }

        ReflectedStructType kind{ m_symbols.get(decl->name),  fields };
        return ReflectedType{ kind, size, size }; // TODO alignment
    }

    ReflectedType Reflector::reflectArrayType(const ArrayType& type)
    {
        ReflectedArrayType kind{};
        kind.size = type.size;
        kind.elementType = std::make_shared<ReflectedType>(reflectType(type.elementType));
        const uint32_t size = kind.size ? *kind.size * kind.elementType->size() : kind.elementType->size();
        return ReflectedType{ kind, size, size }; // TODO alignment
    }

    ReflectedType Reflector::reflectMatrixType(const MatrixType &type)
    {
        ReflectedMatrixType kind{};
        kind.type = type.scalarType;
        kind.columns = type.columns;
        kind.rows = type.rows;
        const uint32_t size = primitiveSize(kind.type) * kind.rows * kind.columns;
        return ReflectedType{ kind, size, size }; // TODO alignment
    }

    ReflectedType Reflector::reflectVectorType(const VectorType &type)
    {
        ReflectedVectorType kind{};
        kind.type = type.scalarType;
        kind.dimension = type.dimension;
        const uint32_t size = primitiveSize(kind.type) * kind.dimension;
        return ReflectedType{ kind, size, size }; // TODO alignment
    }

    ReflectedType Reflector::reflectEnumType(const EnumType &type)
    {
        ReflectedEnumType kind{};
        const EnumDecl* enumDecl = m_decls.get(type.declId)->as<EnumDecl>();

        const TypeId underlyingType = enumDecl->underlyingType ? enumDecl->underlyingType->resolvedType : TypeId::I32;

        kind.underlyingType = std::make_shared<ReflectedType>(reflectType(underlyingType));

        for (const auto* member : enumDecl->members)
        {
            ReflectedEnumMember enumMember{};
            enumMember.name = m_symbols.get(member->name);
            enumMember.value = member->value;
        }

        uint32_t size = enumDecl->members.size() * kind.underlyingType->size();

        return ReflectedType{ kind, size, size };
    }

    ReflectedAttributes Reflector::reflectAttributes(const Attributes &attributes)
    {
        ReflectedAttributes reflectedAttrs{};

        for (const auto* attrib : attributes.attributes)
        {
            reflectedAttrs.attributes.push_back(reflectAttribute(*attrib));
        }

        return reflectedAttrs;
    }

    ReflectedAttribute Reflector::reflectAttribute(const Attribute &attribute)
    {
        ReflectedAttribute attr{};
        attr.name = attribute.name.string(m_symbols);

        for (const auto& arg : attribute.args)
        {
            attr.args[m_symbols.get(arg->name)] = *arg->value; // TODO maybe check if null but should never be after sema
        }

        return attr;
    }

    ShaderStage Reflector::convertStage(const std::string &name)
    {
        if (name == "vertex")
            return ShaderStage::Vertex;
        if (name == "pixel" || name == "fragment")
            return ShaderStage::Pixel;
        if (name == "geometry")
            return ShaderStage::Geometry;
        if (name == "hull")
            return ShaderStage::Hull;
        if (name == "mesh")
            return ShaderStage::Mesh;
        if (name == "amplification")
            return ShaderStage::Amplification;
        if (name == "compute")
            return ShaderStage::Compute;

        return ShaderStage::Unknown;
    }

    uint32_t Reflector::primitiveSize(const PrimitiveKind kind)
    {
        switch (kind)
        {
            case PrimitiveKind::Float64:
            case PrimitiveKind::Int64:
            case PrimitiveKind::UInt64:
                return 8;
            case PrimitiveKind::Bool:
            case PrimitiveKind::Int32:
            case PrimitiveKind::UInt32:
            case PrimitiveKind::Float32:
                return 4;
            case PrimitiveKind::Int16:
            case PrimitiveKind::UInt16:
            case PrimitiveKind::Float16:
                return 2;
            case PrimitiveKind::Int8:
            case PrimitiveKind::UInt8:
                return 1;
            default:
                return 0;
        }
    }
}
