#pragma once

#include <variant>
#include <string>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

#include "box.h"
#include "lexer.h"

namespace ionsl
{
struct Type;

struct QualifiedName
{
    std::vector<std::string> segments;

    [[nodiscard]] std::string name() const
    {
        return segments.empty() ? "" : segments.back();
    }

    [[nodiscard]] std::string fullName() const
    {
        std::string fullName;

        for(size_t i = 0; i < segments.size(); i++)
        {
            fullName += segments[i];
            if(i != segments.size() - 1)
                fullName += "::";
        }

        return fullName;
    }
};

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

inline static const std::unordered_map<std::string_view, PrimitiveKind> kPrimitiveTypeMap = {
    { "void",     PrimitiveKind::Void },
    { "bool",     PrimitiveKind::Bool },

    { "i8",       PrimitiveKind::Int8 },
    { "i16",      PrimitiveKind::Int16 },
    { "i32",      PrimitiveKind::Int32 },
    { "i64",      PrimitiveKind::Int64 },

    { "u8",       PrimitiveKind::UInt8 },
    { "u16",      PrimitiveKind::UInt16 },
    { "u32",      PrimitiveKind::UInt32 },
    { "u64",      PrimitiveKind::UInt64 },

    { "f16",      PrimitiveKind::Float16 },
    { "f32",      PrimitiveKind::Float32 },
    { "f64",      PrimitiveKind::Float64 },

    { "string",   PrimitiveKind::String }
};

struct Trivia
{
    std::string text;
    bool isBlockComment;
    SourceSpan span;
};

struct PrimitiveType
{
    PrimitiveKind kind;
};

struct ExprNode;

struct ArrayType
{
    Box<Type> elementType;
    std::optional<Box<ExprNode>> size;
};

struct VectorType
{
    Box<ExprNode> scalarType;
    Box<ExprNode> dimension;
};

struct MatrixType
{
    Box<ExprNode> scalarType;
    Box<ExprNode> rows;
    Box<ExprNode> columns;
};

struct CustomType
{
    QualifiedName name;
    std::vector<Box<Type>> genericArgs;
};

using DeclId = uint64_t;

struct StructDecl;
struct StructType
{
    DeclId declId;
};

struct InterfaceDecl;
struct InterfaceType
{
    DeclId declId;
};

struct AutoType { };
struct InvalidType { };

using TypeKind = std::variant<
    PrimitiveType,
    CustomType,
    VectorType,
    MatrixType,
    ArrayType,
    StructType,
    InterfaceType,
    AutoType,
    InvalidType
>;

struct Type
{
    std::vector<Trivia> trivia;
    SourceSpan span;
    TypeKind kind;

    static Type invalid();
};

inline Box<PrimitiveKind> nameToPrimitiveKind(const std::string_view name)
{
    if(const auto it = kPrimitiveTypeMap.find(name); it != kPrimitiveTypeMap.end())
        return Box<PrimitiveKind>::make(it->second);

    return nullptr;
}
}
