#include "compiler.h"

#include "checker.h"
#include "codeGen.h"
#include "dependencyAnalyzer.h"
#include "parser.h"

namespace ionsl
{
    Module Compiler::compile(const std::string &source, const std::string &path)
    {
        auto tokens = Lexer::tokenize(source);
        auto module = Parser::parse(tokens);
        module.path = path;
        return module;
    }

    Module Compiler::link(const LinkDesc &desc)
    {
        ResolveDesc resDesc{};
        resDesc.modules = desc.modules;
        resDesc.specializations = desc.specializations;

        Module linked = Resolver::resolve(resDesc);
        linked.diagnostics.insert_range(linked.diagnostics.end(), Checker::check(linked));

        return linked;
    }

    std::string Compiler::generate(const Module &linked, const std::string& epName)
    {
        DependencyAnalyzer analyzer{linked};
        auto ep = linked.findFunctionByName(epName);
        if(!ep) return "";
        auto epOnlyAst = analyzer.collectDependencies(*ep);

        CodeGen generator{epOnlyAst};
        return generator.generate();
    }

    refl::Data Compiler::reflect(const Module &module)
    {
        return Reflector::reflect(module);
    }
}
