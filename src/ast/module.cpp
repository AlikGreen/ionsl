#include "module.h"

#include "declarations.h"

namespace ionsl
{
    Module Module::clone() const
    {
        Module newAst{arena.capacity()};
        for(const auto* decl : declarations)
            newAst.declarations.push_back(decl->clone(newAst.arena));

        return std::move(newAst);
    }

    Module::Module(const size_t arenaSize)
        : arena(arenaSize)
    {

    }
}
