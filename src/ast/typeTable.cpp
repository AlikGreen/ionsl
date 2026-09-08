#include "typeTable.h"

namespace ionsl
{
    static constexpr std::unordered_map<TypeId, PrimitiveKind> kPrimitives =
    {
        { TypeId::Void,   PrimitiveKind::Void },
        { TypeId::Bool,   PrimitiveKind::Bool },
        { TypeId::I8,     PrimitiveKind::Int8 },
        { TypeId::I16,    PrimitiveKind::Int16 },
        { TypeId::I32,    PrimitiveKind::Int32 },
        { TypeId::I64,    PrimitiveKind::Int64 },
        { TypeId::U8,     PrimitiveKind::UInt8 },
        { TypeId::U16,    PrimitiveKind::UInt16 },
        { TypeId::U32,    PrimitiveKind::UInt32 },
        { TypeId::U64,    PrimitiveKind::UInt64 },
        { TypeId::F16,    PrimitiveKind::Float16 },
        { TypeId::F32,    PrimitiveKind::Float32 },
        { TypeId::F64,    PrimitiveKind::Float64 },
        { TypeId::String, PrimitiveKind::String },
    };

    TypeTable::TypeTable()
    {
        m_types.resize(kPrimitives.size() + 1);
        m_types[TypeId::Error.value()] = {ErrorType{}};


        static_assert(std::size(kPrimitives) == static_cast<size_t>(PrimitiveKind::Unknown) - 1, "kPrimitives is missing an entry — every PrimitiveKind except Unknown must be listed here");

        for(const auto& [id, kind] : kPrimitives)
        {
            m_types[id.value()] = {PrimitiveType{kind}};
            m_primitiveTypes[kind] = id;
        }
    }

    TypeId TypeTable::getPrimitiveType(const PrimitiveKind kind)
    {
        if(const auto it = m_primitiveTypes.find(kind); it != m_primitiveTypes.end())
            return it->second;

        TypeInfo info{};
        info.kind = PrimitiveType{kind};

        const TypeId id = addType(info);
        m_primitiveTypes[kind] = id;
        return id;
    }

    TypeId TypeTable::getVectorType(TypeId scalarKind, uint32_t dimension)
    {
        auto type = VectorType{ scalarKind, dimension };
        if(const auto it = m_vectorTypes.find(type); it != m_vectorTypes.end())
            return it->second;

        const TypeId id = addType(type);
        m_vectorTypes[type] = id;
        return id;
    }

    TypeId TypeTable::getMatrixType(const TypeId scalarKind, const uint32_t rows, const uint32_t columns)
    {
        auto type = MatrixType{ scalarKind, rows, columns };
        if(const auto it = m_matrixTypes.find(type); it != m_matrixTypes.end())
            return it->second;

        const TypeId id = addType(type);
        m_matrixTypes[type] = id;
        return id;
    }

    TypeId TypeTable::getArrayType(const TypeId elementType, const std::optional<uint32_t> size)
    {
        auto type = ArrayType{ elementType, size };
        if(const auto it = m_arrayTypes.find(type); it != m_arrayTypes.end())
            return it->second;

        const TypeId id = addType(type);
        m_arrayTypes[type] = id;
        return id;
    }

    TypeId TypeTable::getStructType(const DeclId id)
    {
        if(const auto it = m_structTypes.find(id); it != m_structTypes.end())
            return it->second;

        TypeInfo info{};
        info.kind = StructType{id};

        const TypeId typeId{m_types.size()};
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

        const TypeId typeId{m_types.size()};
        m_types.push_back(info);

        m_interfaceTypes[id] = typeId;
        return typeId;
    }

    TypeInfo TypeTable::getInfo(const TypeId id) const
    {
        return m_types.at(id.value());
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

    TypeId TypeTable::addType(const TypeInfo info)
    {
        const TypeId typeId{m_types.size()};
        m_types.push_back(info);
        return typeId;
    }

    void TypeTable::addPrimitiveType(PrimitiveKind kind, TypeId id)
    {

        m_primitiveTypes[PrimitiveKind::Void] = id;
    }
}
