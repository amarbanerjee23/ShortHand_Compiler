#include "C3EcoAssessment.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>

using namespace shorthand::c3eco;

namespace {
void check(bool condition, const char *message) {
    if (!condition)
        throw std::runtime_error(message);
}

struct Fixture {
    Metadata metadata;
    std::map<std::string, Gate> gates;
    std::vector<Criterion> criteria;
    MaterialityResult materiality;
    std::vector<Regression> regressions;

    Fixture() {
        metadata.measurementQuality = 4;
        metadata.dataQuality = 4;
        metadata.uncertaintyPercent = 5;
        metadata.aiInScope = true;
        metadata.aiRole = "application_deployer";
        metadata.trainingBoundaryDeclared = true;
        metadata.inferenceBoundaryDeclared = true;
        metadata.tokenOrInferenceMetricDeclared = true;
        metadata.aiExclusionsDeclared = true;
        metadata.qualityEnergyFrontierOptions = 2;
        metadata.ecoRegressionControlDefined = true;
        metadata.independentReview = true;
        metadata.publicReport = true;
        metadata.continuousTelemetry = true;
        metadata.publicEvidence = true;
        for (int i = 1; i <= 14; ++i) {
            const auto id = "G" + std::to_string(i);
            gates.emplace(id, Gate{id, true, true, "evidence/gate"});
        }
        const std::map<char, int> counts = {{'A', 8}, {'B', 8}, {'C', 8},  {'D', 8},
                                            {'E', 6}, {'F', 7}, {'G', 10}, {'H', 5},
                                            {'I', 5}, {'J', 4}, {'K', 7}};
        for (const auto &domain : counts) {
            for (int i = 1; i <= domain.second; ++i) {
                Criterion c;
                c.id = std::string(1, domain.first) + std::to_string(i);
                c.domain = domain.first;
                c.rawScore = c.effectiveScore = 5;
                c.weight = 1;
                criteria.push_back(c);
            }
        }
    }

    AssessmentDecision assess() {
        return decide(metadata, gates, criteria, scoreDomains(criteria), materiality, regressions);
    }
};
} // namespace

int main() {
    try {
        Fixture baseline;
        auto decision = baseline.assess();
        check(decision.eligible && decision.recommendation == Level::Diamond &&
                  std::abs(decision.totalScore - 100) < 1e-8,
              "complete evidence must reach Diamond candidate");
        for (int i = 1; i <= 14; ++i) {
            Fixture failed;
            failed.gates.at("G" + std::to_string(i)).effectivePass = false;
            decision = failed.assess();
            check(!decision.eligible && decision.recommendation == Level::None &&
                      decision.totalScore == 100,
                  "each mandatory gate must override a perfect score");
        }
        Fixture dq;
        dq.metadata.dataQuality = 2;
        check(dq.assess().recommendation == Level::Silver,
              "DQ2 must cap a perfect score at Silver");
        Fixture uncertainty;
        uncertainty.metadata.uncertaintyPercent = 30.01;
        check(uncertainty.assess().recommendation == Level::Candidate,
              "uncertainty above Bronze ceiling must retain only Candidate");
        Fixture penalty;
        penalty.metadata.penaltyPoints = 60;
        check(penalty.assess().recommendation == Level::Candidate,
              "penalties must precede tier selection");
        penalty.metadata.penaltyPoints = 61;
        check(penalty.assess().recommendation == Level::None,
              "penalty can remove all candidate levels");
        Fixture telemetry;
        telemetry.metadata.continuousTelemetry = false;
        check(telemetry.assess().recommendation == Level::Gold,
              "Platinum requires continuous telemetry");
        Fixture materiality;
        materiality.materiality.individualMaterialOmission = true;
        check(!materiality.assess().eligible && !materiality.gates.at("G14").effectivePass,
              "derived materiality failure must invalidate G14");
        Fixture regression;
        Regression eco;
        eco.kind = "eco";
        eco.thresholdExceeded = eco.unresolved = true;
        regression.regressions.push_back(eco);
        check(regression.assess().recommendation == Level::Bronze,
              "unresolved eco regression caps at Bronze");
        regression.regressions.front().kind = "quality";
        check(!regression.assess().eligible && !regression.gates.at("G13").effectivePass,
              "quality degradation must fail G13");
        Fixture na;
        for (auto &criterion : na.criteria)
            if (criterion.domain == 'G')
                criterion.applicable = false;
        const auto domains = scoreDomains(na.criteria);
        double totalWeight = 0;
        for (const auto &domain : domains) {
            totalWeight += domain.adjustedWeight;
            if (domain.domain == 'G')
                check(domain.adjustedWeight == 0, "N/A domain must receive no points");
        }
        check(std::abs(totalWeight - 100) < 1e-8, "N/A must conserve the total domain weight");
        Claim claim;
        claim.id = "request";
        claim.type = "comparative";
        claim.text = "Uses less energy than Python.";
        claim.scopeMatches = claim.functionalUnitEquivalent = claim.boundaryEquivalent = true;
        claim.qualityEquivalent = claim.methodEquivalent = claim.legalTechnicalEvidence = true;
        std::vector<Claim> claims{claim};
        evaluateClaims(claims, baseline.metadata, baseline.assess());
        check(!claims.front().permitted, "PR90 cannot authorize comparative superiority");
        claims.front().type = "candidate_assessment";
        claims.front().text = "Candidate officially certified Diamond.";
        evaluateClaims(claims, baseline.metadata, baseline.assess());
        check(!claims.front().permitted, "candidate label cannot disguise certification wording");
        claims.front().type = "green_ai";
        claims.front().text = "Green AI evidence within the declared scope.";
        evaluateClaims(claims, baseline.metadata, baseline.assess());
        check(claims.front().permitted,
              "Gold-or-higher scoped AI evidence can reach external review");
        evaluateClaims(claims, dq.metadata, dq.assess());
        check(!claims.front().permitted, "Green AI requires at least Gold candidate evidence");
        std::cout << "PASS: PR90 independent scoring and claim-control unit\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
