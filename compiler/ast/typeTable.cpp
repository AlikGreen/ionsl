#include "typeTable.h"

namespace ionsl
{

    TypeTable::TypeTable()
    {
        static std::unordered_map<TypeId, PrimitiveKind> kPrimitives =
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

        m_types.resize(kPrimitives.size() + 1);
        m_types[TypeId::Error.value()] = {ErrorType{}};

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

    TypeId TypeTable::getVectorType(const PrimitiveKind scalarKind, uint32_t dimension)
    {
        auto type = VectorType{ scalarKind, dimension };
        if(const auto it = m_vectorTypes.find(type); it != m_vectorTypes.end())
            return it->second;

        const TypeId id = addType(type);
        m_vectorTypes[type] = id;
        return id;
    }

    TypeId TypeTable::getMatrixType(const PrimitiveKind scalarKind, const uint32_t rows, const uint32_t columns)
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

        const TypeId typeId = addType(info);

        m_structTypes[id] = typeId;
        return typeId;
    }

    TypeId TypeTable::getInterfaceType(const DeclId id)
    {
        if(const auto it = m_interfaceTypes.find(id); it != m_interfaceTypes.end())
            return it->second;

        TypeInfo info{};
        info.kind = InterfaceType{id};

        const TypeId typeId = addType(info);

        m_interfaceTypes[id] = typeId;
        return typeId;
    }

    TypeId TypeTable::getResourceType(const DeclId id, const TypeId containedType)
    {
        if(const auto it = m_resourceTypes.find(id); it != m_resourceTypes.end())
            return it->second;

        TypeInfo info{};
        info.kind = ResourceType{id, containedType};

        const TypeId typeId = addType(info);

        m_resourceTypes[id] = typeId;
        return typeId;
    }

    TypeId TypeTable::getEnumType(const DeclId id)
    {
        if(const auto it = m_enumTypes.find(id); it != m_enumTypes.end())
            return it->second;

        TypeInfo info{};
        info.kind = EnumType{id};

        const TypeId typeId = addType(info);

        m_enumTypes[id] = typeId;
        return typeId;
    }

    TypeId TypeTable::getGenericType(const DeclId id)
    {
        if(const auto it = m_genericTypes.find(id); it != m_genericTypes.end())
            return it->second;

        TypeInfo info{};
        info.kind = GenericType{id};

        const TypeId typeId = addType(info);

        m_genericTypes[id] = typeId;
        return typeId;
    }

    TypeId TypeTable::getGenericVectorType(const TypeId elementType, const uint32_t dimension)
    {
        auto type = GenericVectorType{ elementType, dimension };
        if(const auto it = m_genericVectorTypes.find(type); it != m_genericVectorTypes.end())
            return it->second;

        const TypeId id = addType(type);
        m_genericVectorTypes[type] = id;
        return id;
    }

    TypeId TypeTable::getGenericMatrixType(const TypeId elementType, const uint32_t rows, const uint32_t columns)
    {
        auto type = GenericMatrixType{ elementType, rows, columns };
        if(const auto it = m_genericMatrixTypes.find(type); it != m_genericMatrixTypes.end())
            return it->second;

        const TypeId id = addType(type);
        m_genericMatrixTypes[type] = id;
        return id;
    }

    TypeInfo TypeTable::getInfo(const TypeId id) const
    {
        return m_types.at(id.value());
    }

    bool TypeTable::isIntegral(const TypeId id) const
    {
        auto info = getInfo(id);
        if (info.is<EnumType>()) return true;
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

    bool TypeTable::isFloat(const TypeId id)
    {
        return id == TypeId::F16 || id == TypeId::F32 || id == TypeId::F64;
    }

    bool TypeTable::containsGenericParam(const TypeId id) const
    {
        thread_local std::vector<uint8_t> cache;
        const uint32_t idx = id.value();
        if (idx < cache.size())
        {
            if (cache[idx] != 0) return cache[idx] == 1;
        }
        else
        {
            cache.resize(m_types.size() < idx + 32 ? idx + 256 : m_types.size(), 0);
        }

        if (id == TypeId::Error) { cache[idx] = 2; return false; }

        const TypeInfo info = m_types.at(idx);

        bool result = false;

        if (info.is<GenericType>())
        {
            result = true;
        }
        else if (const auto* array = info.as<ArrayType>())
        {
            result = containsGenericParam(array->elementType);
        }
        else if (const auto* resource = info.as<ResourceType>())
        {
            result = containsGenericParam(resource->containedType);
        }

        // TODO update when adding generic structs

        cache[idx] = result ? 1 : 2;
        return result;
    }

    TypeId TypeTable::addType(const TypeInfo info)
    {
        const TypeId typeId{static_cast<uint32_t>(m_types.size())};
        m_types.push_back(info);
        return typeId;
    }
}
