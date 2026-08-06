#pragma once
#include <unordered_set>

#include "astWalker.h"
#include "module.h"

namespace ionsl
{
class DependencyAnalyzer
{
public:
    explicit DependencyAnalyzer(const Module& module) : m_module(module) {}
    Module collectDependencies(const FunctionDecl& entry);
private:
    void visitByName(const std::string& name);
    const DeclNode* findDeclNode(const std::string& name);

    const Module& m_module;
    std::unordered_set<std::string> m_visitedNames;
    std::vector<DeclNode> m_closure;
};
}
