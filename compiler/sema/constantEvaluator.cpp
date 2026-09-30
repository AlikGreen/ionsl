#include "constantEvaluator.h"

#include "../ast/declarations.h"

namespace ionsl
{
    ConstantEvaluator::ConstantEvaluator(const DeclTable &declTable, const TypeSystem& typeSystem)
        : m_declTable(declTable), m_typeSystem(typeSystem) { }

    std::optional<ConstantValue> ConstantEvaluator::evaluate(Expression& expr)
    {
        if(auto* binary = expr.as<BinaryExpr>())
            return evaluateBinaryExpr(*binary);
        if(auto* unary = expr.as<UnaryExpr>())
            return evaluateUnaryExpr(*unary);
        if(auto* literal = expr.as<LiteralExpr>())
            return evaluateLiteralExpr(*literal);
        if(auto* identifier = expr.as<IdentifierExpr>())
            return evaluateIdentifierExpr(*identifier);

        return std::nullopt;
    }

    std::optional<ConstantValue> ConstantEvaluator::evaluateBinaryExpr(const BinaryExpr &expr)
    {
        const auto left = evaluate(*expr.left);
        const auto right = evaluate(*expr.right);

        if(!left || !right) return std::nullopt;

        TypeId resultType = expr.resultType;

        throw std::runtime_error("not implemented");
    }

    std::optional<ConstantValue> ConstantEvaluator::evaluateUnaryExpr(UnaryExpr &expr)
    {
        throw std::runtime_error("not implemented");
    }

    std::optional<ConstantValue> ConstantEvaluator::evaluateLiteralExpr(const LiteralExpr &expr)
    {
        return expr.value;
    }

    std::optional<ConstantValue> ConstantEvaluator::evaluateIdentifierExpr(const IdentifierExpr &expr)
    {
        const auto var = m_declTable.get(expr.decl)->as<ValueDecl>();
        if (!var || !var->initializer) return std::nullopt;
        return evaluate(*var->initializer);
    }
}
