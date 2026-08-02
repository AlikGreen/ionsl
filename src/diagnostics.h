#pragma once
#include <string>
#include <vector>

#include "sourceSpan.h"

namespace ionsl
{
enum class Severity
{
    Info, Warning, Error
};

struct Diagnostic
{
    std::string message;
    SourceSpan sourceSpan;
    Severity severity;
};

class DiagnosticSink
{
public:
    void add(const std::string& message, SourceSpan span, Severity severity) { m_diagnostics.emplace_back(message, span, severity); }
    std::vector<Diagnostic> diagnostics() { return m_diagnostics; }
private:
    std::vector<Diagnostic> m_diagnostics{};
};
}
