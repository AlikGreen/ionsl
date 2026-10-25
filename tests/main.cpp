#include <algorithm>
#include <chrono>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <ionsl/ionsl.h>

#include "../compiler/codegen/hlsl/hlslGenerator.h"

namespace
{
    using Clock = std::chrono::steady_clock;

    const std::string kTestsDir  = "../tests/";
    const std::string kOutputDir = "../tests/out/";

    // ---------- helpers ----------

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

    bool hasErrors(const std::vector<ionsl::Diagnostic>& diags)
    {
        return std::ranges::any_of(diags, [](const ionsl::Diagnostic& d)
        {
            return d.severity == ionsl::Severity::Error;
        });
    }

    void printDiagnostics(const std::vector<ionsl::Diagnostic>& diags)
    {
        for (const auto& diag : diags)
        {
            std::cout << "  " << (diag.severity == ionsl::Severity::Error ? "error" : "warning")
                      << " at " << diag.sourceSpan.startLoc.line << ":" << diag.sourceSpan.startLoc.column
                      << ": " << diag.message << '\n';
        }
    }

    struct Stopwatch
    {
        Clock::time_point start = Clock::now();

        // milliseconds since the last lap (or construction)
        double lap()
        {
            const auto now = Clock::now();
            const double ms = std::chrono::duration<double, std::milli>(now - start).count();
            start = now;
            return ms;
        }
    };

    // ---------- test description ----------

    struct EntryRequest
    {
        std::string name;                   // entry point name in the source
        std::vector<std::string> typeArgs;  // empty = not generic, compile as written
        std::string outputName;             // only used when typeArgs is non-empty

        const std::string& finalName() const { return typeArgs.empty() ? name : outputName; }
    };

    struct TestCase
    {
        std::string file;
        std::vector<EntryRequest> entries;  // all compiled together into one output
    };

    struct Timings
    {
        double parse = 0, link = 0, extract = 0, generate = 0, reflect = 0;
        double total() const { return parse + link + extract + generate + reflect; }
    };

    struct TestResult
    {
        std::string file;
        bool ok = true;
        std::string failedStage;
        Timings timings;
        size_t entryPoints = 0;
        size_t resources = 0;
    };

    // ---------- one test ----------

    TestResult runTest(ionsl::Compiler& compiler, const TestCase& test, const bool verbose)
    {
        std::cout << "--- " << test.file << " ---\n";

        TestResult result{.file = test.file};
        auto fail = [&](std::string stage)
        {
            std::cout << "FAILED at " << stage << "\n\n";
            result.ok = false;
            result.failedStage = std::move(stage);
            return result;
        };

        Stopwatch sw;

        // read
        const auto source = loadFile(kTestsDir + test.file);
        if (!source)
        {
            std::cout << "  could not read " << kTestsDir + test.file << '\n';
            return fail("read");
        }

        // parse
        auto module = compiler.parse(*source);
        result.timings.parse = sw.lap();
        printDiagnostics(module.diagnostics().logs());
        if (hasErrors(module.diagnostics().logs()))
            return fail("parse");

        // request a specialization for every generic entry point
        ionsl::LinkDescription desc{};
        desc.modules = { &module };

        for (const auto& entry : test.entries)
        {
            if (entry.typeArgs.empty())
                continue;

            const auto generic = module.findEntryPoint(entry.name);
            if (!generic)
            {
                std::cout << "  no entry point named '" << entry.name << "'\n";
                return fail("find entry point");
            }

            std::vector<ionsl::TypeId> typeArgs;
            for (const auto& typeName : entry.typeArgs)
            {
                const auto type = module.findType(typeName);
                if (!type)
                {
                    std::cout << "  unknown type argument '" << typeName << "'\n";
                    return fail("find type");
                }
                typeArgs.push_back(*type);
            }

            desc.specializations.emplace_back(ionsl::EntrypointSpecialization{*generic, std::move(typeArgs), entry.outputName});
        }

        sw.lap(); // exclude the lookups above from the link time

        // link (this also runs semantic analysis and instantiation)
        ionsl::Module linked = compiler.link(desc);
        result.timings.link = sw.lap();
        printDiagnostics(linked.diagnostics().logs());
        if (hasErrors(linked.diagnostics().logs()))
            return fail("link");

        // find every requested entry point in the linked module
        std::vector<ionsl::DeclId> roots;
        for (const auto& entry : test.entries)
        {
            const auto ep = linked.findEntryPoint(entry.finalName());
            if (!ep)
            {
                std::cout << "  entry point '" << entry.finalName() << "' missing after link\n";
                return fail("find linked entry point");
            }
            roots.push_back(*ep);
        }

        sw.lap();

        // extract all entry points together, so they share one set of declarations
        ionsl::Module extracted = compiler.extract(linked, roots);
        result.timings.extract = sw.lap();

        // generate
        const std::string hlsl = compiler.generate<ionsl::HlslGenerator>(extracted);
        result.timings.generate = sw.lap();

        // reflect
        const auto reflection = compiler.reflect(extracted);
        result.timings.reflect = sw.lap();

        result.entryPoints = reflection.entryPoints.size();
        for (const auto& reflected : reflection.entryPoints)
            result.resources += reflected.resources.size();

        if (result.entryPoints != test.entries.size())
        {
            std::cout << std::format("  reflection reported {} entry point(s), expected {}\n",
                                     result.entryPoints, test.entries.size());
            return fail("reflect");
        }

        // output: one file per test containing every entry point
        std::filesystem::create_directories(kOutputDir);
        const std::string outPath = kOutputDir + std::filesystem::path(test.file).stem().string() + ".hlsl";
        if (!saveFile(outPath, hlsl))
            std::cout << "  warning: could not write " << outPath << '\n';

        if (verbose)
            std::cout << hlsl << '\n';

        std::cout << std::format("OK  {} decls extracted, {} entry point(s), {} resource(s), {:.2f} ms\n\n",
                                 extracted.declarations().size(), result.entryPoints, result.resources, result.timings.total());
        return result;
    }

    void printSummary(const std::vector<TestResult>& results)
    {
        std::cout << "=== summary ===\n";
        std::cout << std::format("{:<6}{:<24}{:>8}{:>8}{:>9}{:>10}{:>9}{:>9}\n",
                                 "", "file", "parse", "link", "extract", "generate", "reflect", "total");

        size_t failed = 0;
        Timings sum;

        for (const auto& r : results)
        {
            const auto& t = r.timings;
            if (r.ok)
                std::cout << std::format("{:<6}{:<24}{:>8.2f}{:>8.2f}{:>9.2f}{:>10.2f}{:>9.2f}{:>9.2f}\n",
                                         "PASS", r.file, t.parse, t.link, t.extract, t.generate, t.reflect, t.total());
            else
            {
                std::cout << std::format("{:<6}{:<24}failed at {}\n", "FAIL", r.file, r.failedStage);
                ++failed;
            }

            sum.parse += t.parse;       sum.link += t.link;
            sum.extract += t.extract;   sum.generate += t.generate;
            sum.reflect += t.reflect;
        }

        std::cout << std::format("{:<6}{:<24}{:>8.2f}{:>8.2f}{:>9.2f}{:>10.2f}{:>9.2f}{:>9.2f}\n",
                                 "", "total (ms)", sum.parse, sum.link, sum.extract, sum.generate, sum.reflect, sum.total());
        std::cout << std::format("\n{} passed, {} failed\n", results.size() - failed, failed);
    }
}

int main(const int argc, char** argv)
{
    const bool verbose = argc > 1 && std::string(argv[1]) == "-v"; // -v prints the generated HLSL

    // list every entry point each file should produce; test1 is the vertex + fragment pair
    // adjust the names for test2..test5 to match your files
    const std::vector<TestCase> tests = {
        { "test1.ionsl", { { "vertexMain" }, { "fragmentMain" } } },
        { "test2.ionsl", { { "computeMain" } } },
        { "test3.ionsl", { { "fragmentMain" } } },
        { "test4.ionsl", { { "fragmentMain" } } },
        { "test5.ionsl", { { "fragmentMain" } } },
        { "generictest.ionsl", { { "vertexMain", { "f32" }, "vertexMain_f32" } } },
    };

    ionsl::Compiler compiler; // shared across tests, so stdlib setup cost lands in the first test

    std::vector<TestResult> results;
    for (const auto& test : tests)
        results.push_back(runTest(compiler, test, verbose));

    printSummary(results);

    return std::ranges::all_of(results, [](const TestResult& r) { return r.ok; }) ? 0 : 1;
}