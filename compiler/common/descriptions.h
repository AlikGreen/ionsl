#pragma once
#include <string>

#include "../ast/decl.h"

namespace ionsl
{
    struct EntrypointSpecialization
    {
        DeclId entrypoint = DeclId::Error;
        std::vector<TypeId> typeArgs{};
        std::string newName{};
    };
}
