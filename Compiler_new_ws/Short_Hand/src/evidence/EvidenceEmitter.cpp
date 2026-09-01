#include "EvidenceEmitter.h"
#include <map>
#include <ostream>

static std::string esc(const std::string &s) {
    std::string o;
    for (char c : s) {
        if (c == '"' || c == '\\') { o += '\\'; o += c; }
        else if (c == '\n') { o += "\\n"; }
        else { o += c; }
    }
    return o;
}

static void writeTypedC3EcoValue(std::ostream &out, const C3EcoValueData &value) {
    if (value.kind == C3EcoValueKind::Integer || value.kind == C3EcoValueKind::Decimal ||
        value.kind == C3EcoValueKind::Boolean) {
        out << value.text;
    } else {
        out << "\"" << esc(value.text) << "\"";
    }
}

EvidenceEmitter::EvidenceEmitter(const std::string &s) : source(s) {}

void EvidenceEmitter::collect(AST_PROGRAM *p) {
    models.clear();
    tensors.clear();
    contracts.clear();
    measures.clear();
    c3eco_declarations.clear();
    infer_calls.clear();
    if (p) p->accept(*this);
}

bool EvidenceEmitter::hasMinimumC3EcoEvidence() const {
    if (contracts.empty() || models.empty() || tensors.empty() || infer_calls.empty()) return false;
    const auto &c = contracts.front();
    return c.has_functional_unit && c.has_success_criteria && c.has_boundary &&
           c.has_mq && c.has_dq && c.has_carbon_factor && c.has_quality_guardrail &&
           c.claims_mode == "evidence_only";
}

bool EvidenceEmitter::hasC3EcoProfileV2() const {
    for (const auto &declaration : c3eco_declarations) {
        if (declaration.kind == C3EcoDeclarationKind::CertificationProfile) return true;
    }
    return false;
}

void EvidenceEmitter::write(AST_PROGRAM *p, std::ostream &out) {
    writeCandidateReport(p, out);
}

void EvidenceEmitter::writeCandidateReport(AST_PROGRAM *p, std::ostream &out) {
    collect(p);
    auto c = contracts.empty() ? GreenAIContractData() : contracts.front();
    bool minimum = hasMinimumC3EcoEvidence();
    const bool profile_v2 = hasC3EcoProfileV2();
    const bool migration_required = !profile_v2 && !c3eco_declarations.empty();

    out << "{\n";
    out << "  \"schema\": \"shorthand.c3eco.candidate_report.v1\",\n";
    out << "  \"report_status\": \"candidate_assessment_only\",\n";
    out << "  \"source_file\": \"" << esc(source) << "\",\n";
    out << "  \"compiler_version\": \"shorthand-greenai-core\",\n";
    out << "  \"minimum_c3eco_evidence_present\": " << (minimum ? "true" : "false") << ",\n";
    out << "  \"official_certification_granted\": false,\n";
    out << "  \"c3eco_language_contract\": \"shorthand.c3eco.language.v1\",\n";
    out << "  \"c3eco_profile_contract\": \"shorthand.c3eco.profile.v2\",\n";
    out << "  \"c3eco_profile_status\": \""
        << (profile_v2 ? "conformant" : (migration_required ? "legacy_migration_review_required" : "not_declared"))
        << "\",\n";
    out << "  \"c3eco_profile_migration_required\": " << (migration_required ? "true" : "false") << ",\n";
    out << "  \"c3eco_declarations\": [";
    for (size_t i = 0; i < c3eco_declarations.size(); ++i) {
        if (i) out << ",";
        const auto &decl = c3eco_declarations[i];
        out << "{\"kind\":\"" << esc(c3EcoDeclarationKindName(decl.kind))
            << "\",\"name\":\"" << esc(decl.name) << "\",\"fields\":{";
        for (size_t j = 0; j < decl.fields.size(); ++j) {
            if (j) out << ",";
            out << "\"" << esc(decl.fields[j].name) << "\":[";
            for (size_t k = 0; k < decl.fields[j].values.size(); ++k) {
                if (k) out << ",";
                out << "\"" << esc(decl.fields[j].values[k].text) << "\"";
            }
            out << "]";
        }
        out << "},\"typed_fields\":{";
        for (size_t j = 0; j < decl.fields.size(); ++j) {
            if (j) out << ",";
            out << "\"" << esc(decl.fields[j].name) << "\":[";
            for (size_t k = 0; k < decl.fields[j].values.size(); ++k) {
                if (k) out << ",";
                out << "{\"type\":\"" << c3EcoValueKindName(decl.fields[j].values[k].kind)
                    << "\",\"value\":";
                writeTypedC3EcoValue(out, decl.fields[j].values[k]);
                out << "}";
            }
            out << "]";
        }
        out << "}}";
    }
    out << "],\n";
    out << "  \"workload\": \"" << esc(c.name) << "\",\n";
    out << "  \"functional_unit\": \"" << esc(c.functional_unit) << "\",\n";
    out << "  \"success_criteria\": \"" << esc(c.success_criteria) << "\",\n";
    out << "  \"measurement_quality\": \"" << esc(c.measurement_quality) << "\",\n";
    out << "  \"data_quality\": \"" << esc(c.data_quality) << "\",\n";
    out << "  \"carbon_factor_gco2e_per_kwh\": " << c.carbon_factor << ",\n";
    out << "  \"energy_budget_j\": " << c.energy_budget_j << ",\n";
    out << "  \"carbon_budget_gco2e\": " << c.carbon_budget_gco2e << ",\n";
    out << "  \"measurement_status\": \"declared_budget_only\",\n";
    out << "  \"runtime_backend\": \"fallback\",\n";
    out << "  \"inference_status\": \"not_executed\",\n";
    out << "  \"reason\": \"backend_not_available_or_not_configured\",\n";
    out << "  \"offsets_gco2e\": 0,\n";
    out << "  \"avoided_impact_gco2e\": 0,\n";
    out << "  \"base_footprint_not_reduced_by_offsets\": true,\n";

    out << "  \"models\": [";
    for (size_t i = 0; i < models.size(); ++i) {
        if (i) out << ",";
        out << "{\"name\":\"" << esc(models[i].name)
            << "\",\"format\":\"" << esc(models[i].format)
            << "\",\"precision\":\"" << esc(models[i].precision)
            << "\",\"input_shape\":\"" << esc(models[i].input_shape)
            << "\",\"output_shape\":\"" << esc(models[i].output_shape)
            << "\",\"backend_preference\":[";
        for (size_t j = 0; j < models[i].backend_preference.size(); ++j) {
            if (j) out << ",";
            out << "\"" << esc(models[i].backend_preference[j]) << "\"";
        }
        out << "]}";
    }
    out << "],\n";

    out << "  \"tensors\": [";
    for (size_t i = 0; i < tensors.size(); ++i) {
        if (i) out << ",";
        out << "{\"name\":\"" << esc(tensors[i].name)
            << "\",\"element_type\":\"" << esc(tensors[i].element_type)
            << "\",\"shape\":\"" << esc(tensors[i].shape_csv)
            << "\",\"rank\":" << tensors[i].rank
            << ",\"total_elements\":" << tensors[i].total_elements << "}";
    }
    out << "],\n";

    out << "  \"infer_calls\": [";
    for (size_t i = 0; i < infer_calls.size(); ++i) {
        if (i) out << ",";
        out << "{\"model\":\"" << esc(infer_calls[i].model_name)
            << "\",\"input\":\"" << esc(infer_calls[i].input_name)
            << "\",\"output\":\"" << esc(infer_calls[i].output_name) << "\"}";
    }
    out << "],\n";

    out << "  \"measurements\": [";
    for (size_t i = 0; i < measures.size(); ++i) {
        if (i) out << ",";
        out << "{\"workload\":\"" << esc(measures[i].workload)
            << "\",\"backend\":\"" << esc(measures[i].backend)
            << "\",\"inferences\":" << measures[i].inferences
            << ",\"watts\":" << measures[i].watts
            << ",\"seconds\":" << measures[i].seconds << "}";
    }
    out << "],\n";

    out << "  \"blocked_certification_items\": [";
    bool first = true;
    auto add_blocker = [&](const std::string &b) {
        if (!first) out << ",";
        out << "\"" << esc(b) << "\"";
        first = false;
    };
    if (contracts.empty()) add_blocker("missing_greenai_contract");
    if (models.empty()) add_blocker("missing_model_declaration");
    if (tensors.empty()) add_blocker("missing_tensor_declaration");
    if (infer_calls.empty()) add_blocker("missing_infer_statement");
    if (!profile_v2) add_blocker("typed_c3eco_profile_not_declared");
    add_blocker("real_backend_execution_not_yet_verified");
    add_blocker("external_certifier_not_signed");
    out << "],\n";
    out << "  \"claim_safe_text\": \"Candidate evidence report only. This output does not grant C3-ECO certification and must not be marketed as certified without external review.\",\n";
    out << "  \"assumptions\": [\"missing telemetry is reported as unavailable or declared_budget_only, never fabricated\"],\n";
    out << "  \"warnings\": [\"optional SDKs absent use deterministic fallback\",\"offsets and avoided impact do not reduce the base footprint\"],\n";
    out << "  \"disclaimer\": \"Evidence report only; this tool does not grant certification.\"\n";
    out << "}\n";
}

void EvidenceEmitter::writeWorkbookCsv(AST_PROGRAM *p, std::ostream &out) {
    collect(p);
    auto c = contracts.empty() ? GreenAIContractData() : contracts.front();
    out << "component,functional_unit,activity_kwh,carbon_factor_gco2e_per_kwh,carbon_kgco2e,measurement_quality,data_quality,measurement_status,evidence_ref\n";
    if (measures.empty()) {
        out << "compute," << esc(c.functional_unit) << ",,,," << esc(c.measurement_quality) << "," << esc(c.data_quality) << ",declared_budget_only," << esc(source) << "\n";
        return;
    }
    for (const auto &m : measures) {
        double kwh = (m.watts * m.seconds) / 3600000.0;
        double kg = (kwh * c.carbon_factor) / 1000.0;
        out << "compute," << esc(c.functional_unit) << "," << kwh << "," << c.carbon_factor << "," << kg << "," << esc(c.measurement_quality) << "," << esc(c.data_quality) << ",declared_budget_only," << esc(source) << "\n";
    }
}

void EvidenceEmitter::writeCheck(AST_PROGRAM *p, std::ostream &out) {
    collect(p);
    bool minimum = hasMinimumC3EcoEvidence();
    const bool profile_v2 = hasC3EcoProfileV2();
    const bool migration_required = !profile_v2 && !c3eco_declarations.empty();
    out << "{\n";
    out << "  \"schema\": \"shorthand.c3eco.check.v1\",\n";
    out << "  \"status\": \"" << (minimum ? "candidate_ready_with_blockers" : "missing_required_evidence") << "\",\n";
    out << "  \"minimum_c3eco_evidence_present\": " << (minimum ? "true" : "false") << ",\n";
    out << "  \"official_certification_granted\": false,\n";
    out << "  \"c3eco_profile_contract\": \"shorthand.c3eco.profile.v2\",\n";
    out << "  \"c3eco_profile_status\": \""
        << (profile_v2 ? "conformant" : (migration_required ? "legacy_migration_review_required" : "not_declared"))
        << "\",\n";
    out << "  \"c3eco_profile_migration_required\": "
        << (migration_required ? "true" : "false") << ",\n";
    out << "  \"blocking_items\": [";
    if (!profile_v2) out << "\"typed_c3eco_profile_not_declared\",";
    out << "\"real_backend_execution_not_yet_verified\",\"external_certifier_not_signed\"],\n";
    out << "  \"disclaimer\": \"Candidate readiness check only; this tool does not grant certification.\"\n";
    out << "}\n";
}

void EvidenceEmitter::writeProfileMigration(AST_PROGRAM *p, std::ostream &out) {
    collect(p);
    const bool profile_v2 = hasC3EcoProfileV2();
    std::map<C3EcoDeclarationKind, std::string> first_names;
    for (const auto &declaration : c3eco_declarations) {
        first_names.emplace(declaration.kind, declaration.name);
    }
    out << "{\n";
    out << "  \"schema\": \"shorthand.c3eco.profile_migration.v1\",\n";
    out << "  \"source_contract\": \"shorthand.c3eco.language.v1\",\n";
    out << "  \"target_contract\": \"shorthand.c3eco.profile.v2\",\n";
    out << "  \"status\": \"" << (profile_v2 ? "already_current" : "review_required") << "\",\n";
    out << "  \"migration_required\": " << (profile_v2 ? "false" : "true") << ",\n";
    out << "  \"official_certification_granted\": false,\n";
    out << "  \"declaration_count\": " << c3eco_declarations.size() << ",\n";
    out << "  \"suggested_profile_references\": {";
    const std::vector<std::pair<const char *, C3EcoDeclarationKind>> references = {
        {"certification", C3EcoDeclarationKind::Certification},
        {"functional_unit", C3EcoDeclarationKind::FunctionalUnit},
        {"workload", C3EcoDeclarationKind::Workload},
        {"boundary", C3EcoDeclarationKind::Boundary},
        {"ai_lifecycle", C3EcoDeclarationKind::AILifecycle},
        {"guardrails", C3EcoDeclarationKind::Guardrails}};
    for (std::size_t i = 0; i < references.size(); ++i) {
        if (i) out << ",";
        out << "\"" << references[i].first << "\":";
        const auto found = first_names.find(references[i].second);
        if (found == first_names.end()) out << "null";
        else out << "\"" << esc(found->second) << "\"";
    }
    out << "},\n";
    out << "  \"review_reasons\": [";
    if (!profile_v2) {
        out << "\"legacy_string_fields_require_typed_review\","
               "\"profile_links_and_validity_window_required\"";
    }
    out << "],\n";
    out << "  \"claim_safe_text\": \"Migration output is candidate preparation only and does not grant C3-ECO certification.\"\n";
    out << "}\n";
}

int EvidenceEmitter::visit(AST_PROGRAM *p) { if (p->code_block) p->code_block->accept(*this); return 0; }
int EvidenceEmitter::visit(AST_LOGIC_BLOCK *b) { if (b->block_statement) b->block_statement->accept(*this); return 0; }
int EvidenceEmitter::visit(AST_STATEMENTS_BLOCK *b) { for (auto s : b->statements) s->accept(*this); return 0; }
int EvidenceEmitter::visit(AST_MODEL_DECLARATION *n) { models.push_back(n->data); return 0; }
int EvidenceEmitter::visit(AST_TENSOR_DECLARATION *n) { tensors.push_back(n->data); return 0; }
int EvidenceEmitter::visit(AST_GREENAI_CONTRACT *n) { contracts.push_back(n->data); return 0; }
int EvidenceEmitter::visit(AST_GREENAI_MEASUREMENT *n) { measures.push_back(n->data); return 0; }
int EvidenceEmitter::visit(AST_C3ECO_DECLARATION *n) { c3eco_declarations.push_back(n->data); return 0; }
int EvidenceEmitter::visit(AST_INFER_STATEMENT *n) { infer_calls.push_back({n->model_name, n->input_name, n->output_name}); return 0; }

#define ESTUB(T) int EvidenceEmitter::visit(T*){ return 0; }
ESTUB(AST_DATA_DECLARATION_BLOCK) ESTUB(AST_FUNCTION_LIST_RULE) ESTUB(AST_EXPRESSION_STATEMENT_RULE) ESTUB(AST_FUNCTION_RULE) ESTUB(AST_FUNCTION_CALL_RULE) ESTUB(AST_ASSIGNMENT_RULE) ESTUB(AST_IF_STATEMENT) ESTUB(AST_BREAK) ESTUB(AST_IF_ELSE_STATEMENT) ESTUB(AST_FOR_LOOP_STATEMENT_RULE) ESTUB(AST_WHILE_LOOP_STATEMENT_RULE) ESTUB(AST_GOTO_STATEMENT_RULE) ESTUB(AST_READ_RULE) ESTUB(AST_PRINT_RULE) ESTUB(AST_LABEL_RULE) ESTUB(AST_GREENAI_REPORT_RULE) ESTUB(AST_AI_INFER_RULE) ESTUB(AST_CONTINUE) ESTUB(AST_RETURN_STATEMENT) ESTUB(AST_BINARY_EXPRESSION_RULE) ESTUB(AST_UNARY_EXPRESSION_RULE) ESTUB(AST_SIMPLE_VARIABLE) ESTUB(AST_ARRAY_VARIABLE) ESTUB(AST_LITERAL) ESTUB(AST_STRING_LITERAL) ESTUB(AST_BOOL_LITERAL) ESTUB(AST_FLOAT_LITERAL) ESTUB(AST_FUNCTION_CALL_EXPRESSION)
