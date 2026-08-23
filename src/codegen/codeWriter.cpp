#include "codeWriter.h"

namespace ions
{
    void CodeWriter::write(const std::string_view text)
    {
        m_output << text;
        m_atLineStart = false;
    }

    void CodeWriter::writeLine(std::string_view text)
    {
        if(!m_atLineStart) newline();
        m_output << text;
        newline();
    }

    void CodeWriter::newline()
    {
        m_output << '\n';
        writeIndent();
        m_atLineStart = true;
    }

    void CodeWriter::indent()
    {
        m_indent++;
    }

    void CodeWriter::unindent()
    {
        m_indent--;
    }

    void CodeWriter::beginBlock()
    {
        writeLine("{");
        indent();
    }

    void CodeWriter::endBlock()
    {
        unindent();
        writeLine("}");
    }

    void CodeWriter::writeIndent()
    {
        for(size_t i = 0; i < m_indent; i++)
            m_output << "   ";
    }

    std::string CodeWriter::string() const
    {
        return m_output.str();
    }
}
