#include "parser.h"

#include <charconv>
#include <format>
#include <stdexcept>

namespace ionsl
{
    static std::unordered_map<BinaryOp, std::pair<uint8_t, uint8_t>> kInfixBindingPower
    {
        { BinaryOp::Assign,        { 2, 1 } },
        { BinaryOp::AddAssign,     { 2, 1 } },
        { BinaryOp::SubAssign,     { 2, 1 } },
        { BinaryOp::MulAssign,     { 2, 1 } },
        { BinaryOp::DivAssign,     { 2, 1 } },

        { BinaryOp::ModuloAssign,     { 2, 1 } },
        { BinaryOp::BitwiseAndAssign,  { 2, 1 } },
        { BinaryOp::BitwiseOrAssign,   { 2, 1 } },
        { BinaryOp::BitwiseXorAssign,  { 2, 1 } },
        { BinaryOp::ShiftLeftAssign,     { 2, 1 } },
        { BinaryOp::ShiftRightAssign,     { 2, 1 } },

        { BinaryOp::LogicalOr,     { 3, 4 } },
        { BinaryOp::LogicalAnd,    { 5, 6 } },
        { BinaryOp::BitwiseOr,     { 7, 8 } },
        { BinaryOp::BitwiseXor,    { 9, 10 } },
        { BinaryOp::BitwiseAnd,    { 11, 12 } },

        { BinaryOp::Equal,         { 13, 14 } },
        { BinaryOp::NotEqual,      { 13, 14 } },

        { BinaryOp::Less,          { 15, 16 } },
        { BinaryOp::LessEqual,     { 15, 16 } },
        { BinaryOp::Greater,       { 15, 16 } },
        { BinaryOp::GreaterEqual,  { 15, 16 } },

        { BinaryOp::ShiftLeft,     { 17, 18 } },
        { BinaryOp::ShiftRight,    { 17, 18 } },

        { BinaryOp::Add,           { 19, 20 } },
        { BinaryOp::Subtract,      { 19, 20 } },

        { BinaryOp::Multiply,      { 21, 22 } },
        { BinaryOp::Divide,        { 21, 22 } },
        { BinaryOp::Modulo,        { 21, 22 } }
    };

    constexpr uint8_t kPrefixUnaryBindingPower = 23;

    Parser::Parser(const std::span<Token> tokens)
        : m_tokens(tokens), m_pos(0)
    {
    }

    Module Parser::parse()
    {
        consumeTriviaAndAttribs();

        while(!done())
        {
            m_decls.emplace_back(parseDecl());
        }

        Module m{
            "",
            std::move(m_decls),
            m_diagnostics.diagnostics(),
        };

        return m;
    }

    Module Parser::parse(const std::span<Token> tokens)
    {
        Parser parser{ tokens };
        return parser.parse();
    }


    DeclNode Parser::parseDecl()
    {
        auto leadingTrivia = takePendingTrivia();

        const SourceSpan startSpan = peek().span;

        Decl decl{};

        if(check(TokenKind::KwStruct))
            decl = parseStructDecl();
        else if(check(TokenKind::KwFunction))
            decl = parseFunctionDecl();
        else if(check(TokenKind::KwVar))
            decl = parseVarDecl();
        else if(check(TokenKind::KwInterface))
            decl = parseInterfaceDecl();
        else if (check(TokenKind::KwType))
            decl = parseTypeDefDecl();
        else // TODO in future add an ErrorDecl for recovery for LSP
            throw std::runtime_error("Expected a declaration (like function or var) at the top level.");

        return DeclNode{
            .trivia = std::move(leadingTrivia),
            .decl = std::move(decl),
            .span = SourceSpan::between(startSpan, previous().span)
        };
    }

    StructDecl Parser::parseStructDecl()
    {
        consume(TokenKind::KwStruct);
        StructDecl struc{};
        struc.name = advance().text;
        struc.attributes = takePendingAttributes();

        // TODO make able to implement multiple interfaces
        if(match(TokenKind::Colon))
        {
            struc.interfaces.push_back(parseType());
        }

        consume(TokenKind::LBrace);

        std::vector<StructField> fields{};

        while(!match(TokenKind::RBrace))
        {
            if(check(TokenKind::KwFunction))
            {
                Method method{};
                method.trivia = takePendingTrivia();
                SourceSpan startSpan = peek().span;
                method.decl = parseFunctionDecl();
                method.span = SourceSpan::between(startSpan, previous().span);
                struc.methods.push_back(std::move(method));
                continue;
            }

            const SourceSpan startSpan = peek().span;
            StructField field{};
            field.attributes = takePendingAttributes();
            field.name = advance().text; // Should be an identifier
            field.trivia = takePendingTrivia();

            consume(TokenKind::Colon);
            field.type = parseType();

            if(match(TokenKind::Equal))
                field.initializer = Box<ExprNode>::make(parseExpr());

            field.span = SourceSpan::between(startSpan, previous().span);

            consume(TokenKind::Comma);

            struc.fields.push_back(std::move(field));
        }

        return struc;
    }

    FunctionDecl Parser::parseFunctionDecl()
    {
        consume(TokenKind::KwFunction);
        FunctionDecl func{};
        func.attributes = takePendingAttributes();
        func.name = advance().text;

        // TODO make able to parse multiple types
        if(match(TokenKind::LAngle))
        {
            GenericArg arg{};
            arg.type = parseType();

            if(match(TokenKind::Colon))
            {
                arg.interfaceType = parseType();
            }

            func.genericArgs.push_back(std::move(arg));
            consume(TokenKind::RAngle);
        }

        match(TokenKind::LParen);

        if(!check(TokenKind::RParen))
        do
        {
            ParamDecl param{};
            param.attributes = takePendingAttributes();
            param.trivia = takePendingTrivia();
            param.name = advance().text;

            consume(TokenKind::Colon);

            param.type = parseType();

            if(match(TokenKind::Equal))
                param.defaultValue = Box<ExprNode>::make(parseExpr());

            // TODO add modifiers somehow (or just use attributes)

            func.params.push_back(std::move(param));
        }while (match(TokenKind::Comma));

        consume(TokenKind::RParen);

        if(match(TokenKind::Arrow))
            func.returnType = parseType();
        else
            func.returnType = Type{ .kind = PrimitiveType{ PrimitiveKind::Void } };

        if(check(TokenKind::LBrace))
            func.body = parseBlockStmt();
        else
            consume(TokenKind::Semicolon);

        return func;
    }

    VarDecl Parser::parseVarDecl()
    {
        consume(TokenKind::KwVar);

        VarDecl var{};

        if(match(TokenKind::KwMut))
            var.modifier = VarModifier::Mutable;


        var.name = advance().text;
        var.attributes = takePendingAttributes();

        consume(TokenKind::Colon);

        var.type = parseType();

        if(match(TokenKind::Equal))
            var.initializer = Box<ExprNode>::make(parseExpr());

        consume(TokenKind::Semicolon);

        return var;
    }

    InterfaceDecl Parser::parseInterfaceDecl()
    {
        consume(TokenKind::KwInterface);

        InterfaceDecl interface{};
        interface.name = advance().text;
        interface.attributes = takePendingAttributes();

        consume(TokenKind::LBrace);

        std::vector<StructField> fields{};

        while(!match(TokenKind::RBrace))
        {
            Method method{};
            method.trivia = takePendingTrivia();
            SourceSpan startSpan = peek().span;
            method.decl = parseFunctionDecl();
            method.span = SourceSpan::between(startSpan, previous().span);
            interface.methods.push_back(std::move(method));
        }

        return interface;
    }

    TypeDefDecl Parser::parseTypeDefDecl()
    {
        TypeDefDecl typeDef{};
        consume(TokenKind::KwType);
        typeDef.name = std::string(advance().text);
        consume(TokenKind::Equal);
        typeDef.type = parseType();
        consume(TokenKind::Semicolon);
        return typeDef;
    }

    ExprNode Parser::parseExpr(const uint32_t minBP)
    {
        auto lhs = parsePrefixExpr();

        while(true)
        {
            auto bp = getBindingPower(peek().kind);
            if(!bp.has_value()) break;

            auto [lBP, rBP] = bp.value();

            if(lBP < minBP)
                break;

            Token opToken = advance();
            lhs = parseInfixExpr(std::move(lhs), opToken);
        }
        return lhs;
    }

    ExprNode Parser::parseInfixExpr(ExprNode left, const Token& opToken)
    {
        switch (opToken.kind)
        {
            case TokenKind::Dot:
            {
                const auto fieldName = std::string(advance().text);

                FieldAccessExpr fieldAccess;

                fieldAccess.object = Box<ExprNode>::make(std::move(left));
                fieldAccess.memberName = fieldName;

                return ExprNode{
                    .span = SourceSpan::between(left.span, previous().span),
                    .expr = std::move(fieldAccess)
                };
            }

            case TokenKind::LParen:
            {
                std::vector<ExprNode> args;
                while (!match(TokenKind::RParen))
                {
                    args.push_back(parseExpr());
                    match(TokenKind::Comma);
                }

                FunctionCallExpr call{};
                call.callee = Box<ExprNode>::make(std::move(left));
                call.args = std::move(args);

                return ExprNode{
                    .span = SourceSpan::between(left.span, previous().span),
                    .expr = std::move(call)
                };
            }

            case TokenKind::LBracket:
            {
                ExprNode index = parseExpr();
                match(TokenKind::RBracket);

                IndexExpr indexExpr{};
                indexExpr.array = Box<ExprNode>::make(std::move(left));
                indexExpr.index = Box<ExprNode>::make(std::move(index));

                return ExprNode{
                    .span = SourceSpan::between(left.span, previous().span),
                    .expr = std::move(indexExpr)
                };
            }

            default:
            {
                if (auto op = tokenToBinaryOp(opToken.kind))
                {
                    auto [lBP, rBP] = kInfixBindingPower[op.value()];
                    ExprNode right = parseExpr(rBP);

                    BinaryExpr binary;
                    binary.op = op.value();
                    binary.left = Box<ExprNode>::make(std::move(left));
                    binary.right = Box<ExprNode>::make(std::move(right));

                    return ExprNode{
                        .span = SourceSpan::between(left.span, previous().span),
                        .expr = std::move(binary)
                    };
                }
                return left;
            }
        }
    }

    ExprNode Parser::parsePrefixExpr()
    {
        Token token = peek();

        switch (token.kind)
        {
            case TokenKind::NumberLiteral:
            case TokenKind::StringLiteral:
            case TokenKind::KwTrue:
            case TokenKind::KwFalse:
                return makeLiteralExpr(advance());

            case TokenKind::Identifier:
                return parseIdentifierExpr();

            case TokenKind::Minus:
            case TokenKind::Exclamation:
            case TokenKind::MinusMinus:
            case TokenKind::PlusPlus:
            {
                advance();
                ExprNode operand = parseExpr(kPrefixUnaryBindingPower);
                return makeUnaryPrefixExpr(token, std::move(operand));
            }

            case TokenKind::LParen:
            {
                advance();
                ExprNode expr = parseExpr();
                match(TokenKind::RParen);
                return expr;
            }

            default:
            {
                advance();
                m_diagnostics.add(std::format("expected an expression found {}", tokenKindDisplayName(previous().kind)), previous().span, Severity::Error);
                return createErrorExpr();
            }
        }
    }

    ExprNode Parser::parseIdentifierExpr()
    {
        SourceSpan startSpan = peek().span;
        IdentifierExpr expr{};
        expr.name = advance().text;


        if (peek().kind == TokenKind::LAngle && isLookaheadGenericArgs())
        {
            advance();

            while (!match(TokenKind::RAngle))
            {
                expr.genericArgs.push_back(parseType());
                match(TokenKind::Comma);
            }
        }

        return ExprNode {
            .trivia = takePendingTrivia(),
            .span = SourceSpan::between(startSpan, previous().span),
            .expr = std::move(expr)
        };
    }

    ExprNode Parser::createErrorExpr()
    {
        return ExprNode {
            .trivia = takePendingTrivia(),
            .span = previous().span,
            .expr = ErrorExpr{}
        };
    }

    ExprNode Parser::makeLiteralExpr(const Token &token)
    {
        ExprNode expr{};
        expr.expr = parseLiteral(token);
        expr.span = token.span;
        expr.trivia = takePendingTrivia();
        return expr;
    }

    ExprNode Parser::makeUnaryPrefixExpr(const Token &operatorToken, ExprNode operand)
    {
        UnaryOp op;

        switch (operatorToken.kind)
        {
            case TokenKind::Exclamation:
                op = UnaryOp::LogicalNot;
                break;
            case TokenKind::Minus:
                op = UnaryOp::Negate;
                break;
            case TokenKind::MinusMinus:
                op = UnaryOp::PreDecrement;
                break;
            case TokenKind::PlusPlus:
                op = UnaryOp::PreIncrement;
                break;
            default:
                throw std::runtime_error("TODO fix this");
        }

        UnaryExpr expr{};
        expr.op = op;
        expr.operand = Box<ExprNode>::make(std::move(operand));

        return ExprNode {
            .trivia =  takePendingTrivia(),
            .span = SourceSpan::between(operatorToken.span, previous().span),
            .expr = std::move(expr)
        };
    }


    StmtNode Parser::parseStmt()
    {
        const SourceSpan startSpan = peek().span;
        const auto trivia = takePendingTrivia();

        Stmt stmt;
        if(check(TokenKind::KwIf))
            stmt = parseIfStmt();
        else if(check(TokenKind::KwReturn))
            stmt = parseReturnStmt();
        else if(check(TokenKind::KwWhile))
            stmt = parseWhileStmt();
        else if(check(TokenKind::KwFor))
            stmt = parseForStmt();
        else if(check(TokenKind::LBrace))
            stmt = parseBlockStmt();
        else if(check(TokenKind::KwVar))
            stmt = parseDeclStmt();
        else if(match(TokenKind::KwBreak))
        {
            consume(TokenKind::Semicolon);
            stmt = BreakStmt{};
        }
        else if(match(TokenKind::KwContinue))
        {
            consume(TokenKind::Semicolon);
            stmt = ContinueStmt{};
        }
        else
            stmt = parseExprStmt();

        StmtNode node{};
        node.stmt = std::move(stmt);
        node.span = SourceSpan::between(startSpan, previous().span);
        node.trivia = trivia;

        return node;
    }

    BlockStmt Parser::parseBlockStmt()
    {
        match(TokenKind::LBrace);

        BlockStmt block{};

        while (!match(TokenKind::RBrace))
        {
            block.statements.push_back(parseStmt());
        }

        return block;
    }

    IfStmt Parser::parseIfStmt()
    {
        match(TokenKind::KwIf);

        IfStmt stmt{};
        stmt.condition = parseExpr();
        stmt.thenBranch = parseBlockStmt();

        if(match(TokenKind::KwElse))
        {
            stmt.elseBranch = parseBlockStmt();
        }

        return stmt;
    }

    WhileStmt Parser::parseWhileStmt()
    {
        match(TokenKind::KwWhile);

        WhileStmt stmt{};
        stmt.condition = parseExpr();
        stmt.body = parseBlockStmt();
        return stmt;
    }

    ForStmt Parser::parseForStmt()
    {
        match(TokenKind::KwFor);
        match(TokenKind::LParen);

        ForStmt stmt{};
        stmt.init = Box<StmtNode>::make(parseStmt());
        stmt.condition = parseExpr();
        consume(TokenKind::Semicolon);
        stmt.increment = parseExpr();

        match(TokenKind::RParen);

        stmt.body = parseBlockStmt();
        return stmt;
    }

    ExprStmt Parser::parseExprStmt()
    {
        auto stmt = ExprStmt{parseExpr()};
        consume(TokenKind::Semicolon);
        return stmt;
    }

    DeclStmt Parser::parseDeclStmt()
    {
        return DeclStmt{parseDecl()};
    }

    ReturnStmt Parser::parseReturnStmt()
    {
        match(TokenKind::KwReturn);

        ReturnStmt stmt{};

        if(!check(TokenKind::Semicolon))
            stmt.expr = parseExpr();

        consume(TokenKind::Semicolon);

        return stmt;
    }

    Literal Parser::parseLiteral(const Token &token)
    {
        switch (token.kind)
        {
            case TokenKind::StringLiteral:
                return std::string(token.text.substr(1, token.text.size()-2));
            case TokenKind::NumberLiteral:
            {
                std::string_view text = token.text;
                const bool isHex = text.starts_with("0x") || text.starts_with("0X");
                if (isHex) text.remove_prefix(2);

                size_t suffixStart = text.size();
                while (suffixStart > 0 && std::isalpha(text[suffixStart - 1]) && !(isHex && std::isxdigit(text[suffixStart - 1])))
                    suffixStart--;

                const std::string_view digits = text.substr(0, suffixStart);
                const std::string_view suffixText = text.substr(suffixStart);

                if (!isHex && (digits.find('.') != std::string_view::npos || digits.find_first_of("eE") != std::string_view::npos))
                {
                    const double value = std::strtod(std::string(digits).c_str(), nullptr);
                    auto suffix = FloatSuffix::None;
                    if (suffixText == "f" || suffixText == "F") suffix = FloatSuffix::Explicit;
                    else if (suffixText == "h" || suffixText == "H") suffix = FloatSuffix::Half;
                    return FloatLiteral{ value, suffix };
                }

                const int64_t value = std::strtoll(std::string(digits).c_str(), nullptr, isHex ? 16 : 10);
                auto suffix = IntegerSuffix::None;
                if (suffixText == "u" || suffixText == "U") suffix = IntegerSuffix::Unsigned;
                else if (suffixText == "l" || suffixText == "L") suffix = IntegerSuffix::Long;

                return IntegerLiteral{ value, suffix, isHex };
            }
            case TokenKind::KwTrue:
                return true;
            case TokenKind::KwFalse:
                return false;
            default:
                return "Invalid argument";
        }
    }

    void Parser::consumeTriviaAndAttribs()
    {
        while (!done() && (check(TokenKind::BlockComment) || check(TokenKind::LineComment) || check(TokenKind::LBracketLBracket)))
        {
            if(check(TokenKind::LBracketLBracket))
            {
                consumeAttribute();
                continue;
            }

            m_pos++;

            Trivia trivia{};
            trivia.isBlockComment = previous().kind == TokenKind::BlockComment;
            if(!trivia.isBlockComment)
                trivia.text = previous().text.substr(2, previous().text.size()-2);
            else
                trivia.text = previous().text.substr(2, previous().text.size()-4);

            trivia.span = previous().span;

            m_pendingTrivia.push_back(trivia);
        }
    }

    void Parser::consumeAttribute()
    {
        SourceSpan startSpan = peek().span;
        match(TokenKind::LBracketLBracket);

        QualifiedName name{};
        while(true)
        {
            name.segments.emplace_back(advance().text);

            if(!match(TokenKind::ColonColon))
                break;
        }

        std::vector<Literal> args{};
        if(match(TokenKind::LParen))
        {
            while (!match(TokenKind::RParen))
            {
                args.emplace_back(parseLiteral(advance()));
                match(TokenKind::Comma);
            }
        }

        m_pendingAttributes.push_back(Attribute{ name, args, SourceSpan::between(startSpan, peek().span) });

        match(TokenKind::RBracketRBracket);
    }

    Type Parser::parseType()
    {
        auto leadingTrivia = takePendingTrivia();
        const Token startToken = peek();

        Type baseType = parseTypeName(startToken, leadingTrivia);

        return parseArraySuffix(std::move(baseType), startToken);
    }

    Type Parser::parseTypeName(const Token& startToken, const std::vector<Trivia>& leadingTrivia)
    {
        if (!consume(TokenKind::Identifier))
            return Type{leadingTrivia, startToken.span, PrimitiveType{PrimitiveKind::Unknown}};

        const std::string_view baseName = startToken.text;
        const bool hasGenericArgs = match(TokenKind::LAngle);

        if (const auto it = kResourceTypeNames.find(baseName); it != kResourceTypeNames.end())
            return parseResourceType(it->second, startToken, leadingTrivia, hasGenericArgs);

        if (hasGenericArgs && isVectorTypeName(baseName))
            return parseVectorType(startToken, leadingTrivia);
        if (hasGenericArgs && isMatrixTypeName(baseName))
            return parseMatrixType(startToken, leadingTrivia);

        if (!hasGenericArgs)
            if (const auto scalar = parsePrimitiveKind(baseName))
                return Type{leadingTrivia, startToken.span, PrimitiveType{*scalar}};

        return parseCustomType(baseName, startToken, leadingTrivia, hasGenericArgs);
    }

    Type Parser::parseArraySuffix(Type elementType, const Token& startToken)
    {
        if (!match(TokenKind::LBracket))
            return elementType;

        ArrayType arrayType{};
        arrayType.elementType = Box<Type>::make(std::move(elementType));

        if (!match(TokenKind::RBracket))
        {
            arrayType.size = Box<ExprNode>::make(parseExpr());
            consume(TokenKind::RBracket);
        }

        return Type{takePendingTrivia(), startToken.span, std::move(arrayType)};
    }

    Type Parser::parseVectorType(const Token &startToken, const std::vector<Trivia> &leadingTrivia)
    {
        const Type innerType = parseType();
        const PrimitiveKind innerPrimitive = std::get<PrimitiveType>(innerType.kind).kind;
        consume(TokenKind::RAngle);

        VectorType vecType{};
        vecType.scalarType = innerPrimitive;
        vecType.dimension = startToken.text[3] - '0';

        return Type{
            .trivia = leadingTrivia,
            .span = SourceSpan::between(startToken.span, previous().span),
            .kind = vecType,
        };
    }

    Type Parser::parseMatrixType(const Token &startToken, const std::vector<Trivia> &leadingTrivia)
    {
        const Type innerType = parseType();
        const PrimitiveKind innerPrimitive = std::get<PrimitiveType>(innerType.kind).kind;
        match(TokenKind::RAngle);

        MatrixType matType{};
        matType.scalarType = innerPrimitive;
        matType.rows = startToken.text[3] - '0';
        matType.columns = startToken.text[5] - '0';

        return Type{
            .trivia = leadingTrivia,
            .span = SourceSpan::between(startToken.span, previous().span),
            .kind = matType,
        };
    }

    Type Parser::parseResourceType(const ResourceBindingKind resourceKind, const Token &startToken,
                                   const std::vector<Trivia> &leadingTrivia, const bool hasGenericArgs)
    {
        Box<Type> elementType{};

        if(hasGenericArgs)
        {
            elementType = Box<Type>::make(parseType());
            match(TokenKind::RAngle);
        }

        ResourceBindingType resourceType;

        resourceType.kind = resourceKind;
        resourceType.elementType = std::move(elementType);

        return Type{
            .trivia = leadingTrivia,
            .span = SourceSpan::between(startToken.span, previous().span),
            .kind = std::move(resourceType),
        };
    }

    Type Parser::parseCustomType(const std::string_view baseName, const Token &startToken,
                                 const std::vector<Trivia> &leadingTrivia, const bool hasGenericArgs)
    {
        std::vector<Box<Type>> genericArgs{};

        if(hasGenericArgs)
        {
            while (!match(TokenKind::RAngle))
            {
                genericArgs.push_back(Box<Type>::make(std::move(parseType())));
                match(TokenKind::Comma);
            }
        }

        CustomType customType;

        customType.name = QualifiedName{{std::string(baseName)}},
        customType.genericArgs = std::move(genericArgs);

        return Type{
            .trivia = leadingTrivia,
            .span = SourceSpan::between(startToken.span, previous().span),
            .kind = std::move(customType),
        };
    }

    Type Parser::parseGenericTypeArgs(const Token &startToken, const std::vector<Trivia>& leadingTrivia)
    {
        Type innerType = parseType();

        if(startToken.text.starts_with("vec") && startToken.text.size() == 4)
        {
            // TODO add validation to ensure is primitive type
            const PrimitiveKind innerPrimitive = std::get<PrimitiveType>(innerType.kind).kind;
            match(TokenKind::RAngle);

            VectorType vecType{};
            vecType.scalarType = innerPrimitive;
            vecType.dimension = startToken.text[3] - '0';

            return Type{
                .trivia = leadingTrivia,
                .span = SourceSpan::between(startToken.span, previous().span),
                .kind = vecType,
            };
        }

        if(startToken.text.starts_with("mat") && startToken.text.size() == 6)
        {
            // TODO add validation to ensure is primitive type
            const PrimitiveKind innerPrimitive = std::get<PrimitiveType>(innerType.kind).kind;
            consume(TokenKind::RAngle);

            MatrixType matType{};
            matType.scalarType = innerPrimitive;
            matType.rows = startToken.text[3] - '0';
            matType.columns = startToken.text[5] - '0';

            return Type{
                .trivia = leadingTrivia,
                .span = SourceSpan::between(startToken.span, previous().span),
                .kind = matType,
            };
        }

        std::vector<Box<Type>> genericArgs{};
        genericArgs.push_back(Box<Type>::make(std::move(innerType)));

        do
        {
            genericArgs.push_back(Box<Type>::make(std::move(parseType())));
        } while (match(TokenKind::Comma));

        consume(TokenKind::RAngle);

        CustomType customType;

        customType.name = QualifiedName{{std::string(startToken.text)}},
        customType.genericArgs = std::move(genericArgs);

        return Type{
            .trivia = leadingTrivia,
            .span = SourceSpan::between(startToken.span, previous().span),
            .kind = std::move(customType),
        };
    }

    Box<PrimitiveKind> Parser::parsePrimitiveKind(const std::string_view name)
    {
        if(const auto it = kPrimitiveTypeMap.find(name); it != kPrimitiveTypeMap.end())
            return Box<PrimitiveKind>::make(it->second);

        return nullptr;
    }

    std::optional<std::pair<uint32_t, uint32_t>> Parser::getBindingPower(const TokenKind kind)
    {
        if (const auto op = tokenToBinaryOp(kind))
        {
            return kInfixBindingPower[op.value()];
        }

        switch (kind)
        {
            case TokenKind::LParen:
            case TokenKind::LBracket:
            case TokenKind::Dot:
                return std::pair<uint32_t, uint32_t>{100, 0};

            default:
                return std::nullopt;
        }
    }

    bool Parser::isLookaheadGenericArgs() const
    {
        uint32_t offset = 1;

        int angleDepth = 1;

        while (peek().kind != TokenKind::EndOfFile)
        {
            const Token& token = peek(offset);

            if (token.kind == TokenKind::LAngle)
            {
                angleDepth++;
            }
            else if (token.kind == TokenKind::RAngle)
            {
                angleDepth--;

                if (angleDepth == 0)
                {
                    if (peek(offset+1).kind != TokenKind::EndOfFile)
                    {
                        TokenKind afterGT = peek(offset+1).kind;

                        return afterGT == TokenKind::LParen
                            || afterGT == TokenKind::LBrace
                            || afterGT == TokenKind::Identifier
                            || afterGT == TokenKind::ColonColon
                            || afterGT == TokenKind::Dot
                            || afterGT == TokenKind::Comma
                            || afterGT == TokenKind::RParen
                            || afterGT == TokenKind::Semicolon
                            || afterGT == TokenKind::Equal;
                    }
                    return false;
                }
            }
            else if (token.kind == TokenKind::Semicolon ||
                     token.kind == TokenKind::LBrace ||
                     token.kind == TokenKind::RBrace ||
                     token.kind == TokenKind::Plus ||
                     token.kind == TokenKind::Minus ||
                     token.kind == TokenKind::Star ||
                     token.kind == TokenKind::Slash ||
                     token.kind == TokenKind::EqualEqual ||
                     token.kind == TokenKind::ExclamationEqual ||
                     token.kind == TokenKind::AmpAmp ||
                     token.kind == TokenKind::PipePipe)
            {
                return false;
            }

            offset++;
        }

        return false;
    }

    std::vector<Trivia> Parser::takePendingTrivia()
    {
        auto trivia = m_pendingTrivia;
        m_pendingTrivia.clear();
        return trivia;
    }

    std::vector<Attribute> Parser::takePendingAttributes()
    {
        auto attribs = m_pendingAttributes;
        m_pendingAttributes.clear();
        return attribs;
    }

    bool Parser::done() const
    {
        return m_pos >= m_tokens.size() ||  peek().kind == TokenKind::EndOfFile;
    }

    const Token& Parser::peek(const uint32_t offset) const
    {
        return m_tokens[m_pos + offset];
    }

    const Token& Parser::previous() const
    {
        return m_tokens[m_pos - 1];
    }

    const Token& Parser::advance()
    {
        if (!done())
        {
            m_pos++;
        }
        consumeTriviaAndAttribs();
        return previous();
    }

    bool Parser::check(const TokenKind kind) const
    {
        if (done()) return false;
        return peek().kind == kind;
    }

    bool Parser::match(const TokenKind kind)
    {
        if (check(kind))
        {
            advance();
            return true;
        }
        return false;
    }

    bool Parser::consume(const TokenKind kind, std::string_view context, const Severity severity)
    {
        if (match(kind)) return true;

        std::string msg = std::format("expected {} but found {}",
                                        tokenKindDisplayName(kind),
                                        tokenKindDisplayName(peek().kind));
        if (!context.empty())
            msg += std::format(" {}", context);

        m_diagnostics.add(msg, peek().span, severity);
        return false;
    }
}
