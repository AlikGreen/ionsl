#include <chrono>
#include <fstream>
#include <iostream>
#include <string>

#include <ionsl/ionsl.h>


std::optional<std::string> loadFile(const std::string& path)
{
    std::ifstream file{path, std::ios::in | std::ios::binary};
    if (!file)
        return std::nullopt;

    std::ostringstream buffer;
    buffer << file.rdbuf();

    if (file.bad())
        return std::nullopt;

    return buffer.str();
}

bool saveFile(const std::string& path, const std::string& contents)
{
    std::ofstream file{path, std::ios::out | std::ios::binary | std::ios::trunc};
    if (!file)
        return false;

    file << contents;
    return static_cast<bool>(file);
}

void printDiagnostics(const std::vector<ionsl::Diagnostic>& diags)
{
    for (const auto& diag : diags)
        std::cout << (diag.severity == ionsl::Severity::Error ? "error" : "warning") << " at line " <<
            diag.sourceSpan.startLoc.line << " column " << diag.sourceSpan.startLoc.column << ":" << diag.message << std::endl;
}

bool testShaderFile(const std::string& path, ionsl::Compiler& compiler)
{
    std::cout << "--- " << path << " ---" << std::endl;

    const std::string startPath = R"(C:\Users\alikg\CLionProjects\ionsl\tests\)";

    auto source = loadFile(startPath + path);
    if (!source)
    {
        std::cout << "could not read file: " << path << std::endl;
        return false;
    }

    auto tokens = compiler.tokenize(*source);
    auto module = compiler.parse(tokens);
    printDiagnostics(module.diagnostics.diagnostics());

    if (!module.diagnostics.diagnostics().empty())
    {
        std::cout << "FAILED\n";
        return false;
    }

    std::cout << "OK ({} top-level decls)\n" << module.declarations.size() << std::endl;
    return true;
}


int main()
{
    const std::vector<std::string> standaloneShaders = {
        "01_basic_vertex_fragment.ionsl",
        "02_compute_push_constants.ionsl",
        "03_control_flow.ionsl",
        "04_structs_arrays.ionsl",
    };

    ionsl::Compiler compiler;

    for (const auto& path : standaloneShaders)
    {
        if(!testShaderFile(path, compiler))
            return 0;
    }

    return 0;
}