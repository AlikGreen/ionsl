#include "constantEvaluator.h"

namespace ionsl
{
    std::optional<ConstantValue> ConstantEvaluator::evaluate(Expression &expr, const DeclTable &declTable)
    {
        return ConstantEvaluator(declTable).evaluate(expr);
    }

    ConstantEvaluator::ConstantEvaluator(const DeclTable &declTable)
        : m_declTable(declTable) { }

    std::optional<ConstantValue> ConstantEvaluator::evaluate(Expression& expr)
    {
        if(auto* binary = expr.as<BinaryExpr>())
            return evaluateBinaryExpr(*binary);
        if(auto* unary = expr.as<UnaryExpr>())
            return evaluateUnaryExpr(*unary);

        return std::nullopt;
    }

    std::optional<ConstantValue> ConstantEvaluator::evaluateBinaryExpr(const BinaryExpr &expr)
    {
        const auto left = evaluate(*expr.left);
        const auto right = evaluate(*expr.right);

        if(!left.has_value() || !right.has_value()) return std::nullopt;
    }
}
