#pragma once
#include <string>
#include <variant>
#include <cstdint>

namespace ionsl
{
enum class IntKind
{
    Signed,
    Unsigned
};

struct ConstantInt
{
    uint64_t value{};
    IntKind kind{};
};

enum class FloatKind
{
    Half,
    Float,
    Double
};

struct ConstantFloat
{
    double value{};
    FloatKind kind{};
};

using ConstantValue = std::variant<
    bool,
    ConstantInt,
    ConstantFloat,
    std::string
>;
}
