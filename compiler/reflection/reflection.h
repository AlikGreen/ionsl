#pragma once
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "../ast/type.h"


namespace ionsl
{
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

using ReflectedTypeKind = std::variant<PrimitiveKind, ReflectedVectorType, ReflectedMatrixType, ReflectedStructType, ReflectedArrayType>;

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

enum class ReflectedResourceType
{
    ConstantBuffer, PushConstant, StorageBuffer, Texture,
};

enum class ReflectedAccess
{
    ReadOnly, WriteOnly, ReadWrite
};

// TODO texture dimension
struct ReflectedResource
{
    std::string name;
    ReflectedType type;
    ReflectedAccess access;
    ReflectedResourceType resourceType;
};

enum class ShaderStage
{
    Vertex, Pixel, Geometry, Mesh, Hull, Amplification, Compute, Unknown
};

struct ReflectedEntryPoint
{
    ShaderStage stage;
    std::vector<ReflectedResource> resources;
};

struct ShaderReflection
{
    std::vector<ReflectedEntryPoint> entryPoints;
};
}
