#pragma once

#include <cstddef>
#include <map>
#include <string>
#include <vector>

namespace shorthand::c3eco {

struct Gate {
    bool declared_pass = false;
    bool effective_pass = false;
    std::string evidence_status;
    std::string evidence_ref;
};

struct Criterion {
    double raw_score = 0.0;
    double effective_score = 0.0;
    bool applicable = true;
    std::string evidence_status;
    std::string evidence_ref;
};

struct Control {
    std::string value;
    std::string evidence_status;
    std::string evidence_ref;
};

struct AssessmentInput {
    std::map<std::string, Gate> gates;
    std::map<std::string, Criterion> criteria;
    std::map<std::string, Control> controls;
};

struct ProfileInfo {
    bool ai_ml = false;
    std::string software_class;
    double functional_unit_denominator = 0.0;
};

struct MeasurementInfo {
    int mq = 0;
    int dq = 0;
    double uncertainty_percent = 0.0;
    double facility_energy_kwh = 0.0;
    double carbon_kgco2e = 0.0;
    std::size_t record_count = 0U;
};

struct DomainScore {
    char id = '?';
    double base_weight = 0.0;
    double effective_weight = 0.0;
    int applicable_count = 0;
    int na_count = 0;
    double score_sum = 0.0;
    double percent = 0.0;
    double weighted_points = 0.0;
};

struct Decision {
    bool eligible = false;
    bool score_valid = false;
    int score_band = 0;
    int candidate_level = 0;
    double total_score = 0.0;
    int mq = 0;
    int dq = 0;
    double uncertainty = 0.0;
    double current_energy_per_unit_j = 0.0;
    bool baseline_present = false;
    double baseline_energy_per_unit_j = 0.0;
    double eco_regression_percent = 0.0;
    bool eco_regression_triggered = false;
    bool corrective_action_required = false;
    std::vector<std::string> reasons;
    std::vector<std::string> failed_gates;
    std::map<std::string, bool> effective_gates;
};

struct ClaimDecision {
    std::string requested;
    std::string status;
    std::string safe_text;
};

AssessmentInput loadAssessment(const std::string& path);
ProfileInfo validateProfile(const std::string& path);
MeasurementInfo validateMeasurement(const std::string& path);
const Control& control(const AssessmentInput& input, const std::string& id);
bool controlBool(const AssessmentInput& input, const std::string& id);
bool validIdentifierValue(const std::string& value);

std::map<char, DomainScore> calculateDomains(const AssessmentInput& input, bool aiMl);
Decision decide(const AssessmentInput& input, const ProfileInfo& profile,
                const MeasurementInfo& measurement,
                const std::map<char, DomainScore>& domains);
std::vector<ClaimDecision> evaluateClaims(const AssessmentInput& input,
                                          const ProfileInfo& profile,
                                          const MeasurementInfo& measurement,
                                          const Decision& decision);
std::string levelName(int rank);
void writeAssessmentJson(const std::string& path,
                         const std::string& profilePath,
                         const std::string& measurementPath,
                         const std::string& assessmentPath,
                         const ProfileInfo& profile,
                         const MeasurementInfo& measurement,
                         const AssessmentInput& input,
                         const std::map<char, DomainScore>& domains,
                         const Decision& decision,
                         const std::vector<ClaimDecision>& claims);

}  // namespace shorthand::c3eco
