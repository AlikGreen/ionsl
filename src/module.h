#pragma once
#include "ast.h"
#include "diagnostics.h"

namespace ionsl
{
class Module
{
public:
    Module(std::string path, std::vector<DeclNode> decls, std::vector<Diagnostic> diagnostics)
        : path(std::move(path)), decls(std::move(decls)), diagnostics(std::move(diagnostics)) {}


    const FunctionDecl* findFunctionByName(std::string_view name) const;
    const StructDecl* findStructByName(std::string_view name) const;

    std::string path;
    std::vector<DeclNode> decls;
    std::vector<Diagnostic> diagnostics;
};
}
