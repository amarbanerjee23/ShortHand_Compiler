#include "C3EcoAssessment.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <set>
#include <stdexcept>

namespace shorthand::c3eco {
namespace {
constexpr double kEpsilon = 1e-9;
const std::map<char, double> kWeights = {
    {'A',12.0},{'B',12.0},{'C',12.0},{'D',12.0},{'E',8.0},{'F',10.0},
    {'G',10.0},{'H',6.0},{'I',6.0},{'J',4.0},{'K',8.0}};
const std::map<char, int> kCounts = {
    {'A',8},{'B',8},{'C',8},{'D',8},{'E',6},{'F',7},{'G',10},{'H',5},{'I',5},{'J',4},{'K',7}};

void require(bool ok, const std::string& message) {
    if (!ok) throw std::runtime_error(message);
}

double parseNumber(const std::string& text, const std::string& field) {
    try {
        std::size_t used = 0U;
        const double value = std::stod(text, &used);
        require(used == text.size() && std::isfinite(value), "invalid numeric control");
        return value;
    } catch (const std::exception&) {
        throw std::runtime_error("control " + field + " must be a finite number");
    }
}

std::vector<std::string> split(const std::string& text, char delimiter) {
    std::vector<std::string> out;
    std::string current;
    for (char c : text) {
        if (c == delimiter) {
            out.push_back(current);
            current.clear();
        } else {
            current.push_back(c);
        }
    }
    out.push_back(current);
    return out;
}

std::set<char> approvedNaDomains(const AssessmentInput& input) {
    const Control c = control(input, "approved_na_domains");
    if (c.value == "none") return {};
    require(c.evidence_status == "independent" || c.evidence_status == "verified",
            "approved_na_domains requires independent or verified auditor evidence");
    std::set<char> result;
    for (const std::string& token : split(c.value, ',')) {
        require(token.size() == 1U && kWeights.count(token[0]) == 1U,
                "approved_na_domains contains invalid domain: " + token);
        require(result.insert(token[0]).second,
                "approved_na_domains contains duplicate domain: " + token);
    }
    return result;
}

std::vector<std::string> requestedClaims(const AssessmentInput& input) {
    const std::set<std::string> allowed = {
        "none", "measured_baseline", "certified_bronze", "certified_silver", "certified_gold",
        "certified_platinum", "certified_diamond", "green_ai", "comparative", "zero_carbon",
        "carbon_neutral", "net_positive", "renewable_cloud"};
    std::vector<std::string> result;
    std::set<std::string> seen;
    for (const std::string& token : split(control(input, "requested_claims").value, ';')) {
        require(!token.empty() && allowed.count(token) == 1U,
                "unsupported requested_claims token: " + token);
        require(seen.insert(token).second,
                "duplicate requested_claims token: " + token);
        result.push_back(token);
    }
    require(!result.empty(), "requested_claims must not be empty");
    if (result.size() > 1U)
        require(seen.count("none") == 0U,
                "requested_claims none cannot be combined");
    std::sort(result.begin(), result.end());
    return result;
}

int qualityOverride(const AssessmentInput& input, const std::string& id, int measured) {
    const Control c = control(input, id);
    if (c.value == "none") return measured;
    const double raw = parseNumber(c.value, id);
    require(std::floor(raw) == raw && raw >= 1.0 && raw <= 4.0,
            "control " + id + " must be none or integer 1-4");
    const int value = static_cast<int>(raw);
    if (value > measured)
        require(c.evidence_status == "independent",
                id + " may only raise evidence quality with independent evidence");
    return value;
}

int scoreBand(double score) {
    if (score + kEpsilon >= 95.0) return 6;
    if (score + kEpsilon >= 90.0) return 5;
    if (score + kEpsilon >= 80.0) return 4;
    if (score + kEpsilon >= 65.0) return 3;
    if (score + kEpsilon >= 50.0) return 2;
    if (score + kEpsilon >= 40.0) return 1;
    return 0;
}

void addReason(Decision& decision, const std::string& reason) {
    if (std::find(decision.reasons.begin(), decision.reasons.end(), reason) == decision.reasons.end())
        decision.reasons.push_back(reason);
}

void capLevel(Decision& decision, int cap, const std::string& reason) {
    if (decision.candidate_level > cap) {
        decision.candidate_level = cap;
        addReason(decision, reason);
    }
}

double domainPercent(const std::map<char, DomainScore>& domains, char id) {
    const auto it = domains.find(id);
    require(it != domains.end() && it->second.applicable_count > 0,
            std::string("required level domain is not applicable: ") + id);
    return it->second.percent;
}

bool allApplicableDomainsAtLeast(const std::map<char, DomainScore>& domains, double minimum) {
    for (const auto& entry : domains)
        if (entry.second.applicable_count > 0 &&
            entry.second.percent + kEpsilon < minimum)
            return false;
    return true;
}

std::vector<char> activeRecipients(const std::map<char, DomainScore>& domains,
                                   const std::string& ids) {
    std::vector<char> out;
    for (char id : ids) {
        const auto it = domains.find(id);
        if (it != domains.end() && it->second.applicable_count > 0)
            out.push_back(id);
    }
    return out;
}

void distributePool(std::map<char, DomainScore>& domains, double pool,
                    const std::vector<char>& recipients) {
    require(!recipients.empty(), "N/A domain has no valid reallocation recipient");
    double denominator = 0.0;
    for (char id : recipients) denominator += kWeights.at(id);
    require(denominator > 0.0, "invalid N/A reallocation denominator");
    for (char id : recipients)
        domains.at(id).effective_weight += pool * kWeights.at(id) / denominator;
}

bool claimUnsupported(const std::string& claim, const AssessmentInput& input,
                      const ProfileInfo& profile,
                      const MeasurementInfo& measurement,
                      const Decision& decision) {
    if (claim == "none" || claim == "renewable_cloud") return false;
    if (claim == "comparative" || claim == "net_positive") return true;
    if (claim == "zero_carbon")
        return !controlBool(input, "full_boundary_legal_claim_support") ||
               measurement.carbon_kgco2e > kEpsilon;
    if (claim == "carbon_neutral")
        return !controlBool(input, "full_boundary_legal_claim_support") ||
               !controlBool(input, "offsets_reported_separately");
    if (claim == "green_ai")
        return !profile.ai_ml ||
               !controlBool(input, "green_ai_supporting_evidence") ||
               decision.candidate_level < 4;
    const std::map<std::string, int> ranks = {
        {"measured_baseline",1}, {"certified_bronze",2},
        {"certified_silver",3}, {"certified_gold",4},
        {"certified_platinum",5}, {"certified_diamond",6}};
    const auto it = ranks.find(claim);
    require(it != ranks.end(), "internal claim mapping error");
    return decision.candidate_level < it->second;
}

std::string jsonEscape(const std::string& value) {
    std::string out;
    for (unsigned char c : value) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                require(c >= 0x20U,
                        "control character cannot be emitted in JSON");
                out.push_back(static_cast<char>(c));
        }
    }
    return out;
}
}  // namespace

std::map<char, DomainScore> calculateDomains(const AssessmentInput& input,
                                             bool aiMl) {
    const std::set<char> approved = approvedNaDomains(input);
    std::map<char, DomainScore> domains;
    std::vector<char> fullNa;

    for (const auto& spec : kCounts) {
        DomainScore score;
        score.id = spec.first;
        score.base_weight = kWeights.at(spec.first);
        for (int i = 1; i <= spec.second; ++i) {
            const Criterion& criterion =
                input.criteria.at(std::string(1U, spec.first) + std::to_string(i));
            if (criterion.applicable) {
                ++score.applicable_count;
                score.score_sum += criterion.effective_score;
            } else {
                ++score.na_count;
            }
        }
        if (score.na_count > 0)
            require(approved.count(spec.first) == 1U,
                    std::string("N/A criteria require approved_na_domains evidence for domain ") + spec.first);
        if (score.applicable_count == 0) {
            fullNa.push_back(spec.first);
        } else {
            score.percent = score.score_sum * 100.0 /
                            (5.0 * static_cast<double>(score.applicable_count));
            score.effective_weight = score.base_weight;
        }
        domains.emplace(spec.first, score);
    }

    if (aiMl)
        require(domains.at('G').applicable_count > 0,
                "AI/ML profile cannot make the entire G domain N/A");
    else
        require(domains.at('G').applicable_count == 0,
                "non-AI profile must mark the entire G domain N/A");

    for (char id : fullNa) {
        const double pool = kWeights.at(id);
        if (id == 'G') {
            require(!aiMl,
                    "AI domain G cannot be reallocated while AI is in the certified boundary");
            distributePool(domains, pool,
                           activeRecipients(domains, "ABCDEFK"));
        } else if (id == 'H' || id == 'J') {
            distributePool(domains, pool,
                           activeRecipients(domains, "FK"));
        } else {
            std::string all;
            for (const auto& entry : domains)
                if (entry.first != id && entry.second.applicable_count > 0)
                    all.push_back(entry.first);
            distributePool(domains, pool,
                           activeRecipients(domains, all));
        }
    }

    double totalWeight = 0.0;
    for (auto& entry : domains) {
        DomainScore& score = entry.second;
        if (score.applicable_count > 0)
            score.weighted_points =
                score.effective_weight * score.percent / 100.0;
        totalWeight += score.effective_weight;
    }
    require(std::fabs(totalWeight - 100.0) <= 1e-7,
            "N/A reallocation must preserve total domain weight at 100");
    return domains;
}

Decision decide(const AssessmentInput& input, const ProfileInfo& profile,
                const MeasurementInfo& measurement,
                const std::map<char, DomainScore>& domains) {
    Decision decision;
    decision.mq = qualityOverride(input, "measurement_mq_override",
                                  measurement.mq);
    decision.dq = qualityOverride(input, "measurement_dq_override",
                                  measurement.dq);
    decision.uncertainty = measurement.uncertainty_percent;
    decision.current_energy_per_unit_j =
        measurement.facility_energy_kwh * 3600000.0 /
        profile.functional_unit_denominator;

    for (const auto& entry : input.gates)
        decision.effective_gates[entry.first] = entry.second.effective_pass;
    decision.effective_gates["G1"] =
        decision.effective_gates["G1"] && !profile.software_class.empty();
    decision.effective_gates["G2"] =
        decision.effective_gates["G2"] &&
        profile.functional_unit_denominator > 0.0;
    decision.effective_gates["G4"] =
        decision.effective_gates["G4"] &&
        measurement.record_count > 0U && decision.mq >= 1;
    decision.effective_gates["G5"] =
        decision.effective_gates["G5"] && measurement.carbon_kgco2e > 0.0;
    decision.effective_gates["G11"] =
        decision.effective_gates["G11"] &&
        controlBool(input, "offsets_reported_separately");

    const double largest =
        parseNumber(control(input, "largest_unaddressed_omission_percent").value,
                    "largest_unaddressed_omission_percent");
    const double cumulative =
        parseNumber(control(input, "cumulative_omitted_percent").value,
                    "cumulative_omitted_percent");
    require(largest >= 0.0 && largest <= 100.0 &&
            cumulative >= 0.0 && cumulative <= 100.0,
            "materiality omission controls must be in [0,100]");
    decision.effective_gates["G14"] =
        decision.effective_gates["G14"] &&
        largest < 1.0 && cumulative <= 5.0 + kEpsilon;

    double weighted = 0.0;
    for (const auto& entry : domains)
        weighted += entry.second.weighted_points;
    const double penalty =
        parseNumber(control(input, "penalty_points").value,
                    "penalty_points");
    require(penalty >= 0.0 && penalty <= 100.0,
            "penalty_points must be in [0,100]");
    decision.total_score = std::max(0.0, weighted - penalty);
    require(decision.total_score <= 100.0 + 1e-7,
            "calculated score exceeds 100");
    decision.score_band = scoreBand(decision.total_score);
    decision.candidate_level = decision.score_band;

    for (const auto& entry : decision.effective_gates)
        if (!entry.second)
            decision.failed_gates.push_back(entry.first);
    decision.eligible = decision.failed_gates.empty();
    decision.score_valid = decision.eligible;
    if (!decision.eligible) {
        decision.candidate_level = 0;
        addReason(decision, "mandatory_gate_failure_invalidates_level");
    }

    if (decision.eligible && decision.candidate_level >= 2 &&
        (decision.mq < 1 || decision.dq < 1 ||
         decision.uncertainty > 30.0 + kEpsilon ||
         domainPercent(domains,'A') + kEpsilon < 40.0 ||
         domainPercent(domains,'B') + kEpsilon < 35.0 ||
         domainPercent(domains,'K') + kEpsilon < 40.0 ||
         !allApplicableDomainsAtLeast(domains,30.0)))
        capLevel(decision,1,"bronze_floor_not_met");

    if (decision.eligible && decision.candidate_level >= 3 &&
        (decision.mq < 2 || decision.dq < 2 ||
         decision.uncertainty > 20.0 + kEpsilon ||
         domainPercent(domains,'A') + kEpsilon < 50.0 ||
         domainPercent(domains,'B') + kEpsilon < 50.0 ||
         domainPercent(domains,'K') + kEpsilon < 50.0 ||
         !controlBool(input,"eco_regression_control_defined")))
        capLevel(decision,2,"silver_floor_not_met");

    if (decision.eligible && decision.candidate_level >= 4 &&
        (decision.mq < 3 || decision.dq < 3 ||
         decision.uncertainty > 12.0 + kEpsilon ||
         domainPercent(domains,'A') + kEpsilon < 65.0 ||
         domainPercent(domains,'B') + kEpsilon < 65.0 ||
         domainPercent(domains,'K') + kEpsilon < 65.0 ||
         domainPercent(domains,'C') + kEpsilon < 60.0 ||
         domainPercent(domains,'D') + kEpsilon < 60.0 ||
         controlBool(input,"high_severity_claim_risk")))
        capLevel(decision,3,"gold_floor_not_met");

    if (decision.eligible && decision.candidate_level >= 5 &&
        (decision.mq < 3 || decision.dq < 3 ||
         decision.uncertainty > 8.0 + kEpsilon ||
         domainPercent(domains,'A') + kEpsilon < 80.0 ||
         domainPercent(domains,'B') + kEpsilon < 75.0 ||
         domainPercent(domains,'K') + kEpsilon < 80.0 ||
         !controlBool(input,"independent_review") ||
         !controlBool(input,"public_report") ||
         !controlBool(input,"continuous_telemetry")))
        capLevel(decision,4,"platinum_floor_not_met");

    if (decision.eligible && decision.candidate_level >= 6 &&
        (decision.mq < 4 || decision.dq < 4 ||
         decision.uncertainty > 5.0 + kEpsilon ||
         domainPercent(domains,'A') + kEpsilon < 90.0 ||
         domainPercent(domains,'B') + kEpsilon < 85.0 ||
         domainPercent(domains,'K') + kEpsilon < 90.0 ||
         !controlBool(input,"independent_review") ||
         !controlBool(input,"public_report") ||
         !controlBool(input,"continuous_telemetry") ||
         !controlBool(input,"public_evidence_summary") ||
         !controlBool(input,"registry_ready")))
        capLevel(decision,5,"diamond_floor_not_met");

    const std::string baseline =
        control(input,"baseline_energy_per_unit_j").value;
    if (baseline != "none") {
        decision.baseline_energy_per_unit_j =
            parseNumber(baseline,"baseline_energy_per_unit_j");
        require(decision.baseline_energy_per_unit_j > 0.0,
                "baseline_energy_per_unit_j must be none or positive");
        decision.baseline_present = true;
        decision.eco_regression_percent =
            (decision.current_energy_per_unit_j -
             decision.baseline_energy_per_unit_j) * 100.0 /
            decision.baseline_energy_per_unit_j;
        if (decision.eco_regression_percent > 10.0 + kEpsilon &&
            !controlBool(input,"eco_regression_justification_approved")) {
            decision.eco_regression_triggered = true;
            decision.corrective_action_required = true;
            capLevel(decision,2,
                     "unexplained_eco_regression_above_10_percent");
        }
    } else {
        require(!controlBool(input,"eco_regression_justification_approved"),
                "eco_regression_justification_approved cannot be true without a baseline");
    }

    if (decision.eligible) {
        for (const std::string& claim : requestedClaims(input)) {
            if (claimUnsupported(claim,input,profile,measurement,decision)) {
                decision.effective_gates["G10"] = false;
                decision.corrective_action_required = true;
                addReason(decision,
                          "requested_claim_not_supported:" + claim);
            }
        }
        if (!decision.effective_gates["G10"]) {
            if (std::find(decision.failed_gates.begin(),
                          decision.failed_gates.end(), "G10") ==
                decision.failed_gates.end())
                decision.failed_gates.push_back("G10");
            decision.eligible = false;
            decision.score_valid = false;
            decision.candidate_level = 0;
        }
    }
    return decision;
}

std::vector<ClaimDecision> evaluateClaims(const AssessmentInput& input,
                                          const ProfileInfo& profile,
                                          const MeasurementInfo& measurement,
                                          const Decision& decision) {
    std::vector<ClaimDecision> out;
    const std::map<std::string,int> ranks = {
        {"measured_baseline",1},{"certified_bronze",2},
        {"certified_silver",3},{"certified_gold",4},
        {"certified_platinum",5},{"certified_diamond",6}};
    for (const std::string& claim : requestedClaims(input)) {
        ClaimDecision item;
        item.requested = claim;
        if (claim == "none") {
            item.status = "no_public_claim_requested";
            item.safe_text = "No public C3-ECO claim was requested.";
        } else if (claim == "comparative") {
            item.status = "deferred_pr95";
            item.safe_text = "Comparative ShortHand-versus-Python energy or carbon superiority is not established by PR90; equivalent-workload measured qualification remains PR95.";
        } else if (claim == "net_positive") {
            item.status = "denied_not_a_c3eco_level";
            item.safe_text = "Net-positive or regenerative is not a C3-ECO certification level; avoided impact must remain separate.";
        } else if (claim == "renewable_cloud") {
            item.status = "supplementary_only";
            item.safe_text = "Renewable-cloud hosting may be disclosed separately but does not establish C3-ECO certification.";
        } else if (claim == "carbon_neutral") {
            const bool supported =
                controlBool(input,"full_boundary_legal_claim_support") &&
                controlBool(input,"offsets_reported_separately");
            item.status = supported
                ? "restricted_supplementary_external_review"
                : "denied_insufficient_support";
            item.safe_text = "Carbon-neutral wording is outside the core C3-ECO score; offsets and renewable instruments remain separate from the measured base footprint.";
        } else if (claim == "zero_carbon") {
            const bool supported =
                controlBool(input,"full_boundary_legal_claim_support") &&
                measurement.carbon_kgco2e <= kEpsilon;
            item.status = supported
                ? "restricted_external_legal_review"
                : "denied_insufficient_support";
            item.safe_text = "Zero-carbon wording requires full-boundary technical evidence and legal claims review; ShortHand does not authorize that claim.";
        } else if (claim == "green_ai") {
            const bool supported =
                profile.ai_ml &&
                controlBool(input,"green_ai_supporting_evidence") &&
                decision.eligible && decision.candidate_level >= 4;
            item.status = supported
                ? "candidate_for_external_review"
                : "denied_insufficient_ai_evidence";
            item.safe_text = "Green AI is restricted to the declared AI model role, lifecycle boundary, functional unit and exclusions, and remains subject to external review.";
        } else {
            const auto it = ranks.find(claim);
            require(it != ranks.end(),"internal unsupported claim mapping");
            const bool supported =
                decision.eligible && decision.score_valid &&
                decision.candidate_level >= it->second;
            item.status = supported
                ? "candidate_for_external_review"
                : "denied_level_or_gate_not_met";
            item.safe_text = supported
                ? "Candidate level evidence is ready for external certification review; official certification is not granted by ShortHand."
                : "The requested C3-ECO level is not supported by effective gates, score, evidence quality and caps.";
        }
        out.push_back(item);
    }
    return out;
}

std::string levelName(int rank) {
    switch (rank) {
        case 1: return "Candidate";
        case 2: return "Bronze";
        case 3: return "Silver";
        case 4: return "Gold";
        case 5: return "Platinum";
        case 6: return "Diamond";
        default: return "None";
    }
}

void writeAssessmentJson(const std::string& path,
                         const std::string& profilePath,
                         const std::string& measurementPath,
                         const std::string& assessmentPath,
                         const ProfileInfo& profile,
                         const MeasurementInfo& measurement,
                         const AssessmentInput& input,
                         const std::map<char, DomainScore>& domains,
                         const Decision& decision,
                         const std::vector<ClaimDecision>& claims) {
    std::ofstream out(path);
    require(static_cast<bool>(out),
            "cannot open assessment JSON output: " + path);
    out << std::setprecision(12);
    out << "{\n  \"schema\":\"shorthand.c3eco.assessment.v1\",\n"
        << "  \"assessment_id\":\""
        << jsonEscape(control(input,"assessment_id").value) << "\",\n"
        << "  \"standard_profile\":\"C3-ECO authority-review draft v0.6\",\n"
        << "  \"assessment_status\":\""
        << (!decision.eligible ? "insufficient_evidence" :
            (decision.candidate_level >= 2 ? "qualified_readiness" : "provisional"))
        << "\",\n"
        << "  \"official_certification_granted\":false,\n"
        << "  \"production_claim\":false,\n"
        << "  \"profile_contract\":\"shorthand.c3eco.profile.v2\",\n"
        << "  \"measurement_contract\":\"shorthand.c3eco.measurement_workbook.v1\",\n"
        << "  \"input_paths\":{\"profile\":\""
        << jsonEscape(profilePath) << "\",\"measurement\":\""
        << jsonEscape(measurementPath) << "\",\"assessment\":\""
        << jsonEscape(assessmentPath) << "\"},\n"
        << "  \"profile\":{\"software_class\":\""
        << jsonEscape(profile.software_class) << "\",\"ai_ml\":"
        << (profile.ai_ml ? "true" : "false")
        << ",\"functional_unit_denominator\":"
        << profile.functional_unit_denominator << "},\n"
        << "  \"measurement\":{\"record_count\":"
        << measurement.record_count << ",\"effective_mq\":" << decision.mq
        << ",\"effective_dq\":" << decision.dq
        << ",\"uncertainty_percent\":" << decision.uncertainty
        << ",\"facility_energy_kwh\":" << measurement.facility_energy_kwh
        << ",\"carbon_kgco2e\":" << measurement.carbon_kgco2e << "},\n"
        << "  \"eligibility\":{\"status\":\""
        << (decision.eligible ? "eligible" : "not_eligible")
        << "\",\"score_valid\":"
        << (decision.score_valid ? "true" : "false")
        << ",\"failed_gates\":[";
    for (std::size_t i = 0U; i < decision.failed_gates.size(); ++i) {
        if (i) out << ',';
        out << "\"" << jsonEscape(decision.failed_gates[i]) << "\"";
    }
    out << "]},\n  \"gates\":[";
    bool first = true;
    for (const auto& entry : input.gates) {
        if (!first) out << ',';
        first = false;
        out << "\n    {\"id\":\"" << entry.first
            << "\",\"declared_pass\":"
            << (entry.second.declared_pass ? "true" : "false")
            << ",\"effective_pass\":"
            << (decision.effective_gates.at(entry.first) ? "true" : "false")
            << ",\"evidence_status\":\""
            << jsonEscape(entry.second.evidence_status)
            << "\",\"evidence_ref\":\""
            << jsonEscape(entry.second.evidence_ref) << "\"}";
    }
    out << "\n  ],\n  \"domains\":[";
    first = true;
    for (const auto& entry : domains) {
        if (!first) out << ',';
        first = false;
        const DomainScore& domain = entry.second;
        out << "\n    {\"id\":\"" << domain.id
            << "\",\"base_weight\":" << domain.base_weight
            << ",\"effective_weight\":" << domain.effective_weight
            << ",\"applicable_count\":" << domain.applicable_count
            << ",\"na_count\":" << domain.na_count
            << ",\"percent\":" << domain.percent
            << ",\"weighted_points\":" << domain.weighted_points << "}";
    }
    out << "\n  ],\n  \"criteria\":[";
    first = true;
    for (const auto& entry : input.criteria) {
        if (!first) out << ',';
        first = false;
        const Criterion& criterion = entry.second;
        out << "\n    {\"id\":\"" << entry.first
            << "\",\"applicable\":"
            << (criterion.applicable ? "true" : "false")
            << ",\"raw_score\":" << criterion.raw_score
            << ",\"effective_score\":" << criterion.effective_score
            << ",\"evidence_status\":\""
            << jsonEscape(criterion.evidence_status)
            << "\",\"evidence_ref\":\""
            << jsonEscape(criterion.evidence_ref) << "\"}";
    }
    out << "\n  ],\n  \"score\":{\"diagnostic_total\":"
        << decision.total_score << ",\"score_band\":\""
        << levelName(decision.score_band)
        << "\",\"candidate_level_after_caps\":\""
        << levelName(decision.candidate_level) << "\"},\n"
        << "  \"eco_regression\":{\"control_defined\":"
        << (controlBool(input,"eco_regression_control_defined") ? "true" : "false")
        << ",\"baseline_present\":"
        << (decision.baseline_present ? "true" : "false")
        << ",\"baseline_energy_per_unit_j\":"
        << decision.baseline_energy_per_unit_j
        << ",\"current_energy_per_unit_j\":"
        << decision.current_energy_per_unit_j
        << ",\"delta_percent\":" << decision.eco_regression_percent
        << ",\"triggered\":"
        << (decision.eco_regression_triggered ? "true" : "false") << "},\n"
        << "  \"corrective_action_required\":"
        << (decision.corrective_action_required ? "true" : "false")
        << ",\n  \"reasons\":[";
    for (std::size_t i = 0U; i < decision.reasons.size(); ++i) {
        if (i) out << ',';
        out << "\"" << jsonEscape(decision.reasons[i]) << "\"";
    }
    out << "],\n  \"claims\":[";
    first = true;
    for (const auto& claim : claims) {
        if (!first) out << ',';
        first = false;
        out << "\n    {\"requested\":\""
            << jsonEscape(claim.requested) << "\",\"status\":\""
            << jsonEscape(claim.status) << "\",\"safe_text\":\""
            << jsonEscape(claim.safe_text) << "\"}";
    }
    out << "\n  ],\n"
        << "  \"claim_safe_text\":\"Assessment and certification-readiness evidence only. ShortHand does not issue C3-ECO certificates, does not imply authority adoption, and does not establish comparative energy superiority in PR90.\",\n"
        << "  \"next_qualification\":\"PR91 supplies retained auditor bundles and authority-ready replay; PR95 supplies equivalent-workload ShortHand-versus-Python measured energy evidence.\"\n}\n";
}

void writeAssessmentMarkdown(const std::string& path,
                             const ProfileInfo& profile,
                             const MeasurementInfo& measurement,
                             const std::map<char, DomainScore>& domains,
                             const Decision& decision,
                             const std::vector<ClaimDecision>& claims) {
    std::ofstream out(path);
    require(static_cast<bool>(out),
            "cannot open assessment Markdown output: " + path);
    out << std::fixed << std::setprecision(2);
    out << "# C3-ECO assessment readiness report\n\n"
        << "This report is generated assessment evidence only. It does **not** grant C3-ECO certification.\n\n"
        << "- Software class: `" << profile.software_class << "`\n"
        << "- Mandatory eligibility: "
        << (decision.eligible ? "PASS" : "FAIL") << "\n"
        << "- Diagnostic weighted score: " << decision.total_score << "/100\n"
        << "- Score band: " << levelName(decision.score_band) << "\n"
        << "- Candidate level after gates/evidence/caps: "
        << levelName(decision.candidate_level) << "\n"
        << "- Effective MQ/DQ: MQ" << decision.mq << "/DQ" << decision.dq << "\n"
        << "- Maximum propagated measurement uncertainty: "
        << decision.uncertainty << "%\n"
        << "- Facility energy in source workbook: "
        << measurement.facility_energy_kwh << " kWh\n"
        << "- Carbon in source workbook: "
        << measurement.carbon_kgco2e << " kgCO2e\n\n"
        << "## Domain scores\n\n"
        << "| Domain | Effective weight | Score | Weighted points |\n"
        << "| --- | ---: | ---: | ---: |\n";
    for (const auto& entry : domains) {
        const DomainScore& domain = entry.second;
        out << "| " << domain.id << " | " << domain.effective_weight
            << " | " << domain.percent << "% | "
            << domain.weighted_points << " |\n";
    }
    out << "\n## Claims\n\n";
    for (const auto& claim : claims)
        out << "- `" << claim.requested << "`: **" << claim.status
            << "**. " << claim.safe_text << "\n";
    out << "\n## Decision reasons\n\n";
    if (decision.reasons.empty())
        out << "No downgrade or corrective-action reason was triggered.\n";
    else
        for (const auto& reason : decision.reasons)
            out << "- `" << reason << "`\n";
    out << "\nOfficial certification granted: **false**. External certification authority review remains separate.\n";
}

}  // namespace shorthand::c3eco
