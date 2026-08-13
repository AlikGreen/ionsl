#pragma once
#include <cstdint>

namespace ionsl
{
struct SourceLocation
{
    uint32_t line = 1, column = 1, offset = 0;
};

struct SourceSpan
{
    SourceSpan() = default;
    explicit SourceSpan(const SourceLocation start, const SourceLocation end)
        : startLoc(start), endLoc(end) { }
    SourceLocation startLoc;
    SourceLocation endLoc;

    static SourceSpan between(const SourceSpan first, const SourceSpan last)
    {
        return SourceSpan{first.startLoc, last.endLoc};
    }
};
}
