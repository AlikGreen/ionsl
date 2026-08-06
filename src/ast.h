#pragma once
#include <memory>
#include <string>
#include <variant>

#include "box.h"
#include "enums.h"
#include "types.h"

namespace ionsl
{
enum class IntegerSuffix { None, Unsigned, Long };
enum class FloatSuffix   { None, Half, Explicit };

struct IntegerLiteral { int64_t value; IntegerSuffix suffix; bool isHex; };
struct FloatLiteral   { double value; FloatSuffix suffix; };

using Literal = std::variant<std::string, IntegerLiteral, FloatLiteral, bool>;

struct Attribute
{
    QualifiedName name;
    std::vector<Literal> args;
    SourceSpan span;
};

struct ExprNode;

struct UnaryExpr { UnaryOp op; Box<ExprNode> operand; };
struct BinaryExpr { BinaryOp op; Box<ExprNode> left; Box<ExprNode> right; };
struct FieldAccessExpr { Box<ExprNode> object; std::string memberName; };
struct FunctionCallExpr { Box<ExprNode> callee; std::vector<ExprNode> args; };
struct IndexExpr { Box<ExprNode> array; Box<ExprNode> index; };
struct TypeExpr { Type type; };

struct IdentifierExpr{ std::string name; std::vector<Type> genericArgs; };
struct ErrorExpr{ };

using Expr = std::variant<
    Literal,
    IdentifierExpr,
    BinaryExpr,
    UnaryExpr,
    FunctionCallExpr,
    FieldAccessExpr,
    IndexExpr,
    TypeExpr,
    ErrorExpr
>;

struct ExprNode
{
    std::vector<Trivia> trivia;
    SourceSpan span;

    Expr expr;
};

enum class ParamModifier : uint8_t
{
    None,
    In,
    Out,
    InOut,
    Const
};

struct ParamDecl
{
    SourceSpan span;
    std::vector<Trivia> trivia;

    std::string name;
    Type type;
    ParamModifier modifier = ParamModifier::None;

    std::vector<Attribute> attributes;
    Box<ExprNode> defaultValue;
};

struct StmtNode;

struct BlockStmt
{
    std::vector<StmtNode> statements;
};

struct GenericArg
{
    Type type;
    std::optional<Type> interfaceType;
};

struct FunctionDecl
{
    std::string name;
    std::vector<ParamDecl> params;
    Type returnType;
    std::vector<GenericArg> genericArgs;
    std::optional<BlockStmt> body;
    std::vector<Attribute> attributes;
};

struct StructField
{
    SourceSpan span;
    std::vector<Trivia> trivia;

    std::string name;
    Type type;

    std::vector<Attribute> attributes;

    Box<ExprNode> initializer{};
};

struct Method
{
    SourceSpan span;
    std::vector<Trivia> trivia;

    FunctionDecl decl;
};

struct StructDecl
{
    std::string name;
    std::vector<StructField> fields;
    std::vector<Attribute> attributes;
    std::vector<Method> methods;
    std::vector<Type> interfaces;
};

struct InterfaceDecl
{
    std::string name;
    std::vector<Method> methods;
    std::vector<Attribute> attributes;
};

enum class VarModifier
{
    None, Mutable
};

struct VarDecl
{
    std::string name;
    Type type;
    Box<ExprNode> initializer;
    std::vector<Attribute> attributes;
    VarModifier modifier = VarModifier::None;
};



using Decl = std::variant<
    FunctionDecl,
    VarDecl,
    StructDecl,
    InterfaceDecl
>;


struct DeclNode
{
    std::vector<Trivia> trivia;
    Decl decl;
    SourceSpan span;
};

struct IfStmt
{
    ExprNode condition;
    BlockStmt thenBranch;
    std::optional<BlockStmt> elseBranch;
};

struct WhileStmt
{
    ExprNode condition;
    BlockStmt body;
};

struct ForStmt
{
    Box<StmtNode> init;
    ExprNode condition;
    ExprNode increment;
    BlockStmt body;
};

struct DeclStmt
{
    DeclNode decl;
};

struct ReturnStmt
{
    std::optional<ExprNode> expr;
};

struct BreakStmt { };
struct ContinueStmt { };

struct ExprStmt
{
    ExprNode expr;
};

using Stmt = std::variant<
    IfStmt,
    BlockStmt,
    ReturnStmt,
    ExprStmt,
    DeclStmt,
    WhileStmt,
    ForStmt,
    BreakStmt,
    ContinueStmt
>;

struct StmtNode
{
    std::vector<Trivia> trivia;
    Stmt stmt;
    SourceSpan span;
};
}
