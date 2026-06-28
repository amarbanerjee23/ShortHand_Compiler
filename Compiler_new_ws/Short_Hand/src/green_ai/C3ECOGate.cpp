#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

std::string readFile(const std::string &path, bool &ok) {
    std::ifstream in(path.c_str());
    if (!in) {
        ok = false;
        return std::string();
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    ok = true;
    return ss.str();
}

std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

bool contains(const std::string &haystack, const std::string &needle) {
    return haystack.find(needle) != std::string::npos;
}

void requireToken(const std::string &text, const std::string &token, const std::string &why,
                  std::vector<std::string> &errors) {
    if (!contains(text, token)) {
        errors.push_back("missing required C3-ECO evidence: " + token + " (" + why + ")");
    }
}

void forbidToken(const std::string &lowerText, const std::string &token, const std::string &why,
                 std::vector<std::string> &errors) {
    if (contains(lowerText, token)) {
        errors.push_back("restricted claim requires independent certification review: " + token + " (" + why + ")");
    }
}

int validateOne(const std::string &path, bool strict) {
    bool ok = false;
    const std::string text = readFile(path, ok);
    if (!ok) {
        std::cerr << "c3eco_gate: cannot open " << path << "\n";
        return 2;
    }

    const std::string lt = lower(text);
    std::vector<std::string> errors;
    std::vector<std::string> warnings;

    requireToken(text, "green {", "top-level Green/C3-ECO declaration", errors);
    requireToken(text, "product:", "G1 system identity", errors);
    requireToken(text, "product_version:", "G1 versioned system identity", errors);
    requireToken(text, "owner:", "G1 accountable owner", errors);
    requireToken(text, "release_date:", "G1 release date", errors);
    requireToken(text, "software_class:", "C3-ECO software class such as S6 AI/ML", errors);
    requireToken(text, "deployment_mode:", "G1 deployment mode", errors);
    requireToken(text, "certification_scope:", "G1 declared scope", errors);

    requireToken(text, "functional_unit:", "G2 functional unit", errors);
    requireToken(text, "success_criteria:", "G2 success condition and quality threshold", errors);
    requireToken(text, "boundary:", "G3 boundary declaration", errors);
    requireToken(text, "include:", "G3 included layers", errors);
    requireToken(text, "exclude:", "G3 excluded layers", errors);

    requireToken(text, "measurement_quality:", "G4 measurement-quality class", errors);
    requireToken(text, "data_quality:", "G4/G5 data-quality class", errors);
    requireToken(text, "audit_evidence:", "G8/G9 evidence retention and reproducibility references", errors);
    requireToken(text, "carbon {", "G5 carbon calculation block", errors);
    requireToken(text, "accounting_method:", "G5 carbon accounting method", errors);
    requireToken(text, "region:", "G5 carbon factor geography", errors);
    requireToken(text, "grid_factor_kgco2e_per_kwh:", "G5 location-based carbon factor", errors);
    requireToken(text, "offsets_allowed_in_core_score: false", "G11 no offset-only or offset-subtracted claim", errors);

    requireToken(text, "security_floor:", "G6 security floor", errors);
    requireToken(text, "accessibility_safety_floor:", "G7 accessibility/safety/privacy floor", errors);
    requireToken(text, "no_quality_degradation:", "G13 no quality degradation", errors);
    requireToken(text, "materiality:", "G14 materiality control", errors);
    requireToken(text, "cumulative_omissions_percent_max:", "G14 cumulative omissions limit", errors);
    requireToken(text, "evidence_retention_months:", "G9 evidence retention", errors);
    requireToken(text, "recertification_acceptance:", "G12 surveillance and recertification acceptance", errors);
    requireToken(text, "claims:", "G10 claims integrity declaration", errors);

    requireToken(text, "budgets {", "budgeted energy/carbon/resource limits", errors);
    requireToken(text, "measurements {", "raw or telemetry-derived measurements", errors);
    requireToken(text, "functional_unit_count:", "normalization denominator", errors);

    const bool hasAi = contains(text, "model ") || contains(text, "infer endpoint") || contains(text, "llm_task") || contains(text, "rag_pipeline");
    if (hasAi) {
        requireToken(text, "model_role:", "AI provider/deployer/integrator role separation", errors);
        requireToken(text, "green_model:", "AI model energy and quality evidence", errors);
        requireToken(text, "model_necessity_justification:", "G1 AI necessity justification", errors);
        requireToken(text, "smaller_model_evaluated:", "quality-energy frontier evidence", errors);
        requireToken(text, "quality_energy_frontier_required:", "quality-energy frontier control", errors);
        requireToken(text, "green_infer:", "AI inference energy evidence", errors);
        requireToken(text, "measure_energy_per_inference: true", "AI inference energy must be measured or transparently estimated", errors);
        requireToken(text, "batch_size:", "AI inference batch-size boundary", errors);
        requireToken(text, "precision:", "AI precision/quantization disclosure", errors);
        requireToken(text, "target ", "hardware/runtime deployment target", errors);
        requireToken(text, "runtime:", "runtime/backend disclosure", errors);
        requireToken(text, "language_backend:", "software-stack disclosure", errors);
    }

    if (strict) {
        requireToken(text, "green_mode: \"strict\"", "strict certification-candidate mode", errors);
        requireToken(text, "certification_profile: \"C3-ECO-AI\"", "AI certification profile", errors);
        if (!contains(text, "measurement_quality: \"MQ2\"") && !contains(text, "measurement_quality: \"MQ3\"") && !contains(text, "measurement_quality: \"MQ4\"")) {
            errors.push_back("strict AI certification candidate requires MQ2 or higher");
        }
        if (!contains(text, "data_quality: \"DQ2\"") && !contains(text, "data_quality: \"DQ3\"") && !contains(text, "data_quality: \"DQ4\"")) {
            errors.push_back("strict AI certification candidate requires DQ2 or higher");
        }
    }

    forbidToken(lt, "zero-carbon", "zero-carbon software is restricted and requires exact boundary/legal support", errors);
    forbidToken(lt, "carbon-neutral", "offsets/RECs cannot replace product-level engineering evidence", errors);
    forbidToken(lt, "net-positive", "not a C3-ECO certification level", errors);
    forbidToken(lt, "regenerative", "not a C3-ECO certification level", errors);

    if (!contains(text, "location_based") && !contains(text, "accounting_method: \"both\"")) {
        errors.push_back("location-based carbon footprint is required for all certification levels");
    }
    if (contains(text, "offsets_gco2e:") && !contains(text, "offsets_reported_separately: true")) {
        warnings.push_back("offsets are present; ensure they are reported separately and never subtracted from core score");
    }

    for (size_t i = 0; i < warnings.size(); ++i) {
        std::cerr << "warning: " << warnings[i] << "\n";
    }
    for (size_t i = 0; i < errors.size(); ++i) {
        std::cerr << "error: " << path << ": " << errors[i] << "\n";
    }
    if (!errors.empty()) {
        return 1;
    }
    std::cout << "C3-ECO gate passed: " << path << "\n";
    return 0;
}

} // namespace

int main(int argc, char **argv) {
    bool strict = true;
    std::vector<std::string> files;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--strict") {
            strict = true;
        } else if (arg == "--advisory") {
            strict = false;
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "usage: c3eco_gate [--strict|--advisory] file.greenai...\n";
            return 0;
        } else {
            files.push_back(arg);
        }
    }
    if (files.empty()) {
        std::cerr << "c3eco_gate: at least one .greenai file is required\n";
        return 2;
    }
    int status = 0;
    for (size_t i = 0; i < files.size(); ++i) {
        const int rc = validateOne(files[i], strict);
        if (rc != 0) status = rc;
    }
    return status;
}
