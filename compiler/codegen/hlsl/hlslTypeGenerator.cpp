#include "hlslGenerator.h"

#include "../../ast/typeTable.h"

namespace ionsl
{
    void HlslGenerator::genType(const TypeId id)
    {
        TypeInfo info = m_typeTable.getInfo(id);

        if(const auto primitive = info.as<PrimitiveType>())
            genPrimitiveType(primitive->kind);
        if(const auto vector = info.as<VectorType>())
            genVectorType(*vector);
        if(const auto matrix = info.as<MatrixType>())
            genMatrixType(*matrix);
        if(const auto structure = info.as<StructType>())
            genStructType(*structure);
        if(const auto enumeration = info.as<EnumType>())
            genEnumType(*enumeration);
        if(const auto array = info.as<ArrayType>())
            genArrayType(*array);
    }

    void HlslGenerator::genVectorType(VectorType &type)
    {
        // use the simplified type name if possible
        if (type.scalarType == PrimitiveKind::Float32)
        {
            switch (type.dimension)
            {
                case 2:
                    m_writer.write("float2");
                    return;
                case 3:
                    m_writer.write("float3");
                    return;
                case 4:
                    m_writer.write("float4");
                    return;
                default:
                    break;
            }
        }
        if (type.scalarType == PrimitiveKind::UInt32)
        {
            switch (type.dimension)
            {
                case 2:
                    m_writer.write("uint2");
                    return;
                case 3:
                    m_writer.write("uint3");
                    return;
                case 4:
                    m_writer.write("uint4");
                    return;
                default:
                    break;
            }
        }
        if (type.scalarType == PrimitiveKind::Int32)
        {
            switch (type.dimension)
            {
                case 2:
                    m_writer.write("int2");
                    return;
                case 3:
                    m_writer.write("int3");
                    return;
                case 4:
                    m_writer.write("int4");
                    return;
                default:
                    break;
            }
        }

        m_writer.write("vector<");
        genPrimitiveType(type.scalarType);
        m_writer.write(", {}>", type.dimension);
    }

    void HlslGenerator::genMatrixType(MatrixType &type)
    {
        m_writer.write("matrix<");
        genPrimitiveType(type.scalarType);
        m_writer.write(", {}, {}>", type.rows, type.columns);
    }

    void HlslGenerator::genPrimitiveType(const PrimitiveKind kind)
    {
        std::unordered_map<PrimitiveKind, std::string> typeNames {
            { PrimitiveKind::Bool, "bool" },
            { PrimitiveKind::UInt32, "uint" },
            { PrimitiveKind::Int32, "int" },
            { PrimitiveKind::Float32, "float" },
            { PrimitiveKind::String, "string" },
            { PrimitiveKind::Void, "void" },
        };

        if(const auto it = typeNames.find(kind); it != typeNames.end())
            m_writer.write(it->second);
        else
            m_writer.write("unknown_type");
    }

    void HlslGenerator::genEnumType(EnumType &type)
    {
        auto decl = m_declTable.get(type.declId);
        m_writer.writeSymbol(decl->as<EnumDecl>()->name);
    }

    void HlslGenerator::genStructType(StructType& type)
    {
        auto decl = m_declTable.get(type.declId);
        m_writer.writeSymbol(decl->as<StructDecl>()->name);
    }

    void HlslGenerator::genArrayType(ArrayType &type)
    {
        genType(type.elementType);
        m_writer.write("[");
        if (type.size.has_value())
            m_writer.write("{}", *type.size);
        m_writer.write("]");
    }
}