#pragma once
#include <vector>

#include "ast.h"
#include "declTable.h"
#include "module.h"

namespace ionsl
{
namespace refl
{
    enum class ScalarKind { Float16, Float32, Float64, Int8, Int16, Int32, Int64,
                             UInt8, UInt16, UInt32, UInt64, Bool, Unknown };

    enum class ResourceKind { ConstantBuffer, StructuredBuffer, RWStructuredBuffer, RWByteAddressBuffer,
                               Texture2D, RWTexture2D, SamplerState, PushConstant };

    class TypeInfo;

    struct FieldInfo
    {
        std::string name;
        Box<TypeInfo> type;
        uint32_t offsetBytes;
    };

    struct StructLayout
    {
        uint32_t sizeBytes;
        std::vector<FieldInfo> fields;
    };

    struct ScalarTypeInfo   { ScalarKind kind; };
    struct VectorTypeInfo   { ScalarKind componentKind; uint32_t dimension; };
    struct MatrixTypeInfo   { ScalarKind componentKind; uint32_t rows, columns; };
    struct StructTypeInfo   { StructLayout layout; };
    struct ArrayTypeInfo    { Box<TypeInfo> elementType; std::optional<uint32_t> count; };
    struct ResourceTypeInfo { ResourceKind kind; Box<TypeInfo> elementType; };

    class TypeInfo
    {
    public:
        TypeInfo() = default;

        std::string name;
        uint32_t sizeBytes = 0;

        [[nodiscard]] bool isScalar()   const { return std::holds_alternative<ScalarTypeInfo>(m_kind); }
        [[nodiscard]] bool isVector()   const { return std::holds_alternative<VectorTypeInfo>(m_kind); }
        [[nodiscard]] bool isMatrix()   const { return std::holds_alternative<MatrixTypeInfo>(m_kind); }
        [[nodiscard]] bool isStruct()   const { return std::holds_alternative<StructTypeInfo>(m_kind); }
        [[nodiscard]] bool isArray()    const { return std::holds_alternative<ArrayTypeInfo>(m_kind); }
        [[nodiscard]] bool isResource() const { return std::holds_alternative<ResourceTypeInfo>(m_kind); }

        [[nodiscard]] const StructTypeInfo&   asStruct()   const { return std::get<StructTypeInfo>(m_kind); }
        [[nodiscard]] const ArrayTypeInfo&    asArray()    const { return std::get<ArrayTypeInfo>(m_kind); }
        [[nodiscard]] const VectorTypeInfo&   asVector()   const { return std::get<VectorTypeInfo>(m_kind); }
        [[nodiscard]] const MatrixTypeInfo&   asMatrix()   const { return std::get<MatrixTypeInfo>(m_kind); }
        [[nodiscard]] const ResourceTypeInfo& asResource() const { return std::get<ResourceTypeInfo>(m_kind); }
        [[nodiscard]] const ScalarTypeInfo&   asScalar()   const { return std::get<ScalarTypeInfo>(m_kind); }

        [[nodiscard]] const StructTypeInfo*   getStruct()   const { return std::get_if<StructTypeInfo>(&m_kind); }
        [[nodiscard]] const ArrayTypeInfo*    getArray()    const { return std::get_if<ArrayTypeInfo>(&m_kind); }
        [[nodiscard]] const VectorTypeInfo*   getVector()   const { return std::get_if<VectorTypeInfo>(&m_kind); }
        [[nodiscard]] const MatrixTypeInfo*   getMatrix()   const { return std::get_if<MatrixTypeInfo>(&m_kind); }
        [[nodiscard]] const ResourceTypeInfo* getResource() const { return std::get_if<ResourceTypeInfo>(&m_kind); }
        [[nodiscard]] const ScalarTypeInfo*   getScalar()   const { return std::get_if<ScalarTypeInfo>(&m_kind); }

        using Kind = std::variant<ScalarTypeInfo, VectorTypeInfo, MatrixTypeInfo,
                                   StructTypeInfo, ArrayTypeInfo, ResourceTypeInfo>;
        void setKind(Kind kind) { m_kind = std::move(kind); }
        Kind& getKind() { return m_kind; }
    private:
        Kind m_kind;
    };


    enum class VertexInputRate { PerVertex, PerInstance };

    struct VertexAttribute
    {
        std::string fieldName;
        std::string semanticName;
        uint32_t semanticIndex;
        ScalarKind componentKind;
        uint32_t numComponents;
        uint32_t relativeOffset;
    };

    struct VertexBufferBinding
    {
        std::string paramName;
        uint32_t inputSlot;
        uint32_t strideBytes;
        VertexInputRate inputRate;
        std::vector<VertexAttribute> attributes;
    };

    struct VertexInputLayout
    {
        std::vector<VertexBufferBinding> buffers;
    };

    enum class ShaderStage
    {
        Vertex, Pixel, Geometry, Hull, Compute, Mesh, Amplification
    };

    struct InputElement
    {
        std::string name;
        Type type;
    };

    struct EntryPoint
    {
        std::string name;
        ShaderStage stage;
        uint32_t threadsX{}, threadsY{}, threadsZ{};
        std::optional<VertexInputLayout> inputLayout;
    };

    struct ResourceBinding
    {
        std::string name;
        TypeInfo type;
    };

    struct Data
    {
        std::vector<EntryPoint> entryPoints;
        std::vector<ResourceBinding> resources;
    };
}


class Reflector
{
public:
    explicit Reflector(Module &module);
    refl::Data reflect();

    static refl::Data reflect(Module &module);
private:
    DeclTable m_declTable{};
    const std::vector<DeclNode>& m_ast;
    refl::Data m_reflection;

    void reflectDecl(const DeclNode& decl);
    void reflectFunctionDecl(const FunctionDecl& decl);

    std::optional<refl::VertexInputLayout> reflectVertexInputLayout(const FunctionDecl &entry);

    void reflectVarDecl(const VarDecl& decl);
    refl::StructLayout reflectStructLayout(const StructDecl& decl);

    refl::TypeInfo reflectType(const Type &t);

    static std::optional<refl::ShaderStage> tryGetStageAttribute(const std::vector<Attribute>& attrs);
    static bool hasAttribute(const std::string &name, const std::vector<Attribute>& attrs);
    static bool hasSystemValueAttribute(const std::vector<Attribute>& attrs);

    static refl::ScalarKind toScalarKind(PrimitiveKind kind);
    static refl::ResourceKind toResourceKind(ResourceBindingKind kind);

    static std::string primitiveDisplayName(PrimitiveKind kind);
    static std::string resourceKindName(ResourceBindingKind kind);

    static uint32_t typeSize(refl::TypeInfo type);
    static uint32_t scalarSize(refl::ScalarKind kind);
    static uint32_t primitiveSize(PrimitiveKind kind);
    static std::optional<uint32_t> tryFoldConstantInt(const ExprNode &expr);
};
}
