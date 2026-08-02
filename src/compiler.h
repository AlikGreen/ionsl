#pragma once
#include "module.h"
#include "resolver.h"

namespace ionsl
{
struct LinkDesc
{
    std::vector<Module> modules;
    std::vector<SpecializationRequest> specializations;
};

class Compiler
{
public:
    static Module compile(const std::string &source, const std::string &path);
    static Module link(const LinkDesc& desc);
    static std::string generate(const Module& linked);
};
}
