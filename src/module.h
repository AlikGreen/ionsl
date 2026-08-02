#pragma once
#include "ast.h"
#include "diagnostics.h"

namespace ionsl
{
class Module
{
public:
    Module(std::string path, std::vector<DeclNode> decls, std::vector<Diagnostic> diagnostics)
        : m_path(std::move(path)), m_decls(std::move(decls)), m_diagnostics(std::move(diagnostics)) {}

    [[nodiscard]] std::string path() const { return m_path; }
    void path(const std::string &path) { m_path = path; }
    [[nodiscard]] const std::vector<DeclNode>& ast() const { return m_decls; }
    [[nodiscard]] const std::vector<Diagnostic>& diagnostics() const { return m_diagnostics; }

    const FunctionDecl* findFunctionByName(std::string_view name) const;
    const StructDecl* findStructByName(std::string_view name) const;

private:
    std::string m_path;
    std::vector<DeclNode> m_decls;
    std::vector<Diagnostic> m_diagnostics;
};
}
