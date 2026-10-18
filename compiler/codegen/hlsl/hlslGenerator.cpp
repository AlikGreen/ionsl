#include "hlslGenerator.h"




namespace ionsl
{
    std::string HlslGenerator::generate()
    {
        for(const auto decl : m_module.declarations())
            genDecl(*decl);

        return m_writer.string();
    }

    void HlslGenerator::genForStmtStmt(Statement &stmt)
    {
        if(const auto expr = stmt.as<ExprStmt>())
            genExprStmt(*expr);
        if(const auto decl = stmt.as<DeclStmt>())
        {
            if (const auto var = decl->decl->as<ValueDecl>())
            {
                genVarDeclHeader(*var);
            }
        }
    }

    std::string HlslGenerator::opToString(const BinaryOp op)
    {
        switch (op)
        {
            case BinaryOp::Add:
                return "+";
            case BinaryOp::Subtract:
                return "-";
            case BinaryOp::Multiply:
                return "*";
            case BinaryOp::Divide:
                return "/";
            case BinaryOp::Modulo:
                return "%";
            case BinaryOp::Assign:
                return "=";
            case BinaryOp::Equal:
                return "==";
            case BinaryOp::Greater:
                return ">";
            case BinaryOp::Less:
                return "<";
            case BinaryOp::GreaterEqual:
                return ">=";
            case BinaryOp::LessEqual:
                return "<=";
            case BinaryOp::AddAssign:
                return "+=";
            case BinaryOp::SubAssign:
                return "-=";
            case BinaryOp::MulAssign:
                return "*=";
            case BinaryOp::DivAssign:
                return "/=";
            case BinaryOp::ModuloAssign:
                return "%=";
            case BinaryOp::BitwiseAnd:
                return "&";
            case BinaryOp::BitwiseOr:
                return "|";
            case BinaryOp::BitwiseXor:
                return "^";
            case BinaryOp::BitwiseAndAssign:
                return "&=";
            case BinaryOp::BitwiseOrAssign:
                return "|=";
            case BinaryOp::BitwiseXorAssign:
                return "^=";
            case BinaryOp::LogicalAnd:
                return "&&";
            case BinaryOp::LogicalOr:
                return "||";
            case BinaryOp::NotEqual:
                return "!=";
            case BinaryOp::ShiftLeft:
                return "<<";
            case BinaryOp::ShiftRight:
                return ">>";
            case BinaryOp::ShiftLeftAssign:
                return "<<=";
            case BinaryOp::ShiftRightAssign:
                return ">>=";
            default:
                return "unknown_operator";
        }
    }

    std::string HlslGenerator::opToString(UnaryOp op)
    {
        switch (op)
        {
            case UnaryOp::Negate:        return "-";
            case UnaryOp::LogicalNot:    return "!";
            case UnaryOp::BitwiseNot:    return "~";
            case UnaryOp::PreIncrement:  return "++";
            case UnaryOp::PreDecrement:  return "--";
            case UnaryOp::PostIncrement: return "++";
            case UnaryOp::PostDecrement: return "--";

            default:                     return "";
        }
    }

    std::string HlslGenerator::constantIntToString(ConstantInt op)
    {
        if (op.kind == IntKind::Unsigned)
            return std::to_string(op.value);

        return std::to_string(static_cast<int64_t>(op.value));
    }

    bool HlslGenerator::isPostfixOp(const UnaryOp op)
    {
        return (op == UnaryOp::PostIncrement || op == UnaryOp::PostDecrement);
    }

    bool HlslGenerator::isAssignment(const BinaryOp op)
    {
        switch (op)
        {
            case BinaryOp::Assign:
            case BinaryOp::AddAssign:
            case BinaryOp::SubAssign:
            case BinaryOp::DivAssign:
            case BinaryOp::MulAssign:
            case BinaryOp::ModuloAssign:
            case BinaryOp::BitwiseAndAssign:
            case BinaryOp::BitwiseOrAssign:
            case BinaryOp::BitwiseXorAssign:
            case BinaryOp::ShiftLeftAssign:
            case BinaryOp::ShiftRightAssign:
                return true;
            default:
                return false;
        }
    }

    std::string HlslGenerator::convertAttribute(const Attribute &attribute)
    {
        const std::string name = attribute.name.string(m_symbols);

        if(name == "sv_dispatchthreadid")     return "SV_DispatchThreadID";
        if(name == "sv_groupid")              return "SV_GroupID";
        if(name == "sv_groupthreadid")        return "SV_GroupThreadID";
        if(name == "sv_groupindex")           return "SV_GroupIndex";
        if(name == "sv_position")             return "SV_Position";
        if(name == "sv_vertexid")             return "SV_VertexID";
        if(name == "sv_instanceid")           return "SV_InstanceID";
        if(name == "sv_target")               return "SV_Target";
        if(name == "sv_depth")                return "SV_Depth";
        if(name == "sv_isfrontface")          return "SV_IsFrontFace";
        if(name == "sv_primitiveid")          return "SV_PrimitiveID";
        if(name == "sv_outputcontrolpointid") return "SV_OutputControlPointID";
        if(name == "sv_domainlocation")       return "SV_DomainLocation";
        if(name == "sv_tessfactor")           return "SV_TessFactor";
        if(name == "sv_insidetessfactor")     return "SV_InsideTessFactor";

        return "";
    }

    char HlslGenerator::componentIndexToChar(uint8_t index)
    {
        switch (index)
        {
            case 0: return 'x';
            case 1: return 'y';
            case 2: return 'z';
            case 3: return 'w';
            default: return '~';
        }
    }

    bool HlslGenerator::isResourceDecl(const ValueDecl &decl)
    {
        if (!decl.attributes) return false;

        if (decl.attributes->contains("constant", m_symbols) ||
            decl.attributes->contains("storage", m_symbols))
        {
            return true;
        }

        // TODO texture type

        return false;
    }
}
