#include "compiler.h"

#include "codeGen.h"
#include "parser.h"

namespace ionsl
{
    Module Compiler::compile(const std::string &source, const std::string &path)
    {
        auto tokens = Lexer::tokenize(source);
        auto module = Parser::parse(tokens);
        module.path(path);
        return module;
    }

    Module Compiler::link(const LinkDesc &desc)
    {
        ResolveDesc resDesc{};
        resDesc.modules = desc.modules;
        resDesc.specializations = desc.specializations;

        Module linked = Resolver::resolve(resDesc);

        return linked;
    }

    std::string Compiler::generate(const Module &linked)
    {
        CodeGen generator{linked};
        return generator.generate();
    }
}
