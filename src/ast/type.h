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
constexpr TypeId TypeIdBool = 1;
constexpr TypeId TypeIdU64 = 2;
constexpr TypeId TypeIdI64 = 3;
constexpr TypeId TypeIdF64 = 4;
constexpr TypeId TypeIdString = 5;

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

    bool operator==(const ArrayType&) const = default;
};

struct VectorType
{
    TypeId scalarType;
    uint32_t dimension;

    bool operator==(const VectorType&) const = default;
};

struct MatrixType
{
    TypeId scalarType;
    uint32_t rows;
    uint32_t columns;

    bool operator==(const MatrixType&) const = default;
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
    TypeKind kind{};

    template<typename T>
    bool is() const
    {
        return std::holds_alternative<T>(kind);
    }

    template<typename T>
    T* as()
    {
        return std::get_if<T>(&kind);
    }
};

template<typename T>
void hashCombine(size_t& seed, const T& value)
{
    seed ^= std::hash<T>{}(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}
}


template<>
struct std::hash<ionsl::ArrayType>
{
    size_t operator()(const ionsl::ArrayType& value) const noexcept
    {
        size_t seed = 0;

        ionsl::hashCombine(seed, value.elementType);
        if(value.size.has_value())
            ionsl::hashCombine(seed, value.size.value());

        return seed;
    }
};

template<>
struct std::hash<ionsl::VectorType>
{
    size_t operator()(const ionsl::VectorType& value) const noexcept
    {
        size_t seed = 0;

        ionsl::hashCombine(seed, value.scalarType);
        ionsl::hashCombine(seed, value.dimension);

        return seed;
    }
};

template<>
struct std::hash<ionsl::MatrixType>
{
    size_t operator()(const ionsl::MatrixType& value) const noexcept
    {
        size_t seed = 0;

        ionsl::hashCombine(seed, value.scalarType);
        ionsl::hashCombine(seed, value.rows);
        ionsl::hashCombine(seed, value.columns);

        return seed;
    }
};