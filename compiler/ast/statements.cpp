#include "statements.h"

#include "declarations.h"
#include "expressions.h"

namespace ionsl
{
    BlockStmt* BlockStmt::clone(Arena &arena) const
    {
        auto* newStmt = arena.create<BlockStmt>();
        for(const auto stmt : statements)
            newStmt->statements.push_back(stmt->clone(arena));
        return newStmt;
    }

    IfStmt* IfStmt::clone(Arena &arena) const
    {
        auto* newStmt = arena.create<IfStmt>();
        newStmt->condition = condition->clone(arena);
        newStmt->thenBranch = thenBranch->clone(arena);
        if(newStmt->elseBranch) newStmt->elseBranch = elseBranch->clone(arena);
        return newStmt;
    }

    WhileStmt* WhileStmt::clone(Arena &arena) const
    {
        auto* newStmt = arena.create<WhileStmt>();
        newStmt->condition = condition->clone(arena);
        newStmt->body = body->clone(arena);
        return newStmt;
    }

    ForStmt* ForStmt::clone(Arena &arena) const
    {
        auto* newStmt = arena.create<ForStmt>();
        newStmt->init = init->clone(arena);
        newStmt->condition = condition->clone(arena);
        newStmt->increment = increment->clone(arena);
        newStmt->body = body->clone(arena);
        return newStmt;
    }

    ReturnStmt* ReturnStmt::clone(Arena &arena) const
    {
        auto* newStmt = arena.create<ReturnStmt>();
        newStmt->expr = expr->clone(arena);
        return newStmt;
    }

    BreakStmt* BreakStmt::clone(Arena &arena) const
    {
        return arena.create<BreakStmt>();
    }

    ContinueStmt* ContinueStmt::clone(Arena &arena) const
    {
        return arena.create<ContinueStmt>();
    }

    DeclStmt* DeclStmt::clone(Arena &arena) const
    {
        auto* newStmt = arena.create<DeclStmt>();
        newStmt->decl = decl->clone(arena);
        return newStmt;
    }

    ExprStmt* ExprStmt::clone(Arena &arena) const
    {
        auto* newStmt = arena.create<ExprStmt>();
        newStmt->expr = expr->clone(arena);
        return newStmt;
    }
}
