#pragma once
#include <cstdint>
#include <optional>
#include <variant>
#include "decl.h"
#include "../common/diagnostics.h"

namespace ionsl
{
using TypeId = uint32_t;

constexpr TypeId TypeIdInvalid = 0;
constexpr TypeId TypeIdVoid = 0;

enum class PrimitiveKind : uint8_t
{
    Void,
    Bool,

    Int8,
    Int16,
    Int32,
    Int64,

    UInt8,
    UInt16,
    UInt32,
    UInt64,

    Float16,
    Float32,
    Float64,

    String,

    Unknown
};

class Expression;

struct PrimitiveType
{
    PrimitiveKind kind;
};

struct ArrayType
{
    TypeId elementType;
    std::optional<uint32_t> size;
};

struct VectorType
{
    TypeId scalarType;
    uint32_t dimension;
};

struct MatrixType
{
    TypeId scalarType;
    uint32_t rows;
    uint32_t columns;
};

struct StructType
{
    DeclId declId = InvalidDeclId;
};

struct InterfaceType
{
    DeclId declId = InvalidDeclId;
};

struct AutoType { };
struct InvalidType { };

using TypeKind = std::variant<
    PrimitiveType,
    VectorType,
    MatrixType,
    ArrayType,
    StructType,
    InterfaceType,
    AutoType,
    InvalidType
>;

class TypeInfo
{
public:
    SourceSpan span{};
    TypeKind kind{};
};
}
