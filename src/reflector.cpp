#include "reflector.h"

namespace ionsl
{
    Reflector::Reflector(Module &module)
        : m_ast(module.decls)
    {
    }

    refl::Data Reflector::reflect()
    {
        for(const auto& decl : m_ast)
        {
            reflectDecl(decl);
        }

        return m_reflection;
    }

    refl::Data Reflector::reflect(Module &module)
    {
        Reflector reflector{module};
        return reflector.reflect();
    }

    void Reflector::reflectDecl(const DeclNode &decl)
    {
        std::visit(
            [this]<typename T0>(T0&& d)
            {
                using T = std::decay_t<T0>;

                if constexpr (std::is_same_v<T, FunctionDecl>)
                    reflectFunctionDecl(d);
                // else if constexpr (std::is_same_v<T, VarDecl>)
                //     reflectVarDecl(d);
            },
        decl.decl
        );
    }

    void Reflector::reflectFunctionDecl(const FunctionDecl &decl)
    {
        const auto stage = tryGetStageAttribute(decl.attributes);
        if(!stage) return;

        refl::EntryPoint ep{};
        ep.name = decl.name;
        ep.stage = *stage;

        for(const auto& attrib : decl.attributes)
        {
            if(attrib.name.fullName() != "numthreads" || attrib.args.empty()) continue;

            if(const IntegerLiteral* num = std::get_if<IntegerLiteral>(&attrib.args[0]))
                ep.threadsX = num->value;
            else
                continue;

            if(attrib.args.size() < 2)
                ep.threadsY = 1;
            else if(const IntegerLiteral* num = std::get_if<IntegerLiteral>(&attrib.args[1]))
                ep.threadsY = num->value;

            if(attrib.args.size() < 3)
                ep.threadsZ = 1;
            else if(const IntegerLiteral* num = std::get_if<IntegerLiteral>(&attrib.args[2]))
                ep.threadsZ = num->value;
        }

        if(ep.stage == refl::ShaderStage::Vertex)
            ep.inputLayout = reflectVertexInputLayout(decl);

        m_reflection.entryPoints.push_back(ep);
    }

    std::optional<refl::VertexInputLayout> Reflector::reflectVertexInputLayout(const FunctionDecl& entry)
    {
        refl::VertexInputLayout layout{};
        uint32_t nextSlot = 0;

        for (size_t i = 0; i < entry.params.size(); i++)
        {
            const auto& param = entry.params[0];
            if (hasSystemValueAttribute(param.attributes)) continue;

            refl::VertexBufferBinding binding{};
            binding.paramName = param.name;
            binding.inputSlot = nextSlot++;
            binding.inputRate = hasAttribute("per_instance", param.attributes) ? refl::VertexInputRate::PerInstance : refl::VertexInputRate::PerVertex;

            uint32_t offset = 0;

            auto type = reflectType(param.type);

            // TODO allow or explicitly disallow matrices on vertex inputs
            if (type.isScalar() || type.isVector())
            {
                refl::VertexAttribute attr{};
                attr.fieldName = param.name;
                attr.semanticName = "ATTRIB";
                attr.semanticIndex = 0;
                attr.relativeOffset = offset;
                attr.componentKind = type.isScalar() ? type.asScalar().kind : type.asVector().componentKind;
                attr.numComponents  = type.isScalar() ? 1 : type.asVector().dimension;


                binding.attributes.push_back(attr);
                offset += type.sizeBytes;
            }
            else if (type.isStruct())
            {
                // TODO allow or explicitly disallow matrices and nested structs on vertex inputs
                uint32_t autoAttribIndex = 0;
                for (const auto& innerField : type.asStruct().layout.fields)
                {
                    if (type.isScalar() || type.isVector())
                    {
                        refl::VertexAttribute attr{};
                        attr.fieldName = innerField.name;
                        attr.semanticName = "ATTRIB";
                        attr.semanticIndex = autoAttribIndex++;
                        attr.relativeOffset = offset;
                        attr.componentKind = innerField.type->isScalar() ? innerField.type->asScalar().kind : innerField.type->asVector().componentKind;
                        attr.numComponents  = innerField.type->isScalar() ? 1 : innerField.type->asVector().dimension;

                        binding.attributes.push_back(attr);
                        offset += innerField.type->sizeBytes;
                    }
                }
            }

            binding.strideBytes = offset;

            layout.buffers.push_back(std::move(binding));
        }

        return layout.buffers.empty() ? std::nullopt : std::make_optional(std::move(layout));
    }

    refl::StructLayout Reflector::reflectStructLayout(const StructDecl& decl)
    {
        refl::StructLayout layout{};

        uint32_t currentOffset = 0;

        for(const auto& field : decl.fields)
        {
            refl::FieldInfo fieldInfo{};
            refl::TypeInfo fieldType{};

            fieldInfo.name = field.name;
            fieldInfo.offsetBytes = currentOffset;
            fieldInfo.type = Box<refl::TypeInfo>::make(std::move(reflectType(field.type)));
            currentOffset += typeSize(*fieldInfo.type);

            layout.fields.push_back(fieldInfo);
        }

        layout.sizeBytes = currentOffset;

        return layout;
    }

    refl::TypeInfo Reflector::reflectType(const Type& t)
    {
        return std::visit([this]<typename T0>(const T0& kind) -> refl::TypeInfo
        {
            using T = std::decay_t<T0>;

            if constexpr (std::is_same_v<T, PrimitiveType>)
            {
                refl::TypeInfo info;
                info.name = primitiveDisplayName(kind.kind);
                info.sizeBytes = primitiveSize(kind.kind);
                info.setKind(refl::ScalarTypeInfo{ toScalarKind(kind.kind) });
                return info;
            }
            else if constexpr (std::is_same_v<T, VectorType>)
            {
                refl::TypeInfo info;
                info.name = "vector";
                // FIXME
                // info.sizeBytes = primitiveSize(kind.scalarType) * kind.dimension;
                // info.setKind(refl::VectorTypeInfo{ toScalarKind(kind.scalarType), kind.dimension });
                return info;
            }
            else if constexpr (std::is_same_v<T, MatrixType>)
            {
                refl::TypeInfo info;
                info.name = "matrix";
                // FIXME
                // info.sizeBytes = primitiveSize(kind.scalarType) * kind.rows * kind.columns;
                // info.setKind(refl::MatrixTypeInfo{ toScalarKind(kind.scalarType), kind.rows, kind.columns });
                return info;
            }
            else if constexpr (std::is_same_v<T, StructType>)
            {
                refl::StructLayout layout = reflectStructLayout(std::get<StructDecl>(m_declTable.get(kind.declId)->decl));
                refl::TypeInfo info;
                info.name = std::get<StructDecl>(m_declTable.get(kind.declId)->decl).name;
                info.sizeBytes = layout.sizeBytes;
                info.setKind(refl::StructTypeInfo{ std::move(layout) });
                return info;
            }
            else if constexpr (std::is_same_v<T, ArrayType>)
            {
                refl::TypeInfo elem = reflectType(*kind.elementType);
                std::optional<uint32_t> count = kind.size ? tryFoldConstantInt(**kind.size) : std::nullopt;
                refl::TypeInfo info;
                info.name = elem.name + "[]";
                info.sizeBytes = count ? elem.sizeBytes * (*count) : 0;
                info.setKind(refl::ArrayTypeInfo{ Box<refl::TypeInfo>::make(std::move(elem)), count });
                return info;
            }

            return {};
        }, t.kind);
    }

    std::optional<refl::ShaderStage> Reflector::tryGetStageAttribute(const std::vector<Attribute> &attrs)
    {
        for (const auto& attr : attrs)
        {
            if (attr.name.segments.size() == 1 && attr.name.segments[0] == "shader" && !attr.args.empty())
            {
                const std::string* stageName = std::get_if<std::string>(&attr.args[0]);
                if (!stageName) continue;
                if (*stageName == "vertex")  return refl::ShaderStage::Vertex;
                if (*stageName == "pixel")   return refl::ShaderStage::Pixel;
                if (*stageName == "compute") return refl::ShaderStage::Compute;
                if (*stageName == "geometry") return refl::ShaderStage::Geometry;
                if (*stageName == "hull") return refl::ShaderStage::Hull;
                if (*stageName == "amplification") return refl::ShaderStage::Amplification;
                if (*stageName == "Mesh") return refl::ShaderStage::Mesh;
            }
        }
        return std::nullopt;
    }

    bool Reflector::hasAttribute(const std::string &name, const std::vector<Attribute> &attrs)
    {
        for(auto& attr : attrs)
        {
            if(attr.name.fullName() == name)
                return true;
        }

        return false;
    }

    bool Reflector::hasSystemValueAttribute(const std::vector<Attribute> &attrs)
    {
        for(auto& attr : attrs)
        {
            if(attr.name.fullName().starts_with("sv_"))
                return true;
        }

        return false;
    }

    refl::ScalarKind Reflector::toScalarKind(const PrimitiveKind kind)
    {
        switch (kind)
        {
            case PrimitiveKind::Bool:
                return refl::ScalarKind::Bool;
            case PrimitiveKind::Float16:
                return refl::ScalarKind::Float16;
            case PrimitiveKind::Float32:
                return refl::ScalarKind::Float32;
            case PrimitiveKind::Float64:
                return refl::ScalarKind::Float64;
            case PrimitiveKind::Int8:
                return refl::ScalarKind::Int8;
            case PrimitiveKind::Int16:
                return refl::ScalarKind::Int16;
            case PrimitiveKind::Int32:
                return refl::ScalarKind::Int32;
            case PrimitiveKind::Int64:
                return refl::ScalarKind::Int64;
            case PrimitiveKind::UInt8:
                return refl::ScalarKind::UInt8;
            case PrimitiveKind::UInt16:
                return refl::ScalarKind::UInt16;
            case PrimitiveKind::UInt32:
                return refl::ScalarKind::UInt32;
            case PrimitiveKind::UInt64:
                return refl::ScalarKind::UInt64;
            default:
                return refl::ScalarKind::Unknown;
        }
    }


    std::string Reflector::primitiveDisplayName(PrimitiveKind kind)
    {
        switch (kind)
        {
            case PrimitiveKind::Bool:
                return "bool";
            case PrimitiveKind::Float16:
                return "f16";
            case PrimitiveKind::Float32:
                return "f32";
            case PrimitiveKind::Float64:
                return "f64";
            case PrimitiveKind::Int8:
                return "i8";
            case PrimitiveKind::Int16:
                return "i16";
            case PrimitiveKind::Int32:
                return "i32";
            case PrimitiveKind::Int64:
                return "i64";
            case PrimitiveKind::UInt8:
                return "u8";
            case PrimitiveKind::UInt16:
                return "u16";
            case PrimitiveKind::UInt32:
                return "u32";
            case PrimitiveKind::UInt64:
                return "u64";
            default:
                return "unknown";
        }
    }

    uint32_t Reflector::typeSize(refl::TypeInfo type)
    {
        return std::visit([]<typename T0>(const T0& kind) -> uint32_t
        {
            using T = std::decay_t<T0>;

            if constexpr (std::is_same_v<T, refl::ScalarTypeInfo>)
            {
                return scalarSize(kind.kind);
            }
            else if constexpr (std::is_same_v<T, VectorType>)
            {
                return scalarSize(kind.componentKind) * kind.dimension;
            }
            else if constexpr (std::is_same_v<T, MatrixType>)
            {
                return scalarSize(kind.componentKind) * kind.rows * kind.columns;
            }
            else if constexpr (std::is_same_v<T, StructType>)
            {
                return kind.layout.sizeBytes;
            }
            else if constexpr (std::is_same_v<T, ArrayType>)
            {
                return typeSize(*kind.elementType) * (kind.count.has_value() ? kind.count.value() : 0);
            }

            return {};
        }, type.getKind());
    }

    uint32_t Reflector::scalarSize(const refl::ScalarKind kind)
    {
        switch (kind)
        {
            case refl::ScalarKind::Bool:
            case refl::ScalarKind::Int8:
            case refl::ScalarKind::UInt8:
                return 1;
            case refl::ScalarKind::Float16:
            case refl::ScalarKind::Int16:
            case refl::ScalarKind::UInt16:
                return 2;
            case refl::ScalarKind::Float32:
            case refl::ScalarKind::Int32:
            case refl::ScalarKind::UInt32:
                return 4;
            case refl::ScalarKind::Float64:
            case refl::ScalarKind::Int64:
            case refl::ScalarKind::UInt64:
                return 8;
            default:
                return 0;
        }
    }

    uint32_t Reflector::primitiveSize(const PrimitiveKind kind)
    {
        return scalarSize(toScalarKind(kind));
    }

    std::optional<uint32_t> Reflector::tryFoldConstantInt(const ExprNode &expr)
    {
        // TODO try and evaluate const expressions
        if(const auto literalExpr = std::get_if<Literal>(&expr.expr))
            if(const auto val = std::get_if<IntegerLiteral>(literalExpr))
                return static_cast<uint32_t>(val->value);

        return std::nullopt;
    }
}
