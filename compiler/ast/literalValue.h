#pragma once
#include <string>
#include <variant>
#include <cstdint>

namespace ionsl
{
using LiteralValue = std::variant<
    bool,
    int64_t,
    uint64_t,
    double,
    std::string
>;
}
