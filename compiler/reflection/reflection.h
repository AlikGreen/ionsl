#pragma once
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "../ast/constantValue.h"
#include "../ast/type.h"
#include "../common/enums.h"


namespace ionsl
{
struct ReflectedAttribute
{
    std::string name;
    std::unordered_map<std::string, ConstantValue> args{};


    ConstantValue* find(const std::string &argName)
    {
        if (const auto it = args.find(argName); it != args.end())
            return &it->second;

        return nullptr;
    }
};

struct ReflectedAttributes
{
    std::vector<ReflectedAttribute> attributes;

    ReflectedAttribute* find(const std::string &name)
    {
        for (auto& attr : attributes)
            if (attr.name == name) return &attr;

        return nullptr;
    }
};

class ReflectedType;


struct ReflectedVectorType
{
    uint32_t dimension;
    PrimitiveKind type;
};

struct ReflectedMatrixType
{
    uint32_t rows;
    uint32_t columns;
    PrimitiveKind type;
};

struct ReflectedField
{
    std::shared_ptr<ReflectedType> type;
    uint32_t offset;
};

struct ReflectedStructType
{
    std::string name;
    std::vector<ReflectedField> fields;
};

struct ReflectedArrayType
{
    std::shared_ptr<ReflectedType> elementType;
    std::optional<uint32_t> size;
};

struct ReflectedEnumMember
{
    std::string name;
    ConstantInt value;
};

struct ReflectedEnumType
{
    std::shared_ptr<ReflectedType> underlyingType;
    std::vector<ReflectedEnumMember> members;
};

using ReflectedTypeKind = std::variant<PrimitiveKind, ReflectedVectorType, ReflectedMatrixType, ReflectedStructType, ReflectedArrayType, ReflectedEnumType>;

class ReflectedType
{
public:
    ReflectedType(ReflectedTypeKind kind, const uint32_t size, const uint32_t alignment) : m_type(std::move(kind)), m_size(size), m_alignment(alignment) { }
    [[nodiscard]] uint32_t size() const { return m_size; }
    [[nodiscard]] uint32_t alignment() const { return m_alignment; }
    [[nodiscard]] const ReflectedTypeKind& kind() const { return m_type; }
private:
    ReflectedTypeKind m_type;
    uint32_t m_size{};
    uint32_t m_alignment{};
};


// TODO texture dimension
struct ReflectedResource
{
    std::string name;
    ReflectedAttributes attributes;
    ReflectedType type;
    ResourceAccess access;
    ResourceKind kind;
};

struct ReflectedEntryPoint
{
    std::string name;
    ReflectedAttributes attributes;
    ShaderStage stage;
    uint32_t threadsX{}, threadsY{}, threadsZ{};
};

struct ShaderReflection
{
    std::vector<ReflectedEntryPoint> entryPoints;
    std::vector<ReflectedResource> resources;
};
}
