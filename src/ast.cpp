#include "ast.h"

namespace ionsl
{
    DeclId nextDeclId()
    {
        static std::atomic<DeclId> counter{1};
        return counter.fetch_add(1, std::memory_order_relaxed);
    }
}