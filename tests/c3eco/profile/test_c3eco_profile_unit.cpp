#include "ast/AST.h"
#include "evidence/EvidenceEmitter.h"
#include "visitors/SemanticAnalyzer.h"

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

C3EcoFieldData field(const std::string &name,
                     C3EcoValueKind kind,
                     const std::string &value) {
    return C3EcoFieldData{name, {{kind, value}}};
}

C3EcoDeclarationData declaration(C3EcoDeclarationKind kind,
                                 const std::string &name,
                                 std::vector<C3EcoFieldData> fields) {
    C3EcoDeclarationData result;
    result.kind = kind;
    result.name = name;
    result.fields = std::move(fields);
    return result;
}

void require(bool condition, const std::string &message) {
    if (!condition) {
        std::cerr << "profile unit failure: " << message << '\n';
        std::exit(1);
    }
}

} // namespace

int main() {
    auto *statements = new AST_STATEMENTS_BLOCK();
    statements->push_back(new AST_C3ECO_DECLARATION(declaration(
        C3EcoDeclarationKind::CertificationProfile, "profile",
        {field("profile_version", C3EcoValueKind::Integer, "2"),
         field("certification", C3EcoValueKind::Identifier, "identity"),
         field("functional_unit", C3EcoValueKind::Identifier, "unit"),
         field("workload", C3EcoValueKind::Identifier, "load"),
         field("boundary", C3EcoValueKind::Identifier, "scope"),
         field("ai_lifecycle", C3EcoValueKind::Identifier, "lifecycle"),
         field("guardrails", C3EcoValueKind::Identifier, "guards"),
         field("valid_from", C3EcoValueKind::String, "2026-08-27"),
         field("valid_until", C3EcoValueKind::String, "2027-08-27")})));
    statements->push_back(new AST_C3ECO_DECLARATION(declaration(
        C3EcoDeclarationKind::Certification, "identity",
        {field("version", C3EcoValueKind::String, "1.2.0"),
         field("owner", C3EcoValueKind::String, "ExampleCo"),
         field("release_date", C3EcoValueKind::String, "2026-08-27"),
         field("software_class", C3EcoValueKind::Identifier, "S6_AI_GENAI"),
         field("deployment_mode", C3EcoValueKind::Identifier, "kubernetes"),
         field("geography", C3EcoValueKind::String, "GB"),
         field("validity_period", C3EcoValueKind::Integer, "365")})));
    statements->push_back(new AST_C3ECO_DECLARATION(declaration(
        C3EcoDeclarationKind::FunctionalUnit, "unit",
        {field("denominator", C3EcoValueKind::Integer, "1000"),
         field("unit", C3EcoValueKind::Identifier, "successful_inference"),
         field("success_condition", C3EcoValueKind::String, "valid prediction"),
         field("quality_metric", C3EcoValueKind::Identifier, "accuracy"),
         field("quality_threshold", C3EcoValueKind::Decimal, "0.9")})));
    statements->push_back(new AST_C3ECO_DECLARATION(declaration(
        C3EcoDeclarationKind::Workload, "load",
        {field("traffic_profile", C3EcoValueKind::Identifier, "production_representative"),
         field("batch_size", C3EcoValueKind::Integer, "1"),
         field("concurrency", C3EcoValueKind::Integer, "32"),
         field("warmup_runs", C3EcoValueKind::Integer, "5"),
         field("measured_runs", C3EcoValueKind::Integer, "30"),
         field("cache_state", C3EcoValueKind::Identifier, "mixed")})));
    C3EcoDeclarationData boundary = declaration(
        C3EcoDeclarationKind::Boundary, "scope",
        {field("include", C3EcoValueKind::Identifier, "compute"),
         field("exclude", C3EcoValueKind::Identifier, "thirdparty_ai_api"),
         field("exclusion_reason", C3EcoValueKind::String, "provider managed"),
         field("exclusion_materiality_percent", C3EcoValueKind::Decimal, "0.5"),
         field("materiality_threshold_percent", C3EcoValueKind::Decimal, "1.0"),
         field("opaque_provider_treatment", C3EcoValueKind::Identifier, "conservative_estimate")});
    statements->push_back(new AST_C3ECO_DECLARATION(boundary));
    statements->push_back(new AST_C3ECO_DECLARATION(declaration(
        C3EcoDeclarationKind::AILifecycle, "lifecycle",
        {field("role", C3EcoValueKind::Identifier, "hosted_model_consumer"),
         field("model_provider", C3EcoValueKind::String, "Provider"),
         field("lifecycle_scope", C3EcoValueKind::Identifier, "inference"),
         field("training_included", C3EcoValueKind::Boolean, "false"),
         field("fine_tuning_included", C3EcoValueKind::Boolean, "false"),
         field("evaluation_included", C3EcoValueKind::Boolean, "true")})));
    statements->push_back(new AST_C3ECO_DECLARATION(declaration(
        C3EcoDeclarationKind::Guardrails, "guards",
        {field("functional_tests", C3EcoValueKind::Boolean, "true"),
         field("accuracy", C3EcoValueKind::Decimal, "0.9"),
         field("p95_latency_ms", C3EcoValueKind::Integer, "500"),
         field("error_rate_percent", C3EcoValueKind::Decimal, "1.0"),
         field("security_scan", C3EcoValueKind::Boolean, "true"),
         field("accessibility", C3EcoValueKind::Boolean, "true"),
         field("privacy_telemetry", C3EcoValueKind::Boolean, "true")})));

    auto *globals = new AST_DATA_DECLARATION_BLOCK();
    globals->push_back("seed");
    globals->setType(ShortType::Int);
    auto *program = new AST_PROGRAM(globals, new AST_FUNCTION_LIST_RULE(),
                                    new AST_LOGIC_BLOCK(statements));
    SemanticAnalyzer analyzer;
    require(program->accept(analyzer) == 0 && !analyzer.diagnostics.hasErrors(),
            "valid forward-linked typed profile must pass semantic validation");

    EvidenceEmitter emitter("profile-unit.short");
    std::ostringstream report;
    emitter.writeCandidateReport(program, report);
    require(report.str().find("\"c3eco_profile_status\": \"conformant\"") != std::string::npos,
            "candidate report must identify conformant profile v2");
    require(report.str().find("\"type\":\"integer\",\"value\":1000") != std::string::npos,
            "candidate report must preserve native integer evidence");
    require(report.str().find("\"official_certification_granted\": false") != std::string::npos,
            "profile evidence must remain claim safe");

    std::ostringstream migration;
    emitter.writeProfileMigration(program, migration);
    require(migration.str().find("\"status\": \"already_current\"") != std::string::npos,
            "current profile must not be marked for legacy migration");
    std::cout << "PASS C3-ECO typed profile unit\n";
    return 0;
}
