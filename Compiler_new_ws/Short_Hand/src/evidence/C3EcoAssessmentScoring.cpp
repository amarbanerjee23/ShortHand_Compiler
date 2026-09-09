#include "C3EcoAssessment.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace shorthand::c3eco {
namespace {
constexpr double kEpsilon = 1e-9;
void require(bool condition, const std::string &message) {
    if (!condition)
        throw std::runtime_error(message);
}
} // namespace

const std::map<char, double> &normativeDomainWeights() {
    static const std::map<char, double> weights = {
        {'A', 12.0}, {'B', 12.0}, {'C', 12.0}, {'D', 12.0}, {'E', 8.0}, {'F', 10.0},
        {'G', 10.0}, {'H', 6.0},  {'I', 6.0},  {'J', 4.0},  {'K', 8.0}};
    return weights;
}

std::vector<DomainScore> scoreDomains(const std::vector<Criterion> &criteria) {
    const auto &weights = normativeDomainWeights();
    double applicableNormativeWeight = 0.0;
    std::vector<DomainScore> domains;
    for (char domain = 'A'; domain <= 'K'; ++domain) {
        double numerator = 0.0;
        double denominatorWeight = 0.0;
        std::size_t count = 0;
        for (const Criterion &criterion : criteria) {
            if (criterion.domain == domain && criterion.applicable) {
                numerator += static_cast<double>(criterion.effectiveScore) * criterion.weight;
                denominatorWeight += criterion.weight;
                ++count;
            }
        }
        DomainScore score;
        score.domain = domain;
        score.normativeWeight = weights.at(domain);
        score.applicable = count != 0;
        score.criterionCount = count;
        if (score.applicable) {
            score.percent = 100.0 * numerator / (5.0 * denominatorWeight);
            applicableNormativeWeight += score.normativeWeight;
        }
        domains.push_back(score);
    }
    require(applicableNormativeWeight > 0.0, "at least one scoring domain must be applicable");
    for (DomainScore &domain : domains) {
        if (domain.applicable) {
            domain.adjustedWeight = domain.normativeWeight * 100.0 / applicableNormativeWeight;
            domain.points = domain.adjustedWeight * domain.percent / 100.0;
        }
    }
    return domains;
}

std::string levelName(Level level) {
    switch (level) {
    case Level::Candidate:
        return "candidate";
    case Level::Bronze:
        return "bronze";
    case Level::Silver:
        return "silver";
    case Level::Gold:
        return "gold";
    case Level::Platinum:
        return "platinum";
    case Level::Diamond:
        return "diamond";
    case Level::None:
        return "none";
    }
    return "none";
}

Level scoreBand(double total) {
    if (total + kEpsilon >= 95.0)
        return Level::Diamond;
    if (total + kEpsilon >= 90.0)
        return Level::Platinum;
    if (total + kEpsilon >= 80.0)
        return Level::Gold;
    if (total + kEpsilon >= 65.0)
        return Level::Silver;
    if (total + kEpsilon >= 50.0)
        return Level::Bronze;
    if (total + kEpsilon >= 40.0)
        return Level::Candidate;
    return Level::None;
}

double domainPercentOrZero(const std::vector<DomainScore> &domains, char id) {
    const auto found =
        std::find_if(domains.begin(), domains.end(),
                     [id](const DomainScore &domain) { return domain.domain == id; });
    require(found != domains.end(), std::string("scoring domain is unavailable: ") + id);
    return found->applicable ? found->percent : 0.0;
}

bool allApplicableDomainsAtLeast(const std::vector<DomainScore> &domains, double minimum) {
    return std::all_of(domains.begin(), domains.end(), [minimum](const DomainScore &domain) {
        return !domain.applicable || domain.percent + kEpsilon >= minimum;
    });
}

AssessmentDecision decide(const Metadata &metadata, std::map<std::string, Gate> &gates,
                          const std::vector<Criterion> &criteria,
                          const std::vector<DomainScore> &domains,
                          const MaterialityResult &materiality,
                          const std::vector<Regression> &regressions) {
    AssessmentDecision decision;
    for (const Criterion &criterion : criteria) {
        if (criterion.applicable && criterion.effectiveScore != criterion.rawScore) {
            decision.evidenceCaps.push_back(criterion.id);
        }
    }
    for (const Regression &regression : regressions) {
        if (regression.kind == "eco" && regression.thresholdExceeded)
            decision.ecoRegressionTriggered = true;
        if (regression.kind == "eco" && regression.unresolved)
            decision.unresolvedEcoRegression = true;
        if (regression.kind == "quality" && regression.thresholdExceeded)
            decision.qualityRegression = true;
    }
    if (decision.qualityRegression)
        gates.at("G13").effectivePass = false;
    if (materiality.individualMaterialOmission || materiality.cumulativeOmissionExceeded) {
        gates.at("G14").effectivePass = false;
    }
    for (int number = 1; number <= 14; ++number) {
        const std::string id = "G" + std::to_string(number);
        if (!gates.at(id).effectivePass)
            decision.failedGates.push_back(id);
    }
    decision.eligible = decision.failedGates.empty();
    for (const DomainScore &domain : domains)
        decision.totalScore += domain.points;
    if (std::abs(decision.totalScore) < kEpsilon)
        decision.totalScore = 0.0;
    decision.totalScore = std::max(0.0, decision.totalScore - metadata.penaltyPoints);
    decision.scoreCeiling = scoreBand(decision.totalScore);

    const double a = domainPercentOrZero(domains, 'A');
    const double b = domainPercentOrZero(domains, 'B');
    const double c = domainPercentOrZero(domains, 'C');
    const double d = domainPercentOrZero(domains, 'D');
    const double k = domainPercentOrZero(domains, 'K');
    const bool domainFloor = allApplicableDomainsAtLeast(domains, 30.0);
    const bool bronze = decision.eligible && decision.totalScore + kEpsilon >= 50.0 &&
                        domainFloor && a + kEpsilon >= 40.0 && b + kEpsilon >= 35.0 &&
                        k + kEpsilon >= 40.0 && metadata.uncertaintyPercent <= 30.0 + kEpsilon;
    const bool silver = bronze && decision.totalScore + kEpsilon >= 65.0 && a + kEpsilon >= 50.0 &&
                        b + kEpsilon >= 50.0 && k + kEpsilon >= 50.0 &&
                        metadata.measurementQuality >= 2 && metadata.dataQuality >= 2 &&
                        metadata.ecoRegressionControlDefined &&
                        metadata.uncertaintyPercent <= 20.0 + kEpsilon;
    const bool gold = silver && decision.totalScore + kEpsilon >= 80.0 && a + kEpsilon >= 65.0 &&
                      b + kEpsilon >= 65.0 && k + kEpsilon >= 65.0 && c + kEpsilon >= 60.0 &&
                      d + kEpsilon >= 60.0 && metadata.measurementQuality >= 3 &&
                      metadata.dataQuality >= 3 && !metadata.highSeverityClaimRisk &&
                      metadata.uncertaintyPercent <= 12.0 + kEpsilon &&
                      (!metadata.aiInScope || metadata.qualityEnergyFrontierOptions >= 2);
    const bool platinum = gold && decision.totalScore + kEpsilon >= 90.0 && a + kEpsilon >= 80.0 &&
                          b + kEpsilon >= 75.0 && k + kEpsilon >= 80.0 &&
                          metadata.independentReview && metadata.publicReport &&
                          metadata.continuousTelemetry &&
                          metadata.uncertaintyPercent <= 8.0 + kEpsilon;
    const bool diamond = platinum && decision.totalScore + kEpsilon >= 95.0 &&
                         a + kEpsilon >= 90.0 && b + kEpsilon >= 85.0 && k + kEpsilon >= 90.0 &&
                         metadata.measurementQuality >= 4 && metadata.dataQuality >= 4 &&
                         metadata.continuousTelemetry && metadata.publicEvidence &&
                         metadata.uncertaintyPercent <= 5.0 + kEpsilon;

    if (diamond)
        decision.recommendation = Level::Diamond;
    else if (platinum)
        decision.recommendation = Level::Platinum;
    else if (gold)
        decision.recommendation = Level::Gold;
    else if (silver)
        decision.recommendation = Level::Silver;
    else if (bronze)
        decision.recommendation = Level::Bronze;
    else if (decision.eligible && decision.totalScore + kEpsilon >= 40.0) {
        decision.recommendation = Level::Candidate;
    }

    if (decision.unresolvedEcoRegression &&
        static_cast<int>(decision.recommendation) > static_cast<int>(Level::Bronze)) {
        decision.recommendation = bronze ? Level::Bronze : Level::Candidate;
        decision.decisionReasons.push_back(
            "unresolved_eco_regression_caps_recommendation_at_bronze");
    }
    if (!decision.eligible)
        decision.decisionReasons.push_back("mandatory_gate_failure_precedes_scoring");
    if (decision.recommendation != decision.scoreCeiling && decision.eligible &&
        decision.decisionReasons.empty()) {
        decision.decisionReasons.push_back("level_prerequisites_lower_than_score_band");
    }
    if (!decision.evidenceCaps.empty()) {
        decision.decisionReasons.push_back("insufficient_evidence_caps_criteria_at_one");
    }
    if (decision.decisionReasons.empty())
        decision.decisionReasons.push_back("all_recommendation_prerequisites_met");
    return decision;
}

std::string lowerAscii(std::string value) {
    for (char &c : value) {
        if (c >= 'A' && c <= 'Z')
            c = static_cast<char>(c - 'A' + 'a');
    }
    return value;
}

bool containsAny(const std::string &value, const std::vector<std::string> &needles) {
    return std::any_of(needles.begin(), needles.end(), [&](const std::string &needle) {
        return value.find(needle) != std::string::npos;
    });
}

void evaluateClaims(std::vector<Claim> &claims, const Metadata &metadata,
                    const AssessmentDecision &decision) {
    for (Claim &claim : claims) {
        claim.reasons.clear();
        const std::string normalizedText = lowerAscii(claim.text);
        if (!claim.scopeMatches)
            claim.reasons.push_back("claim_scope_does_not_match_assessment");
        if (metadata.highSeverityClaimRisk && claim.type != "candidate_assessment") {
            claim.reasons.push_back("high_severity_claim_risk");
        }
        if (claim.type == "certified" || claim.type == "level") {
            claim.reasons.push_back("external_certification_authority_required");
        } else if (claim.type == "offsets_only") {
            claim.reasons.push_back("offsets_cannot_replace_base_footprint_reduction");
        } else if (claim.type == "candidate_assessment") {
            const std::vector<std::string> unsafeCandidateTerms = {
                "official",       "certified",        "bronze",       "silver",
                "gold",           "platinum",         "diamond",      "less energy",
                "more efficient", "most efficient",   "best",         "zero carbon",
                "carbon neutral", "climate positive", "net positive", "offset"};
            if (normalizedText.rfind("candidate ", 0) != 0 ||
                containsAny(normalizedText, unsafeCandidateTerms)) {
                claim.reasons.push_back("candidate_claim_text_is_not_claim_safe");
            }
        } else {
            if (!decision.eligible)
                claim.reasons.push_back("mandatory_gates_not_satisfied");
            if (claim.type == "comparative" || claim.type == "best") {
                claim.reasons.push_back("comparative_qualification_deferred_pr95");
                if (!claim.functionalUnitEquivalent)
                    claim.reasons.push_back("functional_unit_not_equivalent");
                if (!claim.boundaryEquivalent)
                    claim.reasons.push_back("boundary_not_equivalent");
                if (!claim.qualityEquivalent)
                    claim.reasons.push_back("quality_not_equivalent");
                if (!claim.methodEquivalent)
                    claim.reasons.push_back("method_not_equivalent");
            }
            if (claim.type == "green_ai") {
                if (static_cast<int>(decision.recommendation) < static_cast<int>(Level::Gold))
                    claim.reasons.push_back("green_ai_requires_gold_candidate_evidence");
                if (!metadata.aiInScope)
                    claim.reasons.push_back("AI_not_in_scope");
                if (metadata.aiRole == "not_applicable")
                    claim.reasons.push_back("AI_role_not_declared");
                if (!metadata.trainingBoundaryDeclared)
                    claim.reasons.push_back("training_boundary_not_declared");
                if (!metadata.inferenceBoundaryDeclared)
                    claim.reasons.push_back("inference_boundary_not_declared");
                if (!metadata.tokenOrInferenceMetricDeclared) {
                    claim.reasons.push_back("token_or_inference_metric_not_declared");
                }
                if (!metadata.aiExclusionsDeclared)
                    claim.reasons.push_back("AI_exclusions_not_declared");
            }
            if (claim.type == "zero" || claim.type == "best" || claim.type == "climate_positive" ||
                claim.type == "net_positive" || claim.type == "comparative") {
                if (!claim.legalTechnicalEvidence)
                    claim.reasons.push_back("legal_and_technical_evidence_required");
            }
            if (claim.type == "zero" || claim.type == "climate_positive" ||
                claim.type == "net_positive") {
                claim.reasons.push_back("restricted_claim_requires_external_review");
            }
        }
        claim.permitted = claim.reasons.empty();
    }
}

} // namespace shorthand::c3eco
