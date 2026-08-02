#pragma once
#include <cstdint>

namespace ionsl
{
struct SourceLocation
{
    uint32_t line, column, offset;
};

struct SourceSpan
{
    SourceLocation startLoc;
    SourceLocation endLoc;

    static SourceSpan between(const SourceSpan first, const SourceSpan last)
    {
        return SourceSpan{first.startLoc, last.endLoc};
    }
};
}
