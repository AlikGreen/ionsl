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

        return std::nullopt;
    }

    std::optional<ConstantValue> ConstantEvaluator::evaluateBinaryExpr(const BinaryExpr &expr)
    {
        const auto left = evaluate(*expr.left);
        const auto right = evaluate(*expr.right);

        if(!left || !right) return std::nullopt;

        TypeId resultType = expr.resultType;


    }
}
