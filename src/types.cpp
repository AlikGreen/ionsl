#include "types.h"
#include "ast.h"

namespace ionsl
{
    Type Type::invalid()
    {
        return Type { .kind = InvalidType {} };
    }
}
