#include "hlslGenerator.h"

namespace ionsl
{
    void HlslGenerator::genStmt(Statement &stmt)
    {
        if(const auto block = stmt.as<BlockStmt>())
            genBlockStmt(*block);
        else if(const auto expr = stmt.as<ExprStmt>())
            genExprStmt(*expr);
        else if(const auto decl = stmt.as<DeclStmt>())
            genDecl(*decl->decl);
        else if(const auto ifStmt = stmt.as<IfStmt>())
            genIfStmt(*ifStmt);
        else if(const auto forStmt = stmt.as<ForStmt>())
            genForStmt(*forStmt);
        else if(const auto whileStmt = stmt.as<WhileStmt>())
            genWhileStmt(*whileStmt);
        else if(const auto returnStmt = stmt.as<ReturnStmt>())
            genReturnStmt(*returnStmt);
        else if(const auto breakStmt = stmt.as<BreakStmt>())
            genBreakStmt(*breakStmt);
        else if(const auto continueStmt = stmt.as<ContinueStmt>())
            genContinueStmt(*continueStmt);
    }

    void HlslGenerator::genExprStmt(ExprStmt &stmt)
    {
        genExpr(*stmt.expr);
        m_writer.write(";");
        m_writer.newline();
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
        m_writer.write(")");
        genStmt(*stmt.thenBranch);

        if(stmt.elseBranch)
        {
            m_writer.write("else ");
            genStmt(*stmt.elseBranch);
        }
    }

    void HlslGenerator::genWhileStmt(WhileStmt &stmt)
    {
        m_writer.write("while(");
        genExpr(*stmt.condition);
        m_writer.write(")");
        genBlockStmt(*stmt.body);
    }

    void HlslGenerator::genForStmt(ForStmt &stmt)
    {
        m_writer.write("for(");
        genForStmtStmt(*stmt.init);
        m_writer.write("; ");
        genExpr(*stmt.condition);
        m_writer.write("; ");
        genExpr(*stmt.increment);
        m_writer.write(")");

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
}