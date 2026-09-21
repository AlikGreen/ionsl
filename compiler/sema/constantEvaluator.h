#pragma once
#include "../ast/expressions.h"
#include "../ast/typeSystem.h"

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
    ConstantEvaluator(const DeclTable& declTable, const TypeSystem& typeSystem);
    std::optional<ConstantValue> evaluate(Expression& expr);
private:
    const DeclTable& m_declTable;
    const TypeSystem& m_typeSystem;


    std::optional<ConstantValue> evaluateBinaryExpr(const BinaryExpr& expr);
    std::optional<ConstantValue> evaluateUnaryExpr(UnaryExpr& expr);
    std::optional<ConstantValue> evaluateLiteralExpr(const LiteralExpr& expr);
};
}
