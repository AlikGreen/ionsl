#pragma once
#include <vector>

#include "astNode.h"



namespace ionsl
{
using ScopeId = uint32_t;

// TODO switch/match stmt

class Expression;
class Declaration;
class Statement : public AstNode
{
public:
    Statement* clone(Arena& arena) const override = 0;
};

class BlockStmt final : public Statement
{
public:
    ScopeId scope = ~0u;
    std::vector<Statement*> statements{};

    BlockStmt* clone(Arena &arena) const override;
};

class IfStmt final : public Statement
{
public:
    Expression* condition{};
    BlockStmt* thenBranch{};
    BlockStmt* elseBranch{}; // can be nullptr

    IfStmt* clone(Arena &arena) const override;
};

class WhileStmt final : public Statement
{
public:
    Expression* condition{};
    BlockStmt* body{};

    WhileStmt* clone(Arena &arena) const override;
};

class ForStmt final : public Statement
{
public:
    Statement* init{};
    Expression* condition{};
    Expression* increment{};
    BlockStmt* body{};

    ForStmt* clone(Arena &arena) const override;
};

class ReturnStmt final : public Statement
{
public:
    Expression* expr{};

    ReturnStmt* clone(Arena &arena) const override;
};

class BreakStmt final : public Statement
{
public:
    BreakStmt* clone(Arena &arena) const override;
};

class ContinueStmt final : public Statement
{
public:
    ContinueStmt* clone(Arena &arena) const override;
};

class DeclStmt final : public Statement
{
public:
    Declaration* decl{};

    DeclStmt* clone(Arena &arena) const override;
};

class ExprStmt final : public Statement
{
public:
    Expression* expr{};

    ExprStmt* clone(Arena &arena) const override;
};
}
