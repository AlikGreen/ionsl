#pragma once
#include <unordered_set>
#include <utility>

#include "astWalker.h"
#include "declTable.h"
#include "module.h"

namespace ionsl
{
class DependencyAnalyzer
{
public:
    explicit DependencyAnalyzer(Module module) : m_module(std::move(module)) {}
    Module collectDependencies(const FunctionDecl& entry);
private:
    void visitByName(const std::string& name);
    DeclNode* findDeclNode(const std::string& name);

    Module m_module;
    std::unordered_set<std::string> m_visitedNames;
    std::vector<DeclNode> m_closure;
    DeclTable m_declTable{};
};
}
