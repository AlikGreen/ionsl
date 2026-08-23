#include "expressions.h"

namespace ionsl
{
    BinaryExpr* BinaryExpr::clone(Arena &arena) const
    {
        auto* newExpr = arena.create<BinaryExpr>();
        newExpr->span = span;
        newExpr->op = op;
        newExpr->left = left->clone(arena);
        newExpr->right = right->clone(arena);
        return newExpr;
    }

    UnaryExpr* UnaryExpr::clone(Arena &arena) const
    {
        auto* newExpr = arena.create<UnaryExpr>();
        newExpr->span = span;
        newExpr->op = op;
        newExpr->operand = operand->clone(arena);
        return newExpr;
    }

    LiteralExpr* LiteralExpr::clone(Arena &arena) const
    {
        auto* newExpr = arena.create<LiteralExpr>();
        newExpr->span = span;
        newExpr->literal = literal;
        return newExpr;
    }

    IdentifierExpr* IdentifierExpr::clone(Arena &arena) const
    {
        auto* newExpr = arena.create<IdentifierExpr>();
        newExpr->span = span;
        newExpr->name = name;
        return newExpr;
    }

    CallExpr* CallExpr::clone(Arena &arena) const
    {
        auto* newExpr = arena.create<CallExpr>();
        newExpr->span = span;
        newExpr->callee = callee->clone(arena);
        for(auto* arg : args)
            newExpr->args.push_back(arg->clone(arena));
        return newExpr;
    }

    FieldAccessExpr* FieldAccessExpr::clone(Arena &arena) const
    {
        auto* newExpr = arena.create<FieldAccessExpr>();
        newExpr->span = span;
        newExpr->object = object->clone(arena);
        newExpr->memberName = memberName;
        return newExpr;
    }

    IndexExpr* IndexExpr::clone(Arena &arena) const
    {
        auto* newExpr = arena.create<IndexExpr>();
        newExpr->span = span;
        newExpr->index = index->clone(arena);
        newExpr->array = array->clone(arena);
        return newExpr;
    }

    ErrorExpr* ErrorExpr::clone(Arena &arena) const
    {
        auto* newExpr = arena.create<ErrorExpr>();
        newExpr->span = span;
        return newExpr;
    }
}
