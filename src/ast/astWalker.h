#pragma once

#include <functional>


namespace ionsl
{
class AstNode;
class Module;
class Declaration;
class Statement;
class Expression;
class TypeSyntax;

class AstWalker
{
public:
    using DeclarationCallback = std::function<void(Declaration&)>;
    using StatementCallback = std::function<void(Statement&)>;
    using ExpressionCallback = std::function<void(Expression&)>;
    using TypeSyntaxCallback = std::function<void(TypeSyntax&)>;

    AstWalker& on(DeclarationCallback callback);
    AstWalker& on(StatementCallback callback);
    AstWalker& on(ExpressionCallback callback);
    AstWalker& on(TypeSyntaxCallback callback);

    void walk(const Module& module);
    void walk(AstNode &node);
private:
    void walkDeclaration(Declaration& declaration);
    void walkStatement(Statement& statement);
    void walkExpression(Expression& expression);
    void walkTypeSyntax(TypeSyntax& type);

    DeclarationCallback m_declarationCallback;
    StatementCallback   m_statementCallback;
    ExpressionCallback  m_expressionCallback;
    TypeSyntaxCallback  m_typeSyntaxCallback;
};
}