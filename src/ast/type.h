#pragma once
#include <cstdint>
#include <optional>
#include <variant>
#include "decl.h"
#include "../common/diagnostics.h"

namespace ionsl
{
class TypeId
{
public:
    constexpr TypeId() = default;
    constexpr explicit TypeId(const uint32_t value) : m_value(value) {}

    [[nodiscard]] constexpr uint32_t value() const { return m_value; }
    friend constexpr bool operator==(TypeId, TypeId) = default;

    static const TypeId Error;
    static const TypeId Void;
    static const TypeId Bool;
    static const TypeId I8, I16, I32, I64;
    static const TypeId U8, U16, U32, U64;
    static const TypeId F16, F32, F64;
    static const TypeId String;
private:
    uint32_t m_value = 0;
};

constexpr TypeId TypeId::Error   = TypeId(0);
constexpr TypeId TypeId::Void    = TypeId(1);
constexpr TypeId TypeId::Bool    = TypeId(3);
constexpr TypeId TypeId::I8      = TypeId(4);
constexpr TypeId TypeId::I16     = TypeId(5);
constexpr TypeId TypeId::I32     = TypeId(6);
constexpr TypeId TypeId::I64     = TypeId(7);
constexpr TypeId TypeId::U8      = TypeId(8);
constexpr TypeId TypeId::U16     = TypeId(9);
constexpr TypeId TypeId::U32     = TypeId(10);
constexpr TypeId TypeId::U64     = TypeId(11);
constexpr TypeId TypeId::F16     = TypeId(12);
constexpr TypeId TypeId::F32     = TypeId(13);
constexpr TypeId TypeId::F64     = TypeId(14);
constexpr TypeId TypeId::String  = TypeId(15);

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

struct GenericType
{
    DeclId declId = InvalidDeclId;
};

struct AutoType { };
struct ErrorType { };

using TypeKind = std::variant<
    PrimitiveType,
    VectorType,
    MatrixType,
    ArrayType,
    StructType,
    InterfaceType,
    AutoType,
    GenericType,
    ErrorType
>;

class TypeInfo
{
public:
    TypeKind kind{};

    template<typename T>
    [[nodiscard]] bool is() const
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