#pragma once
#include <cstdint>
#include <format>
#include <string>
#include <vector>

namespace ionsl
{
struct SourceLocation
{
    uint32_t line = 1, column = 1, offset = 0;
};

struct SourceSpan
{
    SourceSpan() = default;
    explicit SourceSpan(const SourceLocation start, const SourceLocation end)
        : startLoc(start), endLoc(end) { }

    SourceLocation startLoc;
    SourceLocation endLoc;

    static SourceSpan between(const SourceSpan first, const SourceSpan last)
    {
        return SourceSpan{first.startLoc, last.endLoc};
    }
};

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

    template<typename... Args>
    void error(SourceSpan span, std::format_string<Args...> fmt, Args&&... args)
    {
        m_diagnostics.emplace_back(std::format(fmt, std::forward<Args>(args)...), span, Severity::Error);
    }

    std::vector<Diagnostic>& diagnostics() { return m_diagnostics; }
    [[nodiscard]] const std::vector<Diagnostic>& diagnostics() const { return m_diagnostics; }
private:
    std::vector<Diagnostic> m_diagnostics{};
};
}
