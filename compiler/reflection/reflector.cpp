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

    refl::ReflectionData Reflector::reflect()
    {
        refl::ReflectionData refl{};

        for (const auto* decl : m_module.declarations())
        {
            if(const auto funcDecl = decl->as<FunctionDecl>())
                reflectFunctionDecl(*funcDecl, refl);
        }

        for (const auto* decl : m_module.declarations())
        {
            if(const auto resourceDecl = decl->as<ResourceDecl>())
                reflectResourceDecl(*resourceDecl, refl);
        }

        return refl;
    }


    void Reflector::reflectFunctionDecl(const FunctionDecl &declaration, refl::ReflectionData& data)
    {
        if (!declaration.genericParams.empty())
            return;

        refl::Attributes attribs = reflectAttributes(*declaration.attributes);

        const auto shaderAttrib = attribs.find("shader");
        if (!shaderAttrib)
            return;

        const auto shaderTypeArg = shaderAttrib->find("shaderType");

        if (!shaderTypeArg || !std::holds_alternative<std::string>(*shaderTypeArg))
            return;

        refl::EntryPoint ep;
        ep.name = m_symbols.get(declaration.name);
        ep.attributes = attribs;
        ep.stage = convertStage(std::get<std::string>(*shaderTypeArg));

        m_stageMap.emplace(declaration.id, ep.stage);

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

        if (ep.stage == ShaderStage::Vertex)
        {
            // TODO skip params with builtin attributes
            uint32_t location = 0;
            uint32_t bufferSlot = 0;
            for (const auto param : declaration.params)
            {
                refl::VertexInput input;
                input.name = m_symbols.get(param->name);
                input.bufferSlot = bufferSlot++;
                input.inputRate = refl::InputRate::PerVertex; // TODO check attribute for per instance
                input.type = reflectType(param->type->resolvedType, &location);

                if (!input.type->isStruct())
                    input.location = location++;

                ep.vertexInputs.push_back(input);
            }
        }

        data.entryPoints.push_back(ep);
    }

    void Reflector::reflectResourceDecl(const ResourceDecl& declaration, refl::ReflectionData& data)
    {
        const std::string name = m_symbols.get(declaration.name);
        const refl::TypeRef t = reflectType(declaration.type);

        const refl::Attributes attrs = reflectAttributes(*declaration.attributes);

        std::vector<ShaderStage> usedByStages;

        for (const auto& id : declaration.usedBy)
        {
            if (auto it = m_stageMap.find(id); it != m_stageMap.end())
                usedByStages.push_back(it->second);
        }

        if (declaration.kind == ResourceKind::Texture)
        {
            refl::TextureResource texKind{};
            texKind.arraySize = 1; // TODO texture arrays
            texKind.dimension = refl::TextureDimension::Tex2D; // TODO reflect correct texture kind
            texKind.access = declaration.access;

            data.resources.push_back(refl::Resource{name, attrs, usedByStages, declaration.index, declaration.space, texKind});
            return;
        }

        refl::BufferResource bufKind{};
        bufKind.elementType = reflectType(declaration.type);
        bufKind.access = declaration.access;
        bufKind.kind = convertBufferKind(declaration.kind);

        data.resources.push_back(refl::Resource{name, attrs, usedByStages, declaration.index, declaration.space, bufKind});
    }

    refl::TypeRef Reflector::reflectType(const TypeId id, uint32_t* location)
    {
        TypeInfo info = m_types.getInfo(id);
        if (const auto primitive = info.as<PrimitiveType>())
            return reflectPrimitiveType(*primitive);
        if (const auto structure = info.as<StructType>())
            return reflectStructType(*structure, location);
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

    refl::TypeRef Reflector::reflectPrimitiveType(const PrimitiveType &type)
    {
        const uint64_t size = primitiveSize(type.kind);
        refl::Type{refl::PrimitiveType{type.kind}, size, size};
        return std::make_shared<refl::Type>(refl::Type(refl::PrimitiveType{type.kind}, size, size)); // TODO get alignment
    }

    refl::TypeRef Reflector::reflectStructType(const StructType &type, uint32_t* location)
    {
        uint32_t size = 0;

        const auto* decl = m_decls.get(type.declId)->as<StructDecl>();
        // TODO ensure decl

        std::vector<refl::Field> fields;

        for (const auto* field : decl->fields)
        {
            refl::Field f{};
            f.type = reflectType(field->type->resolvedType);
            f.offset = size;
            f.name = m_symbols.get(field->name);

            if (location)
                f.location = (*location)++;

            fields.push_back(f);

            size += f.type->size(); // TODO field alignment
        }

        const auto attributes = reflectAttributes(*decl->attributes);

        // TODO attributes
        refl::StructType kind{ m_symbols.get(decl->name),  attributes, fields };
        return std::make_shared<refl::Type>(refl::Type(kind, size, size)); // TODO alignment
    }

    refl::TypeRef Reflector::reflectArrayType(const ArrayType& type)
    {
        refl::ArrayType kind{};
        kind.size = type.size;
        kind.elementType = reflectType(type.elementType);
        const uint32_t size = kind.size ? *kind.size * kind.elementType->size() : kind.elementType->size();
        return std::make_shared<refl::Type>(refl::Type(kind, size, size)); // TODO alignment
    }

    refl::TypeRef Reflector::reflectMatrixType(const MatrixType &type)
    {
        refl::MatrixType kind{};
        kind.elementType = type.scalarType;
        kind.columns = type.columns;
        kind.rows = type.rows;
        const uint32_t size = primitiveSize(kind.elementType) * kind.rows * kind.columns;
        return std::make_shared<refl::Type>(refl::Type(kind, size, size)); // TODO alignment
    }

    refl::TypeRef Reflector::reflectVectorType(const VectorType &type)
    {
        refl::VectorType kind{};
        kind.elementType = type.scalarType;
        kind.dimension = type.dimension;
        const uint32_t size = primitiveSize(kind.elementType) * kind.dimension;
        return std::make_shared<refl::Type>(refl::Type(kind, size, size)); // TODO alignment
    }

    refl::TypeRef Reflector::reflectEnumType(const EnumType &type)
    {
        refl::EnumType kind{};
        const EnumDecl* enumDecl = m_decls.get(type.declId)->as<EnumDecl>();

        // const TypeId underlyingType = enumDecl->underlyingType ? enumDecl->underlyingType->resolvedType : TypeId::I32;

        // TODO underlying type but probably not a general type (restricted to ints)
        // kind.underlyingType = reflectType(underlyingType);

        for (const auto* member : enumDecl->members)
        {
            refl::EnumMember enumMember{};
            enumMember.name = m_symbols.get(member->name);
            enumMember.value = member->value;
        }

        // TODO use underlying type size instead of assuming 4 bytes
        uint32_t size = enumDecl->members.size() * 4; // kind.underlyingType->size();
        kind.name = m_symbols.get(enumDecl->name);

        return std::make_shared<refl::Type>(refl::Type(kind, size, size));
    }

    refl::Attributes Reflector::reflectAttributes(const Attributes &attributes)
    {
        refl::Attributes reflectedAttrs{};

        for (const auto* attrib : attributes.attributes)
        {
            reflectedAttrs.attributes.push_back(reflectAttribute(*attrib));
        }

        return reflectedAttrs;
    }

    refl::Attribute Reflector::reflectAttribute(const Attribute &attribute)
    {
        refl::Attribute attr{};
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

    refl::BufferKind Reflector::convertBufferKind(const ResourceKind kind)
    {
        switch (kind)
        {
            case ResourceKind::ConstantBuffer:
                return refl::BufferKind::Constant;
            case ResourceKind::StorageBuffer:
                return refl::BufferKind::Storage;
            case ResourceKind::PushConstant:
                return refl::BufferKind::PushConstants;
            default:
                return refl::BufferKind::ByteAddress; // TODO add unknown and more
        }
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
