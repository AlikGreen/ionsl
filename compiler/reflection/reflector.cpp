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
    }

    void Reflector::reflectFunctionDecl(const FunctionDecl &declaration, ShaderReflection& data)
    {
        if (!declaration.genericParams.empty())
            return;

        const auto shaderAttrib = declaration.attributes.find("shader", m_symbols);
        if (!shaderAttrib || shaderAttrib->args.empty())
            return;

        const auto arg = shaderAttrib->args.at(0).value;

        if (!std::holds_alternative<ConstantValue>(arg) || !std::holds_alternative<std::string>(std::get<ConstantValue>(arg)))
            return;

        ReflectedEntryPoint ep;
        ep.stage = convertStage(std::get<std::string>(std::get<ConstantValue>(arg)));

        for (const auto* param : declaration.params)
        {
            reflectValueDecl(*param, ep);
        }

        data.entryPoints.push_back(ep);
    }

    void Reflector::reflectValueDecl(const ValueDecl& declaration, ReflectedEntryPoint &ep)
    {
        if (declaration.attributes.attributes().empty())
            return;

        ReflectedResourceType resourceType;
        if (declaration.attributes.contains("constant", m_symbols))
            resourceType = ReflectedResourceType::ConstantBuffer;
        else if (declaration.attributes.contains("storage", m_symbols))
            resourceType = ReflectedResourceType::StorageBuffer;
        else
            return; // TODO texture and vertex inputs

        const std::string name = m_symbols.get(declaration.name);

        const ReflectedType t = reflectType(declaration.type->resolvedType);

        auto access = ReflectedAccess::ReadOnly;
        if (const auto attrib = declaration.attributes.find("access", m_symbols))
        {
            if (!attrib->args.empty() && std::holds_alternative<QualifiedName>(attrib->args.front().value))
            {
                const std::string arg = std::get<QualifiedName>(attrib->args.front().value).string(m_symbols);

                if (arg == "readonly")
                    access = ReflectedAccess::ReadOnly;
                else if (arg == "write")
                    access = ReflectedAccess::WriteOnly;
                if (arg == "readwrite")
                    access = ReflectedAccess::ReadWrite;
            }
        }

        const ReflectedResource res{name, t, access, resourceType};
        ep.resources.push_back(res);
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
