#include "hlslGenerator.h"


namespace ionsl
{
    std::string HlslGenerator::generate()
    {
        for(const auto decl : m_module.declarations)
            genDecl(*decl);

        return m_writer.string();
    }

    void HlslGenerator::genDecl(Declaration &decl)
    {
        if(const auto funcDecl = decl.as<FunctionDecl>())
            genFunctionDecl(*funcDecl);
        if(const auto interfaceDecl = decl.as<InterfaceDecl>())
            genInterfaceDecl(*interfaceDecl);
        if(const auto structDecl = decl.as<StructDecl>())
            genStructDecl(*structDecl);
        if(const auto valDecl = decl.as<ValueDecl>())
            genVarDecl(*valDecl);
    }

    void HlslGenerator::genFunctionDecl(const FunctionDecl &decl)
    {
        genType(decl.resolvedReturnType);
        m_writer.space();
        m_writer.writeSymbol(decl.name);
        m_writer.write("(");

        m_writer.writeSeparated(decl.params, ", ", [this](ValueDecl* param)
        {
            genType(param->resolvedType);
            m_writer.space();
            m_writer.writeSymbol(param->name);

            if(param->initializer)
            {
                m_writer.write(" = ");
                genExpr(*param->initializer);
            }
        });

        m_writer.write(")");
        genBlockStmt(*decl.body);
    }

    void HlslGenerator::genStructDecl(StructDecl &decl)
    {
        for(auto interface : decl.resolvedInterfaces)
        {
            m_writer.write("// Implements interface '");
            genType(interface);
            m_writer.write("'");
            m_writer.newline();
        }

        m_writer.write("struct ");
        m_writer.writeSymbol(decl.name);
        m_writer.beginBlock();

        for(const auto field : decl.fields)
        {
            genType(field->resolvedType);
            m_writer.space();
            m_writer.writeSymbol(field->name);
            m_writer.write(";");

            if(field->initializer)
            {
                m_writer.write("// = ");
                genExpr(*field->initializer);
            }

            m_writer.newline();
        }

        m_writer.endBlock();

    }

    void HlslGenerator::genInterfaceDecl(const InterfaceDecl &decl)
    {
        m_writer.write("// interface ");
        m_writer.writeSymbol(decl.name);
        m_writer.newline();
    }

    void HlslGenerator::genVarDecl(ValueDecl &decl)
    {
        genType(decl.resolvedType);
        m_writer.space();
        m_writer.writeSymbol(decl.name);

        if(decl.initializer)
        {
            m_writer.write(" = ");
            genExpr(*decl.initializer);
        }

        m_writer.write(";");
        m_writer.newline();
    }

    void HlslGenerator::genStmt(Statement &stmt)
    {
        if(const auto block = stmt.as<BlockStmt>())
            genBlockStmt(*block);
        if(const auto expr = stmt.as<ExprStmt>())
            genExpr(*expr->expr);
        if(const auto decl = stmt.as<DeclStmt>())
            genDecl(*decl->decl);
        if(const auto ifStmt = stmt.as<IfStmt>())
            genIfStmt(*ifStmt);
        if(const auto forStmt = stmt.as<ForStmt>())
            genForStmt(*forStmt);
        if(const auto whileStmt = stmt.as<WhileStmt>())
            genWhileStmt(*whileStmt);
        if(const auto returnStmt = stmt.as<ReturnStmt>())
            genReturnStmt(*returnStmt);
        if(const auto breakStmt = stmt.as<BreakStmt>())
            genBreakStmt(*breakStmt);
        if(const auto continueStmt = stmt.as<ContinueStmt>())
            genContinueStmt(*continueStmt);
    }

    void HlslGenerator::genBlockStmt(BlockStmt &stmt)
    {
        m_writer.beginBlock();

        for(const auto s : stmt.statements)
        {
            genStmt(*s);
        }

        m_writer.endBlock();
    }

    void HlslGenerator::genIfStmt(IfStmt &stmt)
    {
        m_writer.write("if(");
        genExpr(*stmt.condition);
        genBlockStmt(*stmt.thenBranch);

        if(stmt.elseBranch)
        {
            m_writer.writeLine("else");
            genBlockStmt(*stmt.elseBranch);
        }
    }

    void HlslGenerator::genWhileStmt(WhileStmt &stmt)
    {
        m_writer.write("while(");
        genExpr(*stmt.condition);
        genBlockStmt(*stmt.body);
    }

    void HlslGenerator::genForStmt(ForStmt &stmt)
    {
        m_writer.write("for(");
        genStmt(*stmt.init);
        genExpr(*stmt.condition);
        genExpr(*stmt.increment);
        genBlockStmt(*stmt.body);
    }

    void HlslGenerator::genReturnStmt(ReturnStmt &stmt)
    {
        m_writer.write("return");
        if(stmt.expr)
        {
            m_writer.write(" ");
            genExpr(*stmt.expr);
        }
        m_writer.write(";");
        m_writer.newline();
    }

    void HlslGenerator::genBreakStmt(BreakStmt &stmt)
    {
        m_writer.writeLine("break;");
    }

    void HlslGenerator::genContinueStmt(ContinueStmt &stmt)
    {
        m_writer.writeLine("continue;");
    }

    void HlslGenerator::genType(TypeId id)
    {
        TypeInfo info = m_typeTable.getInfo(id);

        if(const auto primitive = info.as<PrimitiveType>())
            genPrimitiveType(primitive->kind);
        if(const auto vector = info.as<VectorType>())
            genVectorType(*vector);
        if(const auto matrix = info.as<MatrixType>())
            genMatrixType(*matrix);
        if(const auto structure = info.as<StructType>())
            genStructType(*structure);;
    }

    void HlslGenerator::genVectorType(VectorType &type)
    {
        m_writer.write("vector<");
        genType(type.scalarType);
        m_writer.write(", {}>", type.dimension);
    }

    void HlslGenerator::genMatrixType(MatrixType &type)
    {
        m_writer.write("matrix<");
        genType(type.scalarType);
        m_writer.write(", {}, {}>", type.rows, type.columns);
    }

    void HlslGenerator::genPrimitiveType(const PrimitiveKind kind)
    {
        std::unordered_map<PrimitiveKind, std::string> typeNames {
            { PrimitiveKind::Bool, "bool" },
            { PrimitiveKind::UInt32, "uint" },
            { PrimitiveKind::Int32, "int" },
            { PrimitiveKind::Float32, "float" },
            { PrimitiveKind::String, "string" },
            { PrimitiveKind::Void, "void" },
        };

        if(const auto it = typeNames.find(kind); it != typeNames.end())
            m_writer.write(it->second);

        m_writer.write("unknown_type");
    }

    void HlslGenerator::genStructType(const StructType type)
    {
        auto decl = m_declTable.get(type.declId);
        m_writer.writeSymbol(decl->as<StructDecl>()->name);
    }

    void HlslGenerator::genExpr(Expression &expr, bool addParens)
    {
        if(const auto binary = expr.as<BinaryExpr>())
            genBinaryExpr(*binary, addParens);
        if(const auto unary = expr.as<UnaryExpr>())
            genUnaryExpr(*unary);
        if(const auto call = expr.as<CallExpr>())
            genCallExpr(*call);
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
    }

    void HlslGenerator::genBinaryExpr(BinaryExpr &expr, bool addParens)
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
            m_writer.write("<");

            // TODO need resolved generic args

            m_writer.write(">");
        }

        m_writer.write("(");

        m_writer.writeSeparated(expr.args, ", ", [this](Expression* arg)
        {
            genExpr(*arg);
        });

        m_writer.write(")");
    }

    void HlslGenerator::genConversionExpr(const ConversionExpr& expr)
    {
        m_writer.write("(");
        genType(expr.targetType);
        m_writer.write(")");
        genExpr(*expr.operand, true);
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

            if constexpr(std::is_same_v<T, uint64_t>)
                m_writer.write("{}", arg);
            if constexpr(std::is_same_v<T, int64_t>)
                m_writer.write("{}", arg);
            if constexpr(std::is_same_v<T, double>)
                m_writer.write("{}", arg);
            if constexpr(std::is_same_v<T, std::string>)
                m_writer.write("{}", arg);
            if constexpr(std::is_same_v<T, bool>)
            {
                if(arg)
                    m_writer.write("true");
                else
                    m_writer.write("false");
            }

        }, expr.literal);
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
}
