#pragma once
#include "astNode.h"
#include "decl.h"
#include "literalValue.h"
#include "qualifiedName.h"
#include "symbol.h"
#include "type.h"
#include "../common/arena.h"
#include "../common/enums.h"

namespace ionsl
{
class Expression : public AstNode
{
public:
    Expression* clone(Arena& arena) const override = 0;

    TypeId resultType = TypeIdInvalid;
};

class BinaryExpr final : public Expression
{
public:
    BinaryOp op = BinaryOp::Add;
    Expression* left{};
    Expression* right{};

    BinaryExpr* clone(Arena &arena) const override;
};

class UnaryExpr final : public Expression
{
public:
    UnaryOp op = UnaryOp::Negate;
    Expression* operand{};

    UnaryExpr* clone(Arena &arena) const override;
};

class LiteralExpr final : public Expression
{
public:
    LiteralValue literal;

    LiteralExpr* clone(Arena &arena) const override;
};

class IdentifierExpr final : public Expression
{
public:
    QualifiedName name;
    DeclId decl{};

    IdentifierExpr* clone(Arena &arena) const override;
};

class CallExpr final : public Expression
{
public:
    Expression* callee{};
    std::vector<Expression*> args{};

    CallExpr* clone(Arena &arena) const override;
};

class FieldAccessExpr final : public Expression
{
public:
    Expression* object{};
    SymbolId memberName = ~0u;

    FieldAccessExpr* clone(Arena &arena) const override;
};

class IndexExpr final : public Expression
{
public:
    Expression* array{};
    Expression* index{};

    IndexExpr* clone(Arena &arena) const override;
};

enum class ConversionKind
{
    Implicit,
    Explicit
};

class ConversionExpr final : public Expression
{
public:
    Expression* operand{};
    TypeId targetType = TypeIdInvalid;
    ConversionKind kind = ConversionKind::Implicit;

    ConversionExpr* clone(Arena &arena) const override;
};

class ErrorExpr final : public Expression
{
    ErrorExpr* clone(Arena &arena) const override;
};
}
