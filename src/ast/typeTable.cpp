#include "typeTable.h"

namespace ionsl
{
    TypeTable::TypeTable()
    {
        m_types.emplace_back(PrimitiveType{ PrimitiveKind::Void });
        m_types.emplace_back(PrimitiveType{ PrimitiveKind::Bool });

        m_primitiveTypes[PrimitiveKind::Void] = TypeIdVoid;
        m_primitiveTypes[PrimitiveKind::Bool] = TypeIdBool;
        m_primitiveTypes[PrimitiveKind::UInt64] = TypeIdU64;
        m_primitiveTypes[PrimitiveKind::Int64] = TypeIdI64;
        m_primitiveTypes[PrimitiveKind::Float64] = TypeIdF64;
        m_primitiveTypes[PrimitiveKind::String] = TypeIdString;
    }

    TypeId TypeTable::getPrimitiveType(const PrimitiveKind kind)
    {
        if(const auto it = m_primitiveTypes.find(kind); it != m_primitiveTypes.end())
            return it->second;

        TypeInfo info{};
        info.kind = PrimitiveType{kind};

        const TypeId id = m_types.size();
        m_types.push_back(info);

        m_primitiveTypes[kind] = id;
        return id;
    }

    TypeId TypeTable::getVectorType(TypeId scalarKind, uint32_t dimension)
    {
        auto type = VectorType{ scalarKind, dimension };
        if(const auto it = m_vectorTypes.find(type); it != m_vectorTypes.end())
            return it->second;

        const TypeId id = m_types.size();
        m_types.emplace_back(type);
        m_vectorTypes[type] = id;
        return id;
    }

    TypeId TypeTable::getMatrixType(const TypeId scalarKind, const uint32_t rows, const uint32_t columns)
    {
        auto type = MatrixType{ scalarKind, rows, columns };
        if(const auto it = m_matrixTypes.find(type); it != m_matrixTypes.end())
            return it->second;

        const TypeId id = m_types.size();
        m_types.emplace_back(type);
        m_matrixTypes[type] = id;
        return id;
    }

    TypeId TypeTable::getArrayType(const TypeId elementType, const std::optional<uint32_t> size)
    {
        auto type = ArrayType{ elementType, size };
        if(const auto it = m_arrayTypes.find(type); it != m_arrayTypes.end())
            return it->second;

        const TypeId id = m_types.size();
        m_types.emplace_back(type);
        m_arrayTypes[type] = id;
        return id;
    }

    TypeId TypeTable::getStructType(const DeclId id)
    {
        if(const auto it = m_structTypes.find(id); it != m_structTypes.end())
            return it->second;

        TypeInfo info{};
        info.kind = StructType{id};

        const TypeId typeId = m_types.size();
        m_types.push_back(info);

        m_structTypes[id] = typeId;
        return typeId;
    }

    TypeId TypeTable::getInterfaceType(const DeclId id)
    {
        if(const auto it = m_interfaceTypes.find(id); it != m_interfaceTypes.end())
            return it->second;

        TypeInfo info{};
        info.kind = InterfaceType{id};

        const TypeId typeId = m_types.size();
        m_types.push_back(info);

        m_interfaceTypes[id] = typeId;
        return typeId;
    }

    TypeInfo TypeTable::getInfo(const TypeId id) const
    {
        return m_types.at(id);
    }

    bool TypeTable::isIntegral(const TypeId id) const
    {
        auto info = getInfo(id);
        if(!info.is<PrimitiveType>()) return false;
        const PrimitiveKind kind = info.as<PrimitiveType>()->kind;

        switch (kind)
        {
            case PrimitiveKind::Int8:
            case PrimitiveKind::Int16:
            case PrimitiveKind::Int32:
            case PrimitiveKind::Int64:
            case PrimitiveKind::UInt8:
            case PrimitiveKind::UInt16:
            case PrimitiveKind::UInt32:
            case PrimitiveKind::UInt64:
                return true;
            default:
                return false;
        }
    }
}
