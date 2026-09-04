#include "semanticAnalyzer.h"

#include <ranges>

#include "constantEvaluator.h"
#include "../ast/declarations.h"

namespace ionsl
{
    void SemanticAnalyzer::analyze()
    {
        for(const auto decl : m_module.declarations)
        {
            checkDeclSignature(*decl);
        }

        for(const auto decl : m_module.declarations)
        {
            checkDeclaration(*decl);
        }
    }

    void SemanticAnalyzer::analyze(Module& module, SymbolTable& symbolTable, TypeSystem& typeSystem, DeclTable& declTable, ScopeTable& scopeTable)
    {
        return SemanticAnalyzer(module, symbolTable, typeSystem, declTable, scopeTable).analyze();
    }

    TypeId SemanticAnalyzer::resolveType(TypeSyntax& syntax)
    {
        if(auto* namedSyntax = syntax.as<NamedTypeSyntax>())
        {
            if(namedSyntax->name.string(m_symbols) == "vector")
                return resolveVectorType(*namedSyntax);
            if(namedSyntax->name.string(m_symbols) == "matrix")
                return resolveMatrixType(*namedSyntax);

            PrimitiveKind primitiveKind = toPrimitiveKind(namedSyntax->name.string(m_symbols));
            if(primitiveKind != PrimitiveKind::Unknown)
                return m_typeSystem.types().getPrimitiveType(primitiveKind);

            return resolveNamedType(*namedSyntax);
        }

        if(const auto* arraySyntax = syntax.as<ArrayTypeSyntax>())
        {
            return resolveArrayType(*arraySyntax);
        }

        return TypeIdInvalid;
    }

    TypeId SemanticAnalyzer::resolveVectorType(const NamedTypeSyntax &syntax)
    {
        if(syntax.arguments.size() != 2)
        {
            // TODO diagnostics
            return TypeIdInvalid;
        }


        const TypeId elementType = resolveType(*syntax.arguments.at(0)->as<TypeArgumentType>()->type);
        const auto res = m_constEval.evaluate(*syntax.arguments.at(1)->as<TypeArgumentValue>()->expression);
        if(!res) return TypeIdInvalid; // TODO diagnostics

        const uint32_t dimension = std::get<uint64_t>(res->value);

        return m_typeSystem.types().getVectorType(elementType, dimension);
    }

    TypeId SemanticAnalyzer::resolveMatrixType(const NamedTypeSyntax &syntax)
    {
        if(syntax.arguments.size() != 3)
        {
            // TODO diagnostics
            return TypeIdInvalid;
        }

        const TypeId elementType = resolveType(*syntax.arguments.at(0)->as<TypeArgumentType>()->type);

        const auto rowsRes = m_constEval.evaluate(*syntax.arguments.at(1)->as<TypeArgumentValue>()->expression);
        if(!rowsRes) return TypeIdInvalid; // TODO diagnostics
        const uint32_t rows = std::get<uint64_t>(rowsRes->value);

        const auto columnsRes = m_constEval.evaluate(*syntax.arguments.at(1)->as<TypeArgumentValue>()->expression);
        if(!columnsRes) return TypeIdInvalid; // TODO diagnostics
        const uint32_t columns = std::get<uint64_t>(columnsRes->value);

        return m_typeSystem.types().getMatrixType(elementType, rows, columns);
    }

    TypeId SemanticAnalyzer::resolveNamedType(const NamedTypeSyntax &syntax)
    {
        const auto decls = m_scopeTable.findDecls(m_currentScope, syntax.name);

        for(const DeclId id : decls)
        {
            // TODO make sure type matches full signature when introducing generics
            Declaration* decl = m_declTable.get(id);
            if(decl->is<StructDecl>())
                return m_typeSystem.types().getStructType(decl->id);
            if(decl->is<InterfaceDecl>())
                return m_typeSystem.types().getInterfaceType(decl->id);
            // TODO alias decl
        }

        m_module.diagnostics.error(syntax.span, "Unknown type {}", syntax.name.string(m_symbols));

        return TypeIdInvalid;
    }

    TypeId SemanticAnalyzer::resolveArrayType(const ArrayTypeSyntax &syntax)
    {
        const TypeId elementType = resolveType(*syntax.elementType);

        std::optional<uint32_t> size = std::nullopt;

        if(syntax.size)
        {
            const auto sizeRes = m_constEval.evaluate(*syntax.size);
            if(!sizeRes) return TypeIdInvalid; // TODO diagnostics
            size = std::get<uint64_t>(sizeRes->value);
        }

        return m_typeSystem.types().getArrayType(elementType, size);
    }


    TypeId SemanticAnalyzer::checkExpression(Expression &expression)
    {
        if(auto* binaryExpr = expression.as<BinaryExpr>())
            return checkBinaryExpr(*binaryExpr);
        if(auto* unaryExpr = expression.as<UnaryExpr>())
            return checkUnaryExpr(*unaryExpr);
        if(auto* callExpr = expression.as<CallExpr>())
            return checkCallExpr(*callExpr);
        if(auto* identifierExpr = expression.as<IdentifierExpr>())
            return checkIdentifierExpr(*identifierExpr);
        if(auto* indexExpr = expression.as<IndexExpr>())
            return checkIndexExpr(*indexExpr);
        if(auto* literalExpr = expression.as<LiteralExpr>())
            return checkLiteralExpr(*literalExpr);
        if(auto* fieldAccessExpr = expression.as<FieldAccessExpr>())
            return checkFieldAccessExpr(*fieldAccessExpr);

        return TypeIdInvalid;
    }

    TypeId SemanticAnalyzer::checkBinaryExpr(BinaryExpr &expression)
    {
        const auto leftType = checkExpression(*expression.left);
        const auto rightType = checkExpression(*expression.right);

        if(leftType == TypeIdInvalid || rightType == TypeIdInvalid) return TypeIdInvalid;

        const auto result = m_typeSystem.resolveBinaryTypes(expression.op, leftType, rightType);
        if(!result) return TypeIdInvalid; // TODO diagnostics

        expression.left = makeConversion(expression.left, result->leftType);
        expression.right = makeConversion(expression.right, result->rightType);

        expression.resultType = result->resultType;

        return expression.resultType;
    }

    TypeId SemanticAnalyzer::checkUnaryExpr(UnaryExpr &expression)
    {
        const auto operandType = checkExpression(*expression.operand);

        if(operandType == TypeIdInvalid) return TypeIdInvalid;

        const auto result = m_typeSystem.resolveUnaryType(expression.op, operandType);
        if(!result) return TypeIdInvalid;

        expression.operand = makeConversion(expression.operand, result->operandType);

        expression.resultType = result->resultType;

        return expression.resultType;
    }

    TypeId SemanticAnalyzer::checkCallExpr(CallExpr &expression)
    {
        if(auto* identifier = expression.callee->as<IdentifierExpr>())
        {
            return checkIdentifierCall(expression, *identifier);
        }

        // TypeId calleeType = checkExpression(*expression.callee);
        // if(calleeType == TypeIdInvalid)

        return TypeIdInvalid;

        // TODO check non identifier calls eg
        // var func = () -> { return 2; };
        // var x = func()

    }

    TypeId SemanticAnalyzer::checkIdentifierCall(CallExpr &expression, const IdentifierExpr &identifier)
    {
        std::vector<TypeId> argumentTypes;

        for(auto* argument : expression.args)
        {
            TypeId type = checkExpression(*argument);

            if(type == TypeIdInvalid)
                return TypeIdInvalid;

            argumentTypes.push_back(type);
        }

        auto candidates = m_scopeTable.findDecls(m_currentScope, identifier.name);
        uint32_t bestConversionCost = ~0u;
        Declaration* bestCandidate = nullptr;

        for(const auto candidate : candidates)
        {
            Declaration* decl = m_declTable.get(candidate);

            if(auto* funcDecl = decl->as<FunctionDecl>())
            {
                std::vector<TypeId> paramTypes;
                for(const auto* param : funcDecl->params)
                    paramTypes.push_back(param->resolvedType);

                auto conversion = m_typeSystem.conversionCost(argumentTypes, paramTypes);
                if(!conversion) continue;

                if(bestConversionCost > *conversion)
                {
                    bestConversionCost = *conversion;
                    bestCandidate = decl;
                }
            }
        }

        if(bestCandidate == nullptr) return TypeIdInvalid;

        if(auto* funcDecl = bestCandidate->as<FunctionDecl>())
        {
            for(const auto& [arg, param] : std::views::zip(expression.args, funcDecl->params))
            {
                makeConversion(arg, param->resolvedType);
            }

            return funcDecl->resolvedReturnType;
        }


        // TODO methods and variables
        return TypeIdInvalid;
    }

    TypeId SemanticAnalyzer::checkIdentifierExpr(IdentifierExpr &expression) const
    {
        auto candidates = m_scopeTable.findDecls(m_currentScope, expression.name);

        TypeId bestCandidateType  = TypeIdInvalid;

        for(const auto& id : candidates)
        {
            const auto decl = m_declTable.get(id);
            if(const auto value = decl->as<ValueDecl>())
                bestCandidateType = value->resolvedType;
            if(const auto struc = decl->as<StructDecl>())
                bestCandidateType = m_typeSystem.types().getStructType(struc->id);
            if(const auto interface = decl->as<InterfaceDecl>())
                bestCandidateType = m_typeSystem.types().getInterfaceType(interface->id);
            // TODO other decl types
        }

        // TODO diagnostics if no candidates

        expression.resultType = bestCandidateType;
        return bestCandidateType;
    }

    TypeId SemanticAnalyzer::checkIndexExpr(IndexExpr &expression)
    {
        checkExpression(*expression.index);
        const TypeId arrayTypeId = checkExpression(*expression.array);

        TypeInfo arrayType = m_typeSystem.types().getInfo(arrayTypeId);

        // TODO validation
        expression.resultType = arrayType.as<ArrayType>()->elementType;
        return expression.resultType;
    }

    TypeId SemanticAnalyzer::checkLiteralExpr(LiteralExpr &expression) const
    {
        const auto typeId = std::visit([](auto&& arg) -> TypeId
        {
            using T = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<T, bool>)
                return TypeIdBool;
            if constexpr (std::is_same_v<T, uint64_t>)
                return TypeIdU64;
            if constexpr (std::is_same_v<T, int64_t>)
                return TypeIdI64;
            if constexpr (std::is_same_v<T, double>)
                return TypeIdF64;
            if constexpr (std::is_same_v<T, std::string>)
                return TypeIdString;

            return TypeIdInvalid;

        }, expression.literal);

        expression.resultType = typeId;
        return typeId;
    }

    TypeId SemanticAnalyzer::checkFieldAccessExpr(FieldAccessExpr &expression)
    {
        const TypeId objTypeId = checkExpression(*expression.object);

        if(objTypeId == TypeIdInvalid)
            return TypeIdInvalid;

        TypeInfo objType = m_typeSystem.types().getInfo(objTypeId);
        // TODO validation and interfaces
        const DeclId structDeclId = objType.as<StructType>()->declId;
        const StructDecl& structDecl = *m_declTable.get(structDeclId)->as<StructDecl>();
        const ValueDecl& field = *structDecl.findField(expression.memberName);
        expression.resultType = field.resolvedType;
        return expression.resultType;
    }

    void SemanticAnalyzer::checkStatement(Statement &statement)
    {
        if(const auto exprStmt = statement.as<ExprStmt>())
            checkExpression(*exprStmt->expr);
        if(const auto declStmt = statement.as<DeclStmt>())
            checkDeclaration(*declStmt->decl);
        if(const auto blockStmt = statement.as<BlockStmt>())
            checkBlockStmt(*blockStmt);
        if(const auto ifStmt = statement.as<IfStmt>())
            checkIfStmt(*ifStmt);
        if(const auto whileStmt = statement.as<WhileStmt>())
            checkWhileStmt(*whileStmt);
        if(const auto forStmt = statement.as<ForStmt>())
            checkForStmt(*forStmt);
        if(const auto returnStmt = statement.as<ReturnStmt>())
            checkReturnStmt(*returnStmt);
        if(statement.is<BreakStmt>() || statement.is<ContinueStmt>())
            checkBreakContinueStmt();
    }

    void SemanticAnalyzer::checkBlockStmt(const BlockStmt &statement)
    {
        m_currentScope = statement.scope;
        for(const auto stmt : statement.statements)
        {
            checkStatement(*stmt);
        }
        m_currentScope = m_scopeTable.getScope(m_currentScope).parent;
    }

    void SemanticAnalyzer::checkIfStmt(const IfStmt &statement)
    {
        const TypeId conditionType = checkExpression(*statement.condition);
        if(conditionType != TypeIdBool)
        {
            m_module.diagnostics.add("condition in an if statement must resolve to a bool", statement.condition->span, Severity::Error);
            return;
        }

        checkBlockStmt(*statement.thenBranch);

        if(statement.elseBranch)
            checkBlockStmt(*statement.elseBranch);
    }

    void SemanticAnalyzer::checkForStmt(const ForStmt &statement)
    {
        checkStatement(*statement.init);
        checkExpression(*statement.condition);
        checkExpression(*statement.increment);
        checkBlockStmt(*statement.body);
    }

    void SemanticAnalyzer::checkWhileStmt(const WhileStmt &statement)
    {
        checkExpression(*statement.condition);
        checkBlockStmt(*statement.body);
    }

    void SemanticAnalyzer::checkReturnStmt(const ReturnStmt &statement)
    {
        TypeId returnType = TypeIdVoid;

        if(statement.expr)
            returnType = checkExpression(*statement.expr);

        // TODO check return type against function type
    }

    void SemanticAnalyzer::checkBreakContinueStmt()
    {
        // TODO make sure in for or while loop
    }

    void SemanticAnalyzer::checkDeclaration(Declaration &declaration)
    {
        if(const auto funcDecl = declaration.as<FunctionDecl>())
            checkFunctionDecl(*funcDecl);
        if(const auto interfaceDecl = declaration.as<InterfaceDecl>())
            checkInterfaceDecl(*interfaceDecl);
        if(const auto structDecl = declaration.as<StructDecl>())
            checkStructDecl(*structDecl);
        if(const auto valDecl = declaration.as<ValueDecl>())
            checkValueDecl(*valDecl);
    }

    void SemanticAnalyzer::checkDeclSignature(Declaration &declaration)
    {
        if(const auto funcDecl = declaration.as<FunctionDecl>())
            checkFunctionSignature(*funcDecl);
        if(const auto interfaceDecl = declaration.as<InterfaceDecl>())
            checkInterfaceSignature(*interfaceDecl);
        if(const auto structDecl = declaration.as<StructDecl>())
            checkStructSignature(*structDecl);
    }

    void SemanticAnalyzer::checkFunctionSignature(FunctionDecl &declaration)
    {
        declaration.resolvedReturnType = resolveType(*declaration.returnType);

        for(const auto param : declaration.params)
            checkValueDecl(*param);
    }

    void SemanticAnalyzer::checkStructSignature(const StructDecl &declaration)
    {
        for(const auto field : declaration.fields)
            checkValueDecl(*field);

        for(const auto method : declaration.methods)
            checkFunctionSignature(*method);
    }

    void SemanticAnalyzer::checkInterfaceSignature(const InterfaceDecl &declaration)
    {
        for(const auto method : declaration.methods)
            checkFunctionSignature(*method);
    }


    void SemanticAnalyzer::checkFunctionDecl(const FunctionDecl &declaration)
    {
        checkBlockStmt(*declaration.body);
    }

    void SemanticAnalyzer::checkStructDecl(const StructDecl &declaration)
    {
        for(const auto method : declaration.methods)
            checkFunctionDecl(*method);
    }

    void SemanticAnalyzer::checkInterfaceDecl(const InterfaceDecl &declaration)
    {
        for(const auto method : declaration.methods)
            checkFunctionDecl(*method);
    }

    void SemanticAnalyzer::checkValueDecl(ValueDecl &declaration)
    {
        declaration.resolvedType = resolveType(*declaration.type);

        if(declaration.initializer)
            checkExpression(*declaration.initializer);
    }

    PrimitiveKind SemanticAnalyzer::toPrimitiveKind(const std::string &name)
    {
        const std::unordered_map<std::string, PrimitiveKind> primitiveKinds = {
            {"void", PrimitiveKind::Void},
            {"bool", PrimitiveKind::Bool},

            {"i8", PrimitiveKind::Int8},
            {"i16", PrimitiveKind::Int16},
            {"i32", PrimitiveKind::Int32},
            {"i64", PrimitiveKind::Int64},

            {"u8", PrimitiveKind::UInt8},
            {"u16", PrimitiveKind::UInt16},
            {"u32", PrimitiveKind::UInt32},
            {"u64", PrimitiveKind::UInt64},

            {"f16", PrimitiveKind::Float16},
            {"f32", PrimitiveKind::Float32},
            {"f64", PrimitiveKind::Float64},

            {"string", PrimitiveKind::String}
        };

        if(const auto it = primitiveKinds.find(name); it != primitiveKinds.end())
            return it->second;

        return PrimitiveKind::Unknown;
    }

    Expression* SemanticAnalyzer::makeConversion(Expression* operand, const TypeId type) const
    {
        if(operand->resultType == type)
            return operand;

        auto* expr = m_module.arena.create<ConversionExpr>();
        expr->kind = ConversionKind::Implicit;
        expr->targetType = type;
        expr->operand = operand;
        return expr;
    }
}
