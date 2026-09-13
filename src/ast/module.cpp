#include "module.h"

#include "declarations.h"

namespace ionsl
{
    Module Module::clone() const
    {
        Module newModule{arena.capacity()};
        clone(newModule);
        return std::move(newModule);
    }

    void Module::clone(Module &newModule) const
    {
        for(const auto* decl : declarations)
            newModule.declarations.push_back(decl->clone(newModule.arena));
    }

    Module::Module(const size_t arenaSize)
        : arena(arenaSize)
    {

    }
}
