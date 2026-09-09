#pragma once

#include <cstddef>
#include <map>
#include <string>
#include <vector>

namespace shorthand::c3eco {

struct Metadata {
    std::string productName;
    std::string productVersion;
    std::string softwareClass;
    std::string functionalUnit;
    std::string boundary;
    std::string workload;
    int measurementQuality = 0;
    int dataQuality = 0;
    double uncertaintyPercent = 0.0;
    double penaltyPoints = 0.0;
    bool aiInScope = false;
    std::string aiRole;
    bool trainingBoundaryDeclared = false;
    bool inferenceBoundaryDeclared = false;
    bool tokenOrInferenceMetricDeclared = false;
    bool aiExclusionsDeclared = false;
    int qualityEnergyFrontierOptions = 0;
    bool clientDeviceImpact = false;
    bool ecoRegressionControlDefined = false;
    bool independentReview = false;
    bool publicReport = false;
    bool continuousTelemetry = false;
    bool publicEvidence = false;
    bool highSeverityClaimRisk = false;
    bool highScaleSaas = false;
};

struct Gate {
    std::string id;
    bool inputPass = false;
    bool effectivePass = false;
    std::string evidence;
};

struct Criterion {
    std::string id;
    char domain = 'A';
    int rawScore = 0;
    int effectiveScore = 0;
    double weight = 0.0;
    std::string evidenceStatus;
    bool applicable = true;
    std::string approval;
    std::string evidence;
};

struct MaterialityComponent {
    std::string component;
    double sharePercent = 0.0;
    std::string disposition;
    std::string evidence;
};

struct MaterialityResult {
    std::vector<MaterialityComponent> components;
    double declaredSharePercent = 0.0;
    double omittedSharePercent = 0.0;
    bool individualMaterialOmission = false;
    bool cumulativeOmissionExceeded = false;
};

struct Regression {
    std::string id;
    std::string kind;
    double baseline = 0.0;
    double current = 0.0;
    bool lowerIsBetter = true;
    std::string explanation;
    std::string correctiveAction;
    std::string evidence;
    double deteriorationPercent = 0.0;
    bool thresholdExceeded = false;
    bool unresolved = false;
};

struct Claim {
    std::string id;
    std::string type;
    std::string text;
    bool scopeMatches = false;
    bool functionalUnitEquivalent = false;
    bool boundaryEquivalent = false;
    bool qualityEquivalent = false;
    bool methodEquivalent = false;
    bool legalTechnicalEvidence = false;
    std::string evidence;
    bool permitted = false;
    std::vector<std::string> reasons;
};

struct DomainScore {
    char domain = 'A';
    double normativeWeight = 0.0;
    double adjustedWeight = 0.0;
    bool applicable = true;
    double percent = 0.0;
    double points = 0.0;
    std::size_t criterionCount = 0;
};

enum class Level {
    None = 0,
    Candidate = 1,
    Bronze = 2,
    Silver = 3,
    Gold = 4,
    Platinum = 5,
    Diamond = 6
};

struct AssessmentDecision {
    double totalScore = 0.0;
    Level scoreCeiling = Level::None;
    Level recommendation = Level::None;
    bool eligible = false;
    bool unresolvedEcoRegression = false;
    bool ecoRegressionTriggered = false;
    bool qualityRegression = false;
    std::vector<std::string> failedGates;
    std::vector<std::string> evidenceCaps;
    std::vector<std::string> decisionReasons;
};

std::vector<DomainScore> scoreDomains(const std::vector<Criterion> &criteria);
std::string levelName(Level level);
AssessmentDecision decide(const Metadata &metadata, std::map<std::string, Gate> &gates,
                          const std::vector<Criterion> &criteria,
                          const std::vector<DomainScore> &domains,
                          const MaterialityResult &materiality,
                          const std::vector<Regression> &regressions);
void evaluateClaims(std::vector<Claim> &claims, const Metadata &metadata,
                    const AssessmentDecision &decision);

} // namespace shorthand::c3eco
