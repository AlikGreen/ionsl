#include "constantEvaluator.h"

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

        return std::nullopt;
    }

    std::optional<ConstantValue> ConstantEvaluator::evaluateBinaryExpr(const BinaryExpr &expr)
    {
        const auto left = evaluate(*expr.left);
        const auto right = evaluate(*expr.right);

        if(!left || !right) return std::nullopt;

        TypeId resultType = expr.resultType;

        return std::nullopt; // FIXME
    }

    std::optional<ConstantValue> ConstantEvaluator::evaluateUnaryExpr(UnaryExpr &expr)
    {
        return std::nullopt; // FIXME
    }

    std::optional<ConstantValue> ConstantEvaluator::evaluateLiteralExpr(const LiteralExpr &expr)
    {
        ConstantValue constVal{};
        constVal.value = expr.literal;
        constVal.type = expr.resultType;
        return constVal;
    }
}
