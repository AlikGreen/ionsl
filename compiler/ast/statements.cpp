#include "statements.h"

#include "declarations.h"
#include "expressions.h"

namespace ionsl
{
    BlockStmt* BlockStmt::clone(Arena &arena) const
    {
        auto* newStmt = arena.create<BlockStmt>();

        newStmt->span = span;
        newStmt->scope = scope;

        for(const auto stmt : statements)
            newStmt->statements.push_back(stmt->clone(arena));

        return newStmt;
    }

    IfStmt* IfStmt::clone(Arena &arena) const
    {
        auto* newStmt = arena.create<IfStmt>();
        newStmt->span = span;
        newStmt->condition = condition->clone(arena);
        newStmt->thenBranch = thenBranch->clone(arena);
        if(elseBranch) newStmt->elseBranch = elseBranch->clone(arena);
        return newStmt;
    }

    WhileStmt* WhileStmt::clone(Arena &arena) const
    {
        auto* newStmt = arena.create<WhileStmt>();
        newStmt->span = span;
        newStmt->condition = condition->clone(arena);
        newStmt->body = body->clone(arena);
        return newStmt;
    }

    ForStmt* ForStmt::clone(Arena &arena) const
    {
        auto* newStmt = arena.create<ForStmt>();
        newStmt->span = span;
        newStmt->init = init->clone(arena);
        newStmt->condition = condition->clone(arena);
        newStmt->increment = increment->clone(arena);
        newStmt->body = body->clone(arena);
        return newStmt;
    }

    ReturnStmt* ReturnStmt::clone(Arena &arena) const
    {
        auto* newStmt = arena.create<ReturnStmt>();
        newStmt->span = span;

        if (expr)
            newStmt->expr = expr->clone(arena);

        return newStmt;
    }

    BreakStmt* BreakStmt::clone(Arena &arena) const
    {
        auto* newStmt = arena.create<BreakStmt>();
        newStmt->span = span;
        return newStmt;
    }

    ContinueStmt* ContinueStmt::clone(Arena &arena) const
    {
        auto* newStmt = arena.create<ContinueStmt>();
        newStmt->span = span;
        return newStmt;
    }

    DeclStmt* DeclStmt::clone(Arena &arena) const
    {
        auto* newStmt = arena.create<DeclStmt>();
        newStmt->span = span;
        newStmt->decl = decl->clone(arena);
        return newStmt;
    }

    ExprStmt* ExprStmt::clone(Arena &arena) const
    {
        auto* newStmt = arena.create<ExprStmt>();
        newStmt->span = span;
        newStmt->expr = expr->clone(arena);
        return newStmt;
    }
}
