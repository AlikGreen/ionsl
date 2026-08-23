#include "ast.h"

#include "declarations.h"

namespace ionsl
{
    Ast Ast::clone() const
    {
        Ast newAst{arena.capacity()};
        for(const auto* decl : declarations)
            newAst.declarations.push_back(decl->clone(newAst.arena));

        return std::move(newAst);
    }

    Ast::Ast(const size_t arenaSize)
        : arena(arenaSize)
    {

    }
}
