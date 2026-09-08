#include "semanticAnalyzer.h"

#include <iostream>
#include <ranges>

#include "constantEvaluator.h"
#include "../ast/declarations.h"

namespace ionsl
{
    void SemanticAnalyzer::analyze()
    {
        m_currentScope = m_module.scope;

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

        return TypeId::Error;
    }

    TypeId SemanticAnalyzer::checkBinaryExpr(BinaryExpr &expression)
    {
        const auto leftType = checkExpression(*expression.left);
        const auto rightType = checkExpression(*expression.right);

        if(leftType == TypeId::Error || rightType == TypeId::Error) return TypeId::Error;

        const auto result = m_typeSystem.resolveBinaryTypes(expression.op, leftType, rightType);
        if(!result) return TypeId::Error; // TODO diagnostics

        expression.left = makeConversion(expression.left, result->leftType);
        expression.right = makeConversion(expression.right, result->rightType);

        expression.resultType = result->resultType;

        return expression.resultType;
    }

    TypeId SemanticAnalyzer::checkUnaryExpr(UnaryExpr &expression)
    {
        const auto operandType = checkExpression(*expression.operand);

        if(operandType == TypeId::Error) return TypeId::Error;

        const auto result = m_typeSystem.resolveUnaryType(expression.op, operandType);
        if(!result) return TypeId::Error;

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
        // if(calleeType == TypeId::Error)

        return TypeId::Error;

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

            if(type == TypeId::Error)
                return TypeId::Error;

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
                    paramTypes.push_back(param->type->resolvedType);

                auto conversion = m_typeSystem.conversionCost(argumentTypes, paramTypes);
                if(!conversion) continue;

                if(bestConversionCost > *conversion)
                {
                    bestConversionCost = *conversion;
                    bestCandidate = decl;
                }
            }
        }

        if(bestCandidate == nullptr) return TypeId::Error;

        if(auto* funcDecl = bestCandidate->as<FunctionDecl>())
        {
            for(const auto& [arg, param] : std::views::zip(expression.args, funcDecl->params))
            {
                makeConversion(arg, param->type->resolvedType);
            }

            return funcDecl->returnType->resolvedType;
        }


        // TODO methods and variables
        return TypeId::Error;
    }

    TypeId SemanticAnalyzer::checkIdentifierExpr(IdentifierExpr &expression) const
    {
        auto candidates = m_scopeTable.findDecls(m_currentScope, expression.name);

        TypeId bestCandidateType  = TypeId::Error;

        for(const auto& id : candidates)
        {
            const auto decl = m_declTable.get(id);
            if(const auto value = decl->as<ValueDecl>())
                bestCandidateType = value->type->resolvedType;
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
                return TypeId::Bool;
            if constexpr (std::is_same_v<T, uint64_t>)
                return TypeId::U64;
            if constexpr (std::is_same_v<T, int64_t>)
                return TypeId::I64;
            if constexpr (std::is_same_v<T, double>)
                return TypeId::F64;
            if constexpr (std::is_same_v<T, std::string>)
                return TypeId::String;

            return TypeId::Error;

        }, expression.literal);

        expression.resultType = typeId;
        return typeId;
    }

    TypeId SemanticAnalyzer::checkFieldAccessExpr(FieldAccessExpr &expression)
    {
        const TypeId objTypeId = checkExpression(*expression.object);

        if(objTypeId == TypeId::Error)
            return TypeId::Error;

        TypeInfo objType = m_typeSystem.types().getInfo(objTypeId);
        // TODO validation and interfaces
        const DeclId structDeclId = objType.as<StructType>()->declId;
        const StructDecl& structDecl = *m_declTable.get(structDeclId)->as<StructDecl>();
        const ValueDecl& field = *structDecl.findField(expression.memberName);
        expression.resultType = field.type->resolvedType;
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
        if(conditionType != TypeId::Bool)
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
        TypeId returnType = TypeId::Void;

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
        if(const auto aliasDecl = declaration.as<AliasDecl>())
            checkAliasDecl(*aliasDecl);
    }

    void SemanticAnalyzer::checkFunctionSignature(FunctionDecl &declaration)
    {
        m_typeResolver.resolveType(*declaration.returnType, m_currentScope, m_typeSubstitutions);

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

    void SemanticAnalyzer::checkAliasDecl(const AliasDecl &declaration)
    {
        // TODO resolve generic value type params
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
        m_typeResolver.resolveType(*declaration.type, m_currentScope, m_typeSubstitutions);

        if(declaration.initializer)
            checkExpression(*declaration.initializer);
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
