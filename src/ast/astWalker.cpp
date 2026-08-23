#include "AstWalker.h"

#include "declarations.h"
#include "statements.h"
#include "expressions.h"
#include "typeSyntax.h"
#include "module.h"


namespace ionsl
{
    AstWalker& AstWalker::on(DeclarationCallback callback)
    {
        m_declarationCallback = std::move(callback);
        return *this;
    }

    AstWalker& AstWalker::on(StatementCallback callback)
    {
        m_statementCallback = std::move(callback);
        return *this;
    }

    AstWalker& AstWalker::on(ExpressionCallback callback)
    {
        m_expressionCallback = std::move(callback);
        return *this;
    }

    AstWalker& AstWalker::on(TypeSyntaxCallback callback)
    {
        m_typeSyntaxCallback = std::move(callback);
        return *this;
    }

    void AstWalker::walk(Module& module)
    {
        for(auto* declaration : module.declarations)
        {
            if(declaration != nullptr)
            {
                walk(*declaration);
            }
        }
    }

    void AstWalker::walk(AstNode& node)
    {
        if(auto* declaration = node.as<Declaration>())
        {
            walkDeclaration(*declaration);
            return;
        }

        if(auto* statement = node.as<Statement>())
        {
            walkStatement(*statement);
            return;
        }

        if(auto* expression = node.as<Expression>())
        {
            walkExpression(*expression);
            return;
        }

        if(auto* typeSyntax = node.as<TypeSyntax>())
        {
            walkTypeSyntax(*typeSyntax);
            return;
        }
    }

    void AstWalker::walkDeclaration(Declaration& declaration)
    {
        if(m_declarationCallback)
        {
            m_declarationCallback(declaration);
        }

        if(auto* function = declaration.as<FunctionDecl>())
        {
            if(function->returnType != nullptr)
            {
                walk(*function->returnType);
            }

            for(auto* parameter : function->params)
            {
                if(parameter != nullptr)
                {
                    walk(*parameter);
                }
            }

            if(function->body != nullptr)
            {
                walk(*function->body);
            }

            return;
        }

        if(auto* structure = declaration.as<StructDecl>())
        {
            for(auto* field : structure->fields)
            {
                if(field != nullptr)
                {
                    walk(*field);
                }
            }

            return;
        }

        if(auto* value = declaration.as<ValueDecl>())
        {
            if(value->type != nullptr)
            {
                walk(*value->type);
            }

            if(value->initializer != nullptr)
            {
                walk(*value->initializer);
            }
        }
    }

    void AstWalker::walkStatement(Statement& statement)
    {
        if(m_statementCallback)
        {
            m_statementCallback(statement);
        }

        if(auto* block = statement.as<BlockStmt>())
        {
            for(auto* child : block->statements)
            {
                if(child != nullptr)
                {
                    walk(*child);
                }
            }

            return;
        }

        if(auto* ifStatement = statement.as<IfStmt>())
        {
            if(ifStatement->condition != nullptr) walk(*ifStatement->condition);
            if(ifStatement->thenBranch != nullptr) walk(*ifStatement->thenBranch);
            if(ifStatement->elseBranch != nullptr) walk(*ifStatement->elseBranch);

            return;
        }

        if(auto* returnStatement = statement.as<ReturnStmt>())
        {
            if(returnStatement->expr != nullptr)
            {
                walk(*returnStatement->expr);
            }

            return;
        }
    }

    void AstWalker::walkExpression(Expression& expression)
    {
        if(m_expressionCallback)
        {
            m_expressionCallback(expression);
        }

        if(const auto* binary = expression.as<BinaryExpr>())
        {
            if(binary->left != nullptr)
            {
                walk(*binary->left);
            }

            if(binary->right != nullptr)
            {
                walk(*binary->right);
            }

            return;
        }

        if(const auto* unary = expression.as<UnaryExpr>())
        {
            if(unary->operand != nullptr)
            {
                walk(*unary->operand);
            }

            return;
        }

        if(const auto* call = expression.as<CallExpr>())
        {
            if(call->callee != nullptr)
            {
                walk(*call->callee);
            }

            for(auto* argument : call->args)
            {
                if(argument != nullptr)
                {
                    walk(*argument);
                }
            }

            return;
        }

        if(const auto* member = expression.as<FieldAccessExpr>())
        {
            if(member->object != nullptr)
            {
                walk(*member->object);
            }

            return;
        }

        if(auto* index = expression.as<IndexExpr>())
        {
            if(index->array != nullptr)
            {
                walk(*index->array);
            }

            if(index->index != nullptr)
            {
                walk(*index->index);
            }

            return;
        }
    }

    void AstWalker::walkTypeSyntax(TypeSyntax& type)
    {
        if(m_typeSyntaxCallback)
        {
            m_typeSyntaxCallback(type);
        }

        if(auto* named = type.as<NamedTypeSyntax>())
        {
            for(auto* argument : named->arguments)
            {
                if(argument != nullptr)
                {
                    walk(*argument);
                }
            }

            return;
        }

        if(auto* array = type.as<ArrayTypeSyntax>())
        {
            if(array->elementType != nullptr)
            {
                walk(*array->elementType);
            }

            if(array->size != nullptr)
            {
                walk(*array->size);
            }
        }
    }
}