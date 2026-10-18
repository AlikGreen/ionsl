#include "hlslGenerator.h"

namespace ionsl
{
void HlslGenerator::genExpr(Expression &expr, const bool addParens)
    {
        if(const auto binary = expr.as<BinaryExpr>())
            genBinaryExpr(*binary, addParens);
        if(const auto unary = expr.as<UnaryExpr>())
            genUnaryExpr(*unary);
        if(const auto call = expr.as<CallExpr>())
            genCallExpr(*call);
        if(const auto construct = expr.as<ConstructExpr>())
            genConstructExpr(*construct);
        if(const auto conversion = expr.as<ConversionExpr>())
            genConversionExpr(*conversion);
        if(const auto index = expr.as<IndexExpr>())
            genIndexExpr(*index);
        if(const auto literal = expr.as<LiteralExpr>())
            genLiteralExpr(*literal);
        if(const auto fieldAccess = expr.as<FieldAccessExpr>())
            genFieldAccessExpr(*fieldAccess);
        if(const auto identifier = expr.as<IdentifierExpr>())
            genIdentifierExpr(*identifier);
        if(const auto swizzle = expr.as<SwizzleExpr>())
            genSwizzleExpr(*swizzle);
    }

    void HlslGenerator::genBinaryExpr(const BinaryExpr &expr, const bool addParens)
    {
        if(addParens)
            m_writer.write("(");

        genExpr(*expr.left, !isAssignment(expr.op));
        m_writer.write(" {} ", opToString(expr.op));
        genExpr(*expr.right, !isAssignment(expr.op));

        if(addParens)
            m_writer.write(")");
    }

    void HlslGenerator::genUnaryExpr(UnaryExpr &expr)
    {
        if(!isPostfixOp(expr.op))
            m_writer.write(opToString(expr.op));

        genExpr(*expr.operand, true);

        if(isPostfixOp(expr.op))
            m_writer.write(opToString(expr.op));
    }

    void HlslGenerator::genCallExpr(const CallExpr &expr)
    {
        genExpr(*expr.callee, true);

        if(!expr.genericArgs.empty())
        {
            // m_writer.write("<");

            // TODO need to call the concrete version of the function

            // m_writer.write(">");
        }

        m_writer.write("(");

        m_writer.writeSeparated(expr.args, ", ", [this](Expression* arg)
        {
            genExpr(*arg);
        });

        m_writer.write(")");
    }

    void HlslGenerator::genConstructExpr(const ConstructExpr &expr)
    {
        genType(expr.type->resolvedType);

        m_writer.write("(");

        m_writer.writeSeparated(expr.args, ", ", [this](Expression* arg)
        {
            genExpr(*arg);
        });

        m_writer.write(")");
    }

    void HlslGenerator::genConversionExpr(const ConversionExpr& expr)
    {
        if (expr.kind == ConversionKind::Explicit)
        {
            m_writer.write("(");
            genType(expr.targetType);
            m_writer.write(")");
            m_writer.write("(");
        }
        genExpr(*expr.operand, true);
        if (expr.kind == ConversionKind::Explicit)
            m_writer.write(")");
    }

    void HlslGenerator::genIndexExpr(const IndexExpr &expr)
    {
        genExpr(*expr.array, true);
        m_writer.write("[");
        genExpr(*expr.index);
        m_writer.write("]");
    }

    void HlslGenerator::genLiteralExpr(LiteralExpr &expr)
    {
        std::visit([this](const auto& arg)
        {
            using T = std::decay_t<decltype(arg)>;

            if constexpr(std::is_same_v<T, ConstantInt>)
            {
                if (arg.kind == IntKind::Unsigned)
                    m_writer.write("{}u", arg.value);
                else
                    m_writer.write("{}", static_cast<int64_t>(arg.value));
            }
            if constexpr(std::is_same_v<T, ConstantFloat>)
            {
                if (arg.kind == FloatKind::Double)
                    m_writer.write("{}", arg.value);
                else
                    m_writer.write("{}", static_cast<float>(arg.value));
            }
            if constexpr(std::is_same_v<T, std::string>)
                m_writer.write("{}", arg);
            if constexpr(std::is_same_v<T, bool>)
            {
                if(arg)
                    m_writer.write("true");
                else
                    m_writer.write("false");
            }

        }, expr.value);
    }

    void HlslGenerator::genFieldAccessExpr(FieldAccessExpr &expr)
    {
        genExpr(*expr.object, true);
        m_writer.write(".");
        m_writer.writeSymbol(expr.memberName);
    }

    void HlslGenerator::genIdentifierExpr(IdentifierExpr &expr)
    {
        m_writer.writeSeparated(expr.name.parts, "::", [this](SymbolId id)
        {
            m_writer.writeSymbol(id);
        });
    }

    void HlslGenerator::genSwizzleExpr(SwizzleExpr &expr)
    {
        genExpr(*expr.object, true);
        m_writer.write(".");

        for (const uint8_t index : expr.indices)
            m_writer.write(componentIndexToChar(index));
    }
}