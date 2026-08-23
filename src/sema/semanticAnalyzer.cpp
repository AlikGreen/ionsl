#include "semanticAnalyzer.h"

#include "../ast/declarations.h"

namespace ionsl
{
    void SemanticAnalyzer::analyze()
    {
        for(auto decl : m_module.declarations)
        {
            checkDeclaration(*decl);
        }
    }

    void SemanticAnalyzer::buildDeclTable()
    {
        for(auto decl : m_module.declarations)
        {
            if(const auto struc = decl->as<StructDecl>())
                m_declTable[struc->name].push_back(struc->id);
            else if(const auto interface = decl->as<InterfaceDecl>())
                m_declTable[struc->name].push_back(interface->id);
            else if(const auto func = decl->as<FunctionDecl>())
                m_declTable[struc->name].push_back(func->id);
        }
    }

    TypeId SemanticAnalyzer::resolveType(TypeSyntax& syntax)
    {
        if(auto* namedSyntax = syntax.as<NamedTypeSyntax>())
        {
            if(namedSyntax->name.string(m_symbols) == "vector")
                return resolveVectorType(*namedSyntax);
            if(namedSyntax->name.string(m_symbols) == "matrix")
                return resolveMatrixType(*namedSyntax);
            else
                return resolveNamedType();
        }
    }

    TypeId SemanticAnalyzer::resolveVectorType(const NamedTypeSyntax &syntax)
    {
        if(syntax.arguments.size() != 2)
        {
            // TODO diagnostics
            return TypeIdInvalid;
        }

        TypeInfo info;
        info.span = syntax.span;

        const TypeId elementType = resolveType(*syntax.arguments.at(0)->as<TypeArgumentType>()->type);
        const uint32_t dimension = std::get<uint32_t>(ConstantEvaluator::evaluate(syntax.arguments.at(1)->as<TypeArgumentValue>()->expression));

        return m_module.typeTable.getVectorType(elementType, dimension);
    }

    TypeId SemanticAnalyzer::resolveMatrixType(NamedTypeSyntax &syntax)
    {
        if(syntax.arguments.size() != 3)
        {
            // TODO diagnostics
            return TypeIdInvalid;
        }

        TypeInfo info;
        info.span = syntax.span;

        const TypeId elementType = resolveType(*syntax.arguments.at(0)->as<TypeArgumentType>()->type);
        const uint32_t rows = std::get<uint64_t>(ConstantEvaluator::evaluate(syntax.arguments.at(1)->as<TypeArgumentValue>()->expression));
        const uint32_t columns = std::get<uint64_t>(ConstantEvaluator::evaluate(syntax.arguments.at(2)->as<TypeArgumentValue>()->expression));

        return m_module.typeTable.getMatrixType(elementType, rows, columns);
    }
}
