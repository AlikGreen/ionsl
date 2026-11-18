#pragma once
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <optional>
#include <variant>
#include <unordered_map>


#include "../ast/constantValue.h"
#include "../ast/type.h"
#include "../common/enums.h"

namespace ionsl
{
    class Reflector;
}

namespace ionsl::refl
{
struct Attribute
{
    std::string name;
    std::unordered_map<std::string, ConstantValue> args{};

    [[nodiscard]] const ConstantValue* find(const std::string &argName) const
    {
        if (const auto it = args.find(argName); it != args.end())
            return &it->second;

        return nullptr;
    }
};

struct Attributes
{
    std::vector<Attribute> attributes;

    [[nodiscard]] const Attribute* find(const std::string &name) const
    {
        for (auto& attr : attributes)
            if (attr.name == name) return &attr;

        return nullptr;
    }
};


class Type;
using TypeRef = std::shared_ptr<const Type>;

struct Field
{
    std::string name;
    uint64_t offset;
    TypeRef type;
    std::optional<uint32_t> location;
};

struct StructType
{
    std::string name;
    Attributes attributes;
    std::vector<Field> fields;
};

struct EnumMember
{
    std::string name;
    ConstantInt value;
};

struct EnumType
{
    std::string name;
    Attributes attributes;
    std::vector<EnumMember> members;
};

struct ArrayType
{
    std::string name;
    TypeRef elementType;
    std::optional<uint64_t> size;
};


struct PrimitiveType
{
    PrimitiveKind kind;

    [[nodiscard]] constexpr bool isInteger() const
    {
        return  kind == PrimitiveKind::Int8 ||
                kind == PrimitiveKind::Int16 ||
                kind == PrimitiveKind::Int32 ||
                kind == PrimitiveKind::Int64 ||
                kind == PrimitiveKind::UInt8 ||
                kind == PrimitiveKind::UInt16 ||
                kind == PrimitiveKind::UInt32 ||
                kind == PrimitiveKind::UInt64;
    }
};

struct VectorType
{
    PrimitiveKind elementType;
    uint32_t dimension;
};

struct MatrixType
{
    PrimitiveKind elementType;
    uint32_t rows;
    uint32_t columns;
};


using TypeData = std::variant<StructType, EnumType, ArrayType, PrimitiveType, VectorType, MatrixType>;


class Type
{
public:
    [[nodiscard]] bool isPrimitive() const { return std::holds_alternative<PrimitiveType>(m_data); }
    [[nodiscard]] bool isVector() const { return std::holds_alternative<VectorType>(m_data); }
    [[nodiscard]] bool isMatrix() const { return std::holds_alternative<MatrixType>(m_data); }
    [[nodiscard]] bool isStruct() const { return std::holds_alternative<StructType>(m_data); }
    [[nodiscard]] bool isArray() const { return std::holds_alternative<ArrayType>(m_data); }
    [[nodiscard]] bool isEnum() const { return std::holds_alternative<EnumType>(m_data); }

    [[nodiscard]] const PrimitiveType* asPrimitive() const { return std::get_if<PrimitiveType>(&m_data); }
    [[nodiscard]] const VectorType* asVector() const { return std::get_if<VectorType>(&m_data); }
    [[nodiscard]] const MatrixType* asMatrix() const { return std::get_if<MatrixType>(&m_data); }
    [[nodiscard]] const StructType* asStruct() const { return std::get_if<StructType>(&m_data); }
    [[nodiscard]] const ArrayType* asArray() const { return std::get_if<ArrayType>(&m_data); }
    [[nodiscard]] const EnumType* asEnum() const { return std::get_if<EnumType>(&m_data); }

    [[nodiscard]] uint64_t size() const { return m_size; }
    [[nodiscard]] uint64_t alignment() const { return m_alignment; }
private:
    friend class ionsl::Reflector;

    Type(TypeData data, const uint64_t size, const uint64_t alignment) : m_data(std::move(data)), m_size(size), m_alignment(alignment) { }

    TypeData m_data;
    uint64_t m_size{};
    uint64_t m_alignment{};
};

enum class BufferKind
{
    Constant, PushConstants, Storage, ByteAddress
};

struct BufferResource
{
    TypeRef elementType;
    BufferKind kind;
    ResourceAccess access;
};

enum class TextureDimension
{
    Tex1D, Tex2D, Tex3D, TexCube
};

struct TextureResource
{
    ResourceAccess access;
    TextureDimension dimension;
    uint64_t arraySize;
};

struct SamplerResource
{
    uint64_t arraySize;
};

using ResourceData = std::variant<BufferResource, TextureResource, SamplerResource>;

class Resource
{
public:
    Resource() = default;
    Resource(const Resource&) = default;
    Resource& operator=(const Resource&) = default;

    [[nodiscard]] const std::string& name() const { return m_name; }
    [[nodiscard]] const Attributes& attributes() const { return m_attributes; }
    [[nodiscard]] const std::vector<ShaderStage>& visibility() const { return m_visibility; }
    [[nodiscard]] uint32_t index() const { return m_index; }
    [[nodiscard]] uint32_t space() const { return m_space; }

    [[nodiscard]] bool isBuffer() const { return std::holds_alternative<BufferResource>(m_data); }
    [[nodiscard]] bool isTexture() const { return std::holds_alternative<TextureResource>(m_data); }
    [[nodiscard]] bool isSampler() const { return std::holds_alternative<SamplerResource>(m_data); }

    [[nodiscard]] const BufferResource* asBuffer() const { return std::get_if<BufferResource>(&m_data); }
    [[nodiscard]] const TextureResource* asTexture() const { return std::get_if<TextureResource>(&m_data); }
    [[nodiscard]] const SamplerResource* asSampler() const { return std::get_if<SamplerResource>(&m_data); }
private:
    friend class ionsl::Reflector;

    Resource(std::string name, Attributes attributes, std::vector<ShaderStage> stages, uint32_t index, uint32_t space, ResourceData resource)
        : m_name(std::move(name)), m_attributes(std::move(attributes)), m_visibility(std::move(stages)), m_index(index), m_space(space), m_data(std::move(resource)) { }

    std::string m_name;
    Attributes m_attributes;
    std::vector<ShaderStage> m_visibility;
    uint32_t m_index{};
    uint32_t m_space{};
    ResourceData m_data;
};

enum class InputRate
{
    PerVertex,
    PerInstance
};

struct VertexInput
{
    std::string name;
    TypeRef type;
    InputRate inputRate;
    uint32_t bufferSlot;
    std::optional<uint32_t> location;
};

struct EntryPoint
{
    std::string name;
    Attributes attributes;
    ShaderStage stage;
    uint32_t threadsX{}, threadsY{}, threadsZ{};
    std::vector<VertexInput> vertexInputs;
};

struct ReflectionData
{
    std::vector<EntryPoint> entryPoints;
    std::vector<Resource> resources;
};
}
