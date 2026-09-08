#include "C3EcoAssessment.h"

#include <cassert>
#include <cmath>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>

using namespace shorthand::c3eco;

namespace shorthand::c3eco {
const Control& control(const AssessmentInput& input, const std::string& id) {
    const auto it = input.controls.find(id);
    if (it == input.controls.end()) throw std::runtime_error("missing control");
    return it->second;
}

bool controlBool(const AssessmentInput& input, const std::string& id) {
    const auto& value = control(input, id).value;
    if (value == "true") return true;
    if (value == "false") return false;
    throw std::runtime_error("bad bool");
}

bool validIdentifierValue(const std::string& value) { return !value.empty(); }
AssessmentInput loadAssessment(const std::string&) { return {}; }
ProfileInfo validateProfile(const std::string&) { return {}; }
MeasurementInfo validateMeasurement(const std::string&) { return {}; }
}  // namespace shorthand::c3eco

namespace {
AssessmentInput baseline(bool ai = true) {
    AssessmentInput input;
    for (int i = 1; i <= 14; ++i) {
        input.gates["G" + std::to_string(i)] =
            {true, true, "verified", "evidence/g" + std::to_string(i)};
    }
    const std::map<char, int> counts = {
        {'A', 8}, {'B', 8}, {'C', 8}, {'D', 8}, {'E', 6}, {'F', 7},
        {'G', 10}, {'H', 5}, {'I', 5}, {'J', 4}, {'K', 7}};
    for (const auto& entry : counts) {
        for (int i = 1; i <= entry.second; ++i) {
            input.criteria[std::string(1U, entry.first) + std::to_string(i)] =
                {5.0, 5.0, true, "verified", "evidence/criterion.json"};
        }
    }
    auto addControl = [&](const std::string& id, const std::string& value,
                          const std::string& evidence = "verified") {
        input.controls[id] = {value, evidence, "evidence/control.json"};
    };
    addControl("assessment_id", "a1");
    addControl("requested_claims", "certified_diamond");
    addControl("approved_na_domains", ai ? "none" : "G", "independent");
    addControl("cumulative_omitted_percent", "0");
    addControl("largest_unaddressed_omission_percent", "0");
    addControl("eco_regression_control_defined", "true");
    addControl("baseline_energy_per_unit_j", "none");
    addControl("eco_regression_justification_approved", "false");
    addControl("high_severity_claim_risk", "false");
    addControl("independent_review", "true");
    addControl("public_report", "true");
    addControl("continuous_telemetry", "true");
    addControl("public_evidence_summary", "true");
    addControl("registry_ready", "true");
    addControl("measurement_mq_override", "4", "independent");
    addControl("measurement_dq_override", "4", "independent");
    addControl("penalty_points", "0");
    addControl("full_boundary_legal_claim_support", "false");
    addControl("offsets_reported_separately", "true");
    addControl("green_ai_supporting_evidence", "true");
    if (!ai) {
        for (int i = 1; i <= 10; ++i) {
            input.criteria["G" + std::to_string(i)] =
                {0.0, 0.0, false, "na", "evidence/na.json"};
        }
    }
    return input;
}
}  // namespace

int main() {
    const ProfileInfo aiProfile{true, "S6_AI_GENAI", 1000.0};
    const MeasurementInfo measured{3, 3, 5.0, 1.0, 0.4, 1U};

    auto input = baseline(true);
    auto domains = calculateDomains(input, true);
    auto decision = decide(input, aiProfile, measured, domains);
    assert(decision.eligible);
    assert(decision.candidate_level == 6);
    assert(std::fabs(decision.total_score - 100.0) < 1e-8);
    auto claims = evaluateClaims(input, aiProfile, measured, decision);
    assert(claims.size() == 1U);
    assert(claims[0].status == "candidate_for_external_review");

    auto gateFailure = input;
    gateFailure.gates["G6"].effective_pass = false;
    auto gateDecision = decide(gateFailure, aiProfile, measured,
                               calculateDomains(gateFailure, true));
    assert(!gateDecision.eligible);
    assert(gateDecision.candidate_level == 0);

    auto insufficient = input;
    insufficient.criteria["A1"].effective_score = 1.0;
    auto insufficientDomains = calculateDomains(insufficient, true);
    assert(insufficientDomains.at('A').percent < 100.0);

    auto nonAiInput = baseline(false);
    const ProfileInfo nonAiProfile{false, "S9_DEVELOPER_TOOLS", 1000.0};
    auto nonAiDomains = calculateDomains(nonAiInput, false);
    double redistributedWeight = 0.0;
    for (const auto& entry : nonAiDomains) redistributedWeight += entry.second.effective_weight;
    assert(std::fabs(redistributedWeight - 100.0) < 1e-8);
    assert(nonAiDomains.at('G').effective_weight == 0.0);
    assert(nonAiDomains.at('H').effective_weight == 6.0);
    auto nonAiDecision = decide(nonAiInput, nonAiProfile, measured, nonAiDomains);
    assert(nonAiDecision.eligible);
    assert(nonAiDecision.candidate_level == 6);

    MeasurementInfo highUncertainty = measured;
    highUncertainty.uncertainty_percent = 20.0;
    auto uncertaintyInput = input;
    uncertaintyInput.controls["requested_claims"].value = "certified_silver";
    auto uncertaintyDecision = decide(uncertaintyInput, aiProfile, highUncertainty,
                                      calculateDomains(uncertaintyInput, true));
    assert(uncertaintyDecision.eligible);
    assert(uncertaintyDecision.candidate_level == 3);

    auto regressionInput = input;
    regressionInput.controls["requested_claims"].value = "certified_bronze";
    regressionInput.controls["baseline_energy_per_unit_j"].value = "3000";
    auto regressionDecision = decide(regressionInput, aiProfile, measured,
                                     calculateDomains(regressionInput, true));
    assert(regressionDecision.eligible);
    assert(regressionDecision.eco_regression_triggered);
    assert(regressionDecision.candidate_level == 2);

    auto comparativeInput = input;
    comparativeInput.controls["requested_claims"].value = "comparative";
    auto comparativeDecision = decide(comparativeInput, aiProfile, measured,
                                      calculateDomains(comparativeInput, true));
    assert(!comparativeDecision.eligible);
    assert(comparativeDecision.candidate_level == 0);
    auto comparativeClaims = evaluateClaims(comparativeInput, aiProfile, measured,
                                            comparativeDecision);
    assert(comparativeClaims[0].status == "deferred_pr95");

    auto unapprovedNa = input;
    unapprovedNa.criteria["H1"] = {0.0, 0.0, false, "na", "evidence/na.json"};
    bool threw = false;
    try {
        (void)calculateDomains(unapprovedNa, true);
    } catch (const std::exception&) {
        threw = true;
    }
    assert(threw);

    std::cout << "PASS: PR90 C3-ECO scoring unit\n";
    return 0;
}
