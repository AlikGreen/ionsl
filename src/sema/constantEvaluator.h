#pragma once
#include "../ast/expressions.h"

namespace ionsl
{
struct ConstantValue
{
    TypeId   type;
    LiteralValue value;
};

class ConstantEvaluator
{
public:
    static std::optional<ConstantValue> evaluate(Expression &expr, const DeclTable &declTable);
private:
    const DeclTable& m_declTable;

    ConstantEvaluator(const DeclTable& declTable);
    std::optional<ConstantValue> evaluate(Expression& expr);

    std::optional<ConstantValue> evaluateBinaryExpr(const BinaryExpr& expr);
    std::optional<ConstantValue> evaluateUnaryExpr(UnaryExpr& expr);
};
}
