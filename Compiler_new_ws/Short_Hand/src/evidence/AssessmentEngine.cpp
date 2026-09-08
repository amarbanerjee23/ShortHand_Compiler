#include "C3EcoAssessment.h"

#include <iomanip>
#include <iostream>
#include <stdexcept>

int main(int argc, char** argv) {
    using namespace shorthand::c3eco;
    if (argc != 6) {
        std::cerr << "usage: shorthand_c3eco_assess <profile.json> <measurement.json> <assessment.tsv> <output.json> <output.md>\n";
        return 64;
    }
    try {
        const ProfileInfo profile = validateProfile(argv[1]);
        const MeasurementInfo measurement = validateMeasurement(argv[2]);
        const AssessmentInput input = loadAssessment(argv[3]);
        if (!validIdentifierValue(control(input, "assessment_id").value))
            throw std::runtime_error("control assessment_id must be a bounded identifier");
        const auto domains = calculateDomains(input, profile.ai_ml);
        const Decision decision = decide(input, profile, measurement, domains);
        const auto claims = evaluateClaims(input, profile, measurement, decision);
        writeAssessmentJson(argv[4], argv[1], argv[2], argv[3], profile, measurement, input, domains, decision, claims);
        writeAssessmentMarkdown(argv[5], profile, measurement, domains, decision, claims);
        std::cout << "PASS: shorthand.c3eco.assessment.v1 eligibility="
                  << (decision.eligible ? "pass" : "fail") << " score=" << std::setprecision(6)
                  << decision.total_score << " candidate_level=" << levelName(decision.candidate_level) << "\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "FAIL: " << e.what() << "\n";
        return 2;
    }
}
