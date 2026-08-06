#include "codeGen.h"

namespace ionsl
{
    // Temporary solution as things like texcoord can go up to 32
    static const std::unordered_map<std::string_view, std::string> kBuiltinAttribMap
    {
            { "sv_position", "SV_Position" }, { "position", "POSITION" },
            { "normal",  "NORMAL" }, { "shader",  "shader" },
            { "sv_start_instance_loc",  "SV_StartInstanceLocation" },

        };

    CodeGen::CodeGen(const Module &module)
        : m_declNodes(module.decls)
    {
    }

    std::string CodeGen::generate()
    {
        m_source.clear();
        m_indent = 0;

        for(const auto& decl : m_declNodes)
        {
            genForwardDecl(decl);
            newLine();
        }

        newLine();
        newLine();

        for(const auto& decl : m_declNodes)
        {
            genDecl(decl);
            newLine();
        }

        return m_source.str();
    }

    std::string CodeGen::generate(const Module &module)
    {
        CodeGen codeGen{module};
        return codeGen.generate();
    }

    void CodeGen::genTrivia(const std::vector<Trivia>& trivias)
    {
        for(const auto& trivia : trivias)
        {
            if(trivia.isBlockComment)
                m_source << "/*";
            else
                m_source << "//";

            m_source << trivia.text;

            if(trivia.isBlockComment)
                m_source << "*/";
            else
                newLine();
        }
    }

    void CodeGen::genDecl(const DeclNode& decl)
    {
        genTrivia(decl.trivia);

        std::visit(
            [this]<typename T0>(T0&& d)
            {
                using T = std::decay_t<T0>;

                if constexpr (std::is_same_v<T, FunctionDecl>)
                    genFunctionDecl(d);
                else if constexpr (std::is_same_v<T, StructDecl>)
                    genStructDecl(d);
                else if constexpr (std::is_same_v<T, VarDecl>)
                    genVarDecl(d);
            },
            decl.decl
        );
    }

    void CodeGen::genForwardDecl(const DeclNode &decl)
    {
        std::visit(
            [this]<typename T0>(T0&& d)
            {
                using T = std::decay_t<T0>;

                if constexpr (std::is_same_v<T, FunctionDecl>)
                {
                    genFunctionSignature(d);
                    m_source << ";";
                }
                else if constexpr (std::is_same_v<T, StructDecl>)
                {
                    genStructSignature(d);
                    m_source << ";";
                }
            },
            decl.decl
        );
    }

    void CodeGen::genFunctionSignature(const FunctionDecl &decl)
    {
        genType(decl.returnType);

        m_source << ' ' << decl.name << '(';

        for(size_t i = 0; i < decl.params.size(); i++)
        {
            genType(decl.params[i].type);
            genTrivia(decl.params[i].trivia);
            m_source << " " << decl.params[i].name;

            genAttribute(decl.params[i].attributes);

            if(i != decl.params.size() - 1)
                m_source << ", ";
        }

        m_source << ")";
    }

    void CodeGen::genStructSignature(const StructDecl &decl)
    {
        m_source << "struct " << decl.name;
    }

    void CodeGen::genFunctionDecl(const FunctionDecl &decl)
    {
        genFunctionSignature(decl);
        newLine();

        if(decl.body.has_value())
            genBlockStmt(decl.body.value());
    }

    void CodeGen::genVarDecl(const VarDecl &decl)
    {
        if(decl.modifier != VarModifier::Mutable && !std::holds_alternative<ResourceBindingType>(decl.type.kind))
            m_source << "const ";

        genType(decl.type);
        m_source << " ";
        m_source << decl.name;
        if(const ArrayType* arrayType =  std::get_if<ArrayType>(&decl.type.kind))
        {
            m_source << "[";
            if(arrayType->size.has_value())
                genExpr(*arrayType->size.value(), true);

            m_source << "]";
        }

        if(decl.initializer)
        {
            m_source << " = ";
            genExpr(*decl.initializer);
        }

        m_source << ";";
    }

    void CodeGen::genStructDecl(const StructDecl &decl)
    {
        newLine();
        genStructSignature(decl);
        newLine();
        m_source << "{";

        m_indent++;
        for(const auto & field : decl.fields)
        {
            newLine();
            genTrivia(field.trivia);
            genType(field.type);
            m_source << " " << field.name;
            if(const ArrayType* arrayType =  std::get_if<ArrayType>(&field.type.kind))
            {
                m_source << "[";
                if(arrayType->size.has_value())
                    genExpr(*arrayType->size.value(), true);

                m_source << "]";
            }

            genAttribute(field.attributes);

            // TODO implement initializers

            m_source << ";";
        }

        for(const auto & method : decl.methods)
        {
            newLine();
            genTrivia(method.trivia);
            genFunctionDecl(method.decl);
        }


        m_indent--;
        newLine();

        m_source << "};";
        newLine();
    }

    void CodeGen::genExpr(const ExprNode &expr, bool topLevel)
    {
        genTrivia(expr.trivia);

        std::visit(
            [this, topLevel]<typename T0>(T0&& e)
            {
                using T = std::decay_t<T0>;

                if constexpr (std::is_same_v<T, BinaryExpr>)
                    genBinaryExpr(e, topLevel);
                else if constexpr (std::is_same_v<T, IdentifierExpr>)
                    genIdentifierExpr(e);
                else if constexpr (std::is_same_v<T, IndexExpr>)
                    genIndexExpr(e);
                else if constexpr (std::is_same_v<T, UnaryExpr>)
                    genUnaryExpr(e);
                else if constexpr (std::is_same_v<T, FieldAccessExpr>)
                    genFieldAccessExpr(e);
                else if constexpr (std::is_same_v<T, FunctionCallExpr>)
                    genFunctionCallExpr(e);
                else if constexpr (std::is_same_v<T, TypeExpr>)
                    genTypeExpr(e);
                else if constexpr (std::is_same_v<T, Literal>)
                    genLiteralExpr(e);
            },
            expr.expr
        );
    }

    void CodeGen::genBinaryExpr(const BinaryExpr &expr, bool topLevel)
    {
        if(!topLevel)
            m_source << "(";

        genExpr(*expr.left);
        m_source << " " << opToString(expr.op) << " ";
        genExpr(*expr.right, expr.op == BinaryOp::Assign);

        if(!topLevel)
            m_source << ")";
    }

    void CodeGen::genIdentifierExpr(const IdentifierExpr &expr)
    {
        m_source << expr.name;

        if(!expr.genericArgs.empty())
        {
            m_source << "<";
            for(size_t i = 0; i < expr.genericArgs.size(); i++)
            {
                genType(expr.genericArgs.at(i));
                if(i != expr.genericArgs.size() - 1)
                    m_source << ", ";
            }
            m_source << ">";
        }
    }

    void CodeGen::genIndexExpr(const IndexExpr &expr)
    {
        genExpr(*expr.array);
        m_source << "[";
        genExpr(*expr.index, true);
        m_source << "]";
    }

    void CodeGen::genUnaryExpr(const UnaryExpr &expr)
    {
        if(!isPostfixOp(expr.op))
            m_source << opToString(expr.op);

        genExpr(*expr.operand);

        if(isPostfixOp(expr.op))
            m_source << opToString(expr.op);
    }

    void CodeGen::genFieldAccessExpr(const FieldAccessExpr &expr)
    {
        genExpr(*expr.object);
        m_source << "." << expr.memberName;
    }

    void CodeGen::genFunctionCallExpr(const FunctionCallExpr &expr)
    {
        genExpr(*expr.callee);
        m_source << "(";

        for(size_t i = 0; i < expr.args.size(); i++)
        {
            genExpr(expr.args[i], true);

            if(i != expr.args.size() - 1)
             m_source << ", ";
        }
        m_source << ")";
    }

    void CodeGen::genTypeExpr(const TypeExpr &expr)
    {
        genType(expr.type);
    }

    void CodeGen::genLiteralExpr(const Literal &expr)
    {
        std::visit(
            [this]<typename T0>(T0&& literal)
            {
                using T = std::decay_t<T0>;

                if constexpr (std::is_same_v<T, std::string>)
                    m_source << '"' << literal << '"';
                else if constexpr (std::is_same_v<T, int64_t>)
                    m_source << literal;
                else if constexpr (std::is_same_v<T, double>)
                    genFloatLiteral(literal);
                else if constexpr (std::is_same_v<T, bool>)
                    m_source << literal;
            },
            expr
        );
    }

    void CodeGen::genFloatLiteral(double value)
    {
        std::ostringstream oss;
        oss << value;
        std::string s = oss.str();

        if (s.find_first_of(".eEnN") == std::string::npos)
            s += ".0";

        m_source << s;
    }

    void CodeGen::genStmt(const StmtNode &stmt)
    {
        genTrivia(stmt.trivia);

        std::visit(
            [this]<typename T0>(T0&& s)
            {
                using T = std::decay_t<T0>;

                if constexpr (std::is_same_v<T, DeclStmt>)
                    genDecl(s.decl);
                else if constexpr (std::is_same_v<T, ExprStmt>)
                    genExprStmt(s);
                else if constexpr (std::is_same_v<T, BlockStmt>)
                    genBlockStmt(s);
                else if constexpr (std::is_same_v<T, ForStmt>)
                    genForStmt(s);
                else if constexpr (std::is_same_v<T, WhileStmt>)
                    genWhileStmt(s);
                else if constexpr (std::is_same_v<T, IfStmt>)
                    genIfStmt(s);
                else if constexpr (std::is_same_v<T, ReturnStmt>)
                    genReturnStmt(s);
                else if constexpr (std::is_same_v<T, BreakStmt>)
                    genBreakStmt(s);
                else if constexpr (std::is_same_v<T, ContinueStmt>)
                    genContinueStmt(s);
            },
            stmt.stmt
        );
    }

    void CodeGen::genBlockStmt(const BlockStmt &block)
    {
        m_source << "{";
        m_indent++;
        newLine();

        for(size_t i = 0; i < block.statements.size(); i++)
        {
            genStmt(block.statements[i]);

            if(i != block.statements.size() - 1)
                newLine();
        }

        m_indent--;
        newLine();
        m_source << "}";
    }

    void CodeGen::genForStmt(const ForStmt &stmt)
    {
        m_source << "for(";
        genStmt(*stmt.init);
        m_source << " ";
        genExpr(stmt.condition, true);
        m_source << "; ";
        genExpr(stmt.increment, true);
        m_source << ")";
        newLine();
        genBlockStmt(stmt.body);
    }

    void CodeGen::genWhileStmt(const WhileStmt &stmt)
    {
        m_source << "while(";
        genExpr(stmt.condition, true);
        m_source << ")";
        newLine();
        genBlockStmt(stmt.body);
    }

    void CodeGen::genIfStmt(const IfStmt &stmt)
    {
        m_source << "if(";
        genExpr(stmt.condition, true);
        m_source << ")";
        newLine();
        genBlockStmt(stmt.thenBranch);
        if(stmt.elseBranch.has_value())
        {
            m_source << "else";
            newLine();
            genBlockStmt(stmt.elseBranch.value());
        }
    }

    void CodeGen::genReturnStmt(const ReturnStmt &stmt)
    {
        m_source << "return ";

        if(stmt.expr.has_value())
        {
            genExpr(stmt.expr.value());
        }

        m_source << ";";
    }

    void CodeGen::genBreakStmt(const BreakStmt &stmt)
    {
        m_source << "break;";
    }

    void CodeGen::genContinueStmt(const ContinueStmt &stmt)
    {
        m_source << "continue;";
    }

    void CodeGen::genExprStmt(const ExprStmt &stmt)
    {
        genExpr(stmt.expr, true);
        m_source << ";";
    }


    void CodeGen::genType(const Type &t)
    {
        std::visit(
            [this]<typename T0>(T0&& type)
            {
                using T = std::decay_t<T0>;

                if constexpr (std::is_same_v<T, PrimitiveType>)
                    genPrimitiveType(type.kind);
                else if constexpr (std::is_same_v<T, ArrayType>)
                    genType(*type.elementType); // [?] goes after
                else if constexpr (std::is_same_v<T, CustomType>)
                    genCustomType(type);
                else if constexpr (std::is_same_v<T, StructType>)
                    genStructType(type);
                else if constexpr (std::is_same_v<T, MatrixType>)
                    genMatrixType(type);
                else if constexpr (std::is_same_v<T, VectorType>)
                    genVectorType(type);
                else if constexpr (std::is_same_v<T, ResourceBindingType>)
                    genResourceType(type);
            },
            t.kind
        );
    }

    void CodeGen::genPrimitiveType(const PrimitiveKind &type)
    {
        switch (type)
        {
            case PrimitiveKind::Bool:
                m_source << "bool";
                break;
            case PrimitiveKind::Float16:
                m_source << "half";
                break;
            case PrimitiveKind::Float32:
                m_source << "float";
                break;
            case PrimitiveKind::Float64:
                m_source << "double";
                break;
            case PrimitiveKind::Int8:
                m_source << "i8"; // Not real
                break;
            case PrimitiveKind::Int16:
                m_source << "int16_t";
                break;
            case PrimitiveKind::Int32:
                m_source << "int";
                break;
            case PrimitiveKind::Int64:
                m_source << "int64_t";
                break;
            case PrimitiveKind::UInt8:
                m_source << "u8"; // Not real
                break;
            case PrimitiveKind::UInt16:
                m_source << "uint16_t";
                break;
            case PrimitiveKind::UInt32:
                m_source << "uint";
                break;
            case PrimitiveKind::UInt64:
                m_source << "uint16_t";
                break;
            case PrimitiveKind::String:
                m_source << "string";
                break;
            case PrimitiveKind::Void:
                m_source << "void";
                break;
            default:
                break;
        }
    }

    void CodeGen::genCustomType(const CustomType &type)
    {
        for(size_t i = 0; i < type.name.segments.size(); i++)
        {
            m_source << type.name.segments[i];

            if(i != type.name.segments.size() - 1)
                m_source << "::";
        }

        if(!type.genericArgs.empty())
        {
            m_source << '<';

            for(size_t i = 0; i < type.genericArgs.size(); i++)
            {
                genType(*type.genericArgs.at(i));

                if(i != type.genericArgs.size() - 1)
                    m_source << ", ";
            }

            m_source << '>';
        }
    }

    void CodeGen::genStructType(const StructType &type)
    {
        m_source << type.decl->name;
    }

    void CodeGen::genMatrixType(const MatrixType &type)
    {
        m_source << "matrix<";
        genPrimitiveType(type.scalarType);
        m_source << ", " << static_cast<uint32_t>(type.rows) << ", " << static_cast<uint32_t>(type.columns) << ">";
    }

    void CodeGen::genVectorType(const VectorType &type)
    {
        m_source << "vector<";
        genPrimitiveType(type.scalarType);
        m_source << ", " << static_cast<uint32_t>(type.dimension) << ">";
    }

    void CodeGen::genResourceType(const ResourceBindingType &type)
    {
        m_source << resourceKindToString(type.kind);

        if(type.elementType)
        {
            m_source << '<';

            genType(*type.elementType);

            m_source << '>';
        }
    }

    void CodeGen::genAttribute(const std::vector<Attribute> &attributes)
    {
        for(const auto& attrib : attributes)
        {
            auto name = attrib.name.name();
            const auto it = kBuiltinAttribMap.find(name);
            if(it == kBuiltinAttribMap.end()) continue;

            m_source << " : " << it->second;
            break;
        }
    }

    void CodeGen::newLine()
    {
        m_source << '\n';
        for(size_t i = 0; i < m_indent; i++)
        {
            m_source << "    ";
        }
    }

    std::string CodeGen::opToString(const BinaryOp op)
    {
        switch (op)
        {
            case BinaryOp::Add:          return "+";
            case BinaryOp::Subtract:     return "-";
            case BinaryOp::Multiply:     return "*";
            case BinaryOp::Divide:       return "/";
            case BinaryOp::Modulo:       return "%";

            case BinaryOp::Equal:        return "==";
            case BinaryOp::NotEqual:     return "!=";
            case BinaryOp::Less:         return "<";
            case BinaryOp::LessEqual:    return "<=";
            case BinaryOp::Greater:      return ">";
            case BinaryOp::GreaterEqual: return ">=";

            case BinaryOp::LogicalAnd:   return "&&";
            case BinaryOp::LogicalOr:    return "||";

            case BinaryOp::BitwiseAnd:   return "&";
            case BinaryOp::BitwiseOr:    return "|";
            case BinaryOp::BitwiseXor:   return "^";
            case BinaryOp::ShiftLeft:    return "<<";
            case BinaryOp::ShiftRight:   return ">>";

            case BinaryOp::Assign:       return "=";
            case BinaryOp::AddAssign:    return "+=";
            case BinaryOp::SubAssign:    return "-=";
            case BinaryOp::MulAssign:    return "*=";
            case BinaryOp::DivAssign:    return "/=";

            case BinaryOp::ModuloAssign:  return "%=";
            case BinaryOp::BitwiseAndAssign:  return "&=";
            case BinaryOp::BitwiseOrAssign:   return "|=";
            case BinaryOp::BitwiseXorAssign:  return "^=";
            case BinaryOp::ShiftLeftAssign:   return "<<=";
            case BinaryOp::ShiftRightAssign:  return ">>=";

            default:                     return "";
        }
    }

    std::string CodeGen::opToString(const UnaryOp op)
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

    std::string CodeGen::resourceKindToString(const ResourceBindingKind kind)
    {
        switch (kind)
        {
            case ResourceBindingKind::ConstantBuffer:      return "ConstantBuffer";
            case ResourceBindingKind::PushConstant:        return "ConstantBuffer";
            case ResourceBindingKind::SamplerState:        return "SamplerState";
            case ResourceBindingKind::StructuredBuffer:    return "StructuredBuffer";
            case ResourceBindingKind::Texture2D:           return "Texture2D";
            case ResourceBindingKind::RWStructuredBuffer:  return "RWStructuredBuffer";
            case ResourceBindingKind::RWTexture2D:         return "RWTexture2D";
            case ResourceBindingKind::RWByteAddressBuffer: return "RWByteAddressBuffer";

            default:                     return "";
        }
    }

    bool CodeGen::isPostfixOp(const UnaryOp op)
    {
        return (op == UnaryOp::PostIncrement || op == UnaryOp::PostDecrement);
    }
}
