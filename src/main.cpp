#include <chrono>
#include <print>

#include "checker.h"
#include "codeGen.h"
#include "compiler.h"
#include "lexer.h"
#include "parser.h"
#include "reflector.h"
#include "resolver.h"


int main()
{
    std::string source = R"(
interface Material
{
    fn evaluate(input : SurfaceInput) -> SurfaceOutput;
}

struct VertexOutput
{
    offset : f32,
}

struct SurfaceInput
{
    normal : f32,
}

fn buildSurfaceInput(output : VertexOutput) -> SurfaceInput
{
    return SurfaceInput(1.0);
}

struct This
{
    x: f32,
}

var randomThing : ConstantBuffer<This>;

[[shader("pixel")]]
fn fragmentMain<M : Material>(input : VertexOutput) -> vec4<f32>
{
    var si : SurfaceInput = buildSurfaceInput(input);
    var mat : M;
    var so : SurfaceOutput = mat.evaluate(si);
    // lighting loop, tone mapping, etc - written ONCE
    return vec4<f32>(so.baseColor, 1.0);
})";

    std::string specialization = R"(
struct SurfaceOutput
{
    baseColor : vec3<f32>,
}

struct PBRMaterial : Material
{
    fn evaluate(input : SurfaceInput) -> SurfaceOutput
    {
        var mut so : SurfaceOutput;
        so.baseColor = vec3<f32>(1.0, 1.0, 1.0);
        return so;
    }
})";

    auto genericModule = ionsl::Compiler::compile(source, "generic.ionsl");

    for(const auto& diag : genericModule.diagnostics())
        std::println("Error at line {} column {}: {}", diag.sourceSpan.startLoc.line, diag.sourceSpan.startLoc.column, diag.message);
    if(!genericModule.diagnostics().empty()) return 0;

    auto matModule = ionsl::Compiler::compile(specialization, "mat.ionsl");

    for(const auto& diag : matModule.diagnostics())
        std::println("Error at line {} column {}: {}", diag.sourceSpan.startLoc.line, diag.sourceSpan.startLoc.column, diag.message);
    if(!matModule.diagnostics().empty()) return 0;

    ionsl::SpecializationRequest spec{};
    spec.genericFunction = genericModule.findFunctionByName("fragmentMain");
    spec.bindings["M"] = ionsl::StructType { .decl = matModule.findStructByName("PBRMaterial") };

    ionsl::LinkDesc linkDesc{};
    linkDesc.modules = { genericModule, matModule };
    linkDesc.specializations = { spec };

    ionsl::Module linked = ionsl::Compiler::link(linkDesc);

    auto checkDiags = ionsl::Checker::check(linked);

    for(const auto& diag : checkDiags)
        std::println("Error at line {} column {}: {}", diag.sourceSpan.startLoc.line, diag.sourceSpan.startLoc.column, diag.message);
    if(!checkDiags.empty()) return 0;

    auto code = ionsl::CodeGen::generate(linked);

    std::println("Code:\n {}", code);


    auto refl = ionsl::Reflector::reflect(linked);

    for(auto resource : refl.resources)
    {
        std::println("Res: {}", resource.name);
    }

}
