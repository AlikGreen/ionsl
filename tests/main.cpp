#include <chrono>
#include <fstream>
#include <print>

#include <ionsl/ionsl.h>


std::optional<std::string> loadFile(const std::string& path)
{
    std::ifstream file{path, std::ios::in | std::ios::binary};
    if (!file)
        return std::nullopt;

    std::ostringstream buffer;
    buffer << file.rdbuf();

    if (file.bad()) // rdbuf failed partway through, ignore normal eof
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
        std::println("{} at line {} column {}: {}",
            diag.severity == ionsl::Severity::Error ? "error" : "warning",
            diag.sourceSpan.startLoc.line, diag.sourceSpan.startLoc.column, diag.message);
}

// Loads + compiles a single standalone shader file and reports whether it succeeded.
// Returns the compiled module on success so callers that need to link/generate can reuse it.
std::optional<ionsl::Module> testShaderFile(const std::string& path)
{
    std::println("--- {} ---", path);

    const std::string startPath = R"(C:\Users\alikg\CLionProjects\ionsl\tests\)";

    auto source = loadFile(startPath + path);
    if (!source)
    {
        std::println("could not read file: {}", path);
        return std::nullopt;
    }

    auto module = ionsl::Compiler::compile(*source, path);
    printDiagnostics(module.diagnostics);

    if (!module.diagnostics.empty())
    {
        std::println("FAILED\n");
        return std::nullopt;
    }

    std::println("OK ({} top-level decls)\n", module.decls.size());
    return module;
}


int main()
{
    const std::vector<std::string> standaloneShaders = {
        "01_basic_vertex_fragment.ionsl",
        "02_compute_push_constants.ionsl",
        "03_control_flow.ionsl",
        "04_structs_arrays.ionsl",
    };

    // Shaders 1-4 are self-contained: parse/resolve/check them individually,
    // no linking or specialization required.
    for (const auto& path : standaloneShaders)
        testShaderFile(path);

    // Shader 5 declares a generic entry point + an interface, but no concrete
    // material implementing it — that lives in a separate module we compile
    // and link in, exactly like the two-module material workflow.
    auto genericModule = testShaderFile("05_generics_material.ionsl");
    if (!genericModule) return 1;

    auto stdModule = testShaderFile("../std/core.ionsl");
    if (!stdModule) return 1;

    ionsl::SpecializationRequest spec{};
    spec.genericFunction = genericModule->findFunctionByName("fragmentMain");
    spec.bindings["M"] = ionsl::StructType{ .decl = genericModule->findStructByName("RustyMetal") };

    if (!spec.genericFunction || !spec.bindings["M"].decl)
    {
        std::println("could not find fragmentMain or RustyMetal for specialization");
        return 1;
    }

    ionsl::LinkDesc linkDesc{};
    linkDesc.modules = { *genericModule, *stdModule };
    linkDesc.specializations = { spec };

    ionsl::Module linked = ionsl::Compiler::link(linkDesc);
    printDiagnostics(linked.diagnostics);
    if (!linked.diagnostics.empty())
    {
        std::println("link FAILED\n");
        return 1;
    }

    auto reflection = ionsl::Compiler::reflect(linked);

    std::println("--- reflection ---");
    for (const auto& ep : reflection.entryPoints)
        std::println("entry point: {}", ep.name);
    for (const auto& res : reflection.resources)
        std::println("resource: {}", res.name);

    if (reflection.entryPoints.empty())
    {
        std::println("no entry points found after linking");
        return 1;
    }

    auto code = ionsl::Compiler::generate(linked, reflection.entryPoints.at(0).name);
    std::println("\n--- generated HLSL ---\n{}", code);

    return 0;
}