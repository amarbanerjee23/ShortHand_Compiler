#include "C3EcoAssessment.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace shorthand::c3eco {
namespace {

constexpr const char* kAssessmentSchema = "shorthand.c3eco.assessment.v1";
constexpr std::size_t kMaxInputBytes = 4U * 1024U * 1024U;
constexpr std::size_t kMaxTsvLineBytes = 16384U;
constexpr double kEpsilon = 1e-9;

const std::map<char, double> kDomainWeights = {
    {'A', 12.0}, {'B', 12.0}, {'C', 12.0}, {'D', 12.0}, {'E', 8.0}, {'F', 10.0},
    {'G', 10.0}, {'H', 6.0}, {'I', 6.0}, {'J', 4.0}, {'K', 8.0}};

const std::map<char, int> kCriterionCounts = {
    {'A', 8}, {'B', 8}, {'C', 8}, {'D', 8}, {'E', 6}, {'F', 7},
    {'G', 10}, {'H', 5}, {'I', 5}, {'J', 4}, {'K', 7}};

const std::set<std::string> kGateIds = {
    "G1", "G2", "G3", "G4", "G5", "G6", "G7", "G8", "G9", "G10", "G11", "G12", "G13", "G14"};

const std::set<std::string> kRequiredControls = {
    "assessment_id", "requested_claims", "approved_na_domains", "cumulative_omitted_percent",
    "largest_unaddressed_omission_percent", "eco_regression_control_defined", "baseline_energy_per_unit_j",
    "eco_regression_justification_approved", "high_severity_claim_risk", "independent_review", "public_report",
    "continuous_telemetry", "public_evidence_summary", "registry_ready", "measurement_mq_override",
    "measurement_dq_override", "penalty_points", "full_boundary_legal_claim_support", "offsets_reported_separately",
    "green_ai_supporting_evidence"};

const std::set<std::string> kStrongEvidence = {"independent", "verified", "measured", "documented"};
const std::set<std::string> kCriterionEvidence = {
    "independent", "verified", "measured", "documented", "insufficient", "missing", "na"};

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

std::string trim(const std::string& value) {
    std::size_t begin = 0U;
    while (begin < value.size() && std::isspace(static_cast<unsigned char>(value[begin]))) ++begin;
    std::size_t end = value.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(value[end - 1U]))) --end;
    return value.substr(begin, end - begin);
}

std::vector<std::string> split(const std::string& text, char delimiter) {
    std::vector<std::string> result;
    std::string current;
    for (char c : text) {
        if (c == delimiter) {
            result.push_back(current);
            current.clear();
        } else {
            current.push_back(c);
        }
    }
    result.push_back(current);
    return result;
}

std::string readBounded(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    require(static_cast<bool>(in), "cannot open input: " + path);
    std::ostringstream buffer;
    char chunk[4096];
    std::size_t total = 0U;
    while (in) {
        in.read(chunk, sizeof(chunk));
        const std::streamsize count = in.gcount();
        if (count <= 0) break;
        total += static_cast<std::size_t>(count);
        require(total <= kMaxInputBytes, "input exceeds 4 MiB bound: " + path);
        for (std::streamsize i = 0; i < count; ++i) require(chunk[i] != '\0', "NUL byte in input: " + path);
        buffer.write(chunk, count);
    }
    const std::string out = buffer.str();
    require(!trim(out).empty(), "input is empty: " + path);
    return out;
}

void validateJsonShape(const std::string& text, const std::string& label) {
    const std::string compact = trim(text);
    require(compact.size() >= 2U && compact.front() == '{' && compact.back() == '}', label + " must be a JSON object");
    std::vector<char> stack;
    bool inString = false;
    bool escaped = false;
    for (char c : compact) {
        if (inString) {
            if (escaped) {
                escaped = false;
                continue;
            }
            if (c == '\\') escaped = true;
            else if (c == '"') inString = false;
            continue;
        }
        if (c == '"') inString = true;
        else if (c == '{' || c == '[') stack.push_back(c);
        else if (c == '}' || c == ']') {
            require(!stack.empty(), label + " has unmatched JSON delimiter");
            const char open = stack.back();
            stack.pop_back();
            require((open == '{' && c == '}') || (open == '[' && c == ']'), label + " has mismatched JSON delimiter");
        }
    }
    require(!inString && !escaped && stack.empty(), label + " is truncated or malformed JSON");
}

std::size_t skipWhitespace(const std::string& text, std::size_t pos) {
    while (pos < text.size() && std::isspace(static_cast<unsigned char>(text[pos]))) ++pos;
    return pos;
}

std::vector<std::size_t> keyValuePositions(const std::string& text, const std::string& key) {
    std::vector<std::size_t> positions;
    const std::string needle = "\"" + key + "\"";
    std::size_t pos = 0U;
    while ((pos = text.find(needle, pos)) != std::string::npos) {
        std::size_t colon = skipWhitespace(text, pos + needle.size());
        if (colon < text.size() && text[colon] == ':') positions.push_back(skipWhitespace(text, colon + 1U));
        pos += needle.size();
    }
    return positions;
}

std::string objectAt(const std::string& text, std::size_t pos) {
    require(pos < text.size() && text[pos] == '{', "expected JSON object value");
    int depth = 0;
    bool inString = false;
    bool escaped = false;
    for (std::size_t i = pos; i < text.size(); ++i) {
        const char c = text[i];
        if (inString) {
            if (escaped) escaped = false;
            else if (c == '\\') escaped = true;
            else if (c == '"') inString = false;
            continue;
        }
        if (c == '"') inString = true;
        else if (c == '{') ++depth;
        else if (c == '}') {
            --depth;
            if (depth == 0) return text.substr(pos, i - pos + 1U);
            require(depth >= 0, "malformed JSON object depth");
        }
    }
    throw std::runtime_error("truncated JSON object");
}

std::string objectForKeyFrom(const std::string& text, const std::string& key, std::size_t start) {
    const std::string needle = "\"" + key + "\"";
    const std::size_t keyPos = text.find(needle, start);
    require(keyPos != std::string::npos, "missing JSON object key: " + key);
    std::size_t colon = skipWhitespace(text, keyPos + needle.size());
    require(colon < text.size() && text[colon] == ':', "malformed JSON object key: " + key);
    const std::size_t valuePos = skipWhitespace(text, colon + 1U);
    return objectAt(text, valuePos);
}

std::string parseJsonStringAt(const std::string& text, std::size_t pos) {
    require(pos < text.size() && text[pos] == '"', "expected JSON string value");
    std::string out;
    bool escaped = false;
    for (std::size_t i = pos + 1U; i < text.size(); ++i) {
        const char c = text[i];
        if (escaped) {
            switch (c) {
                case '"': out.push_back('"'); break;
                case '\\': out.push_back('\\'); break;
                case '/': out.push_back('/'); break;
                case 'b': out.push_back('\b'); break;
                case 'f': out.push_back('\f'); break;
                case 'n': out.push_back('\n'); break;
                case 'r': out.push_back('\r'); break;
                case 't': out.push_back('\t'); break;
                default: throw std::runtime_error("unsupported JSON escape in generated evidence");
            }
            escaped = false;
        } else if (c == '\\') escaped = true;
        else if (c == '"') return out;
        else out.push_back(c);
    }
    throw std::runtime_error("unterminated JSON string");
}

double parseJsonNumberAt(const std::string& text, std::size_t pos) {
    std::size_t end = pos;
    while (end < text.size()) {
        const char c = text[end];
        if (!(std::isdigit(static_cast<unsigned char>(c)) || c == '-' || c == '+' || c == '.' || c == 'e' || c == 'E')) break;
        ++end;
    }
    require(end > pos, "expected JSON number value");
    const std::string token = text.substr(pos, end - pos);
    try {
        std::size_t used = 0U;
        const double value = std::stod(token, &used);
        require(used == token.size() && std::isfinite(value), "invalid JSON numeric value");
        return value;
    } catch (const std::exception&) {
        throw std::runtime_error("invalid JSON numeric value");
    }
}

bool parseJsonBoolAt(const std::string& text, std::size_t pos) {
    if (text.compare(pos, 4U, "true") == 0) return true;
    if (text.compare(pos, 5U, "false") == 0) return false;
    throw std::runtime_error("expected JSON boolean value");
}

std::string uniqueJsonString(const std::string& text, const std::string& key) {
    const auto positions = keyValuePositions(text, key);
    require(positions.size() == 1U, "expected exactly one JSON key: " + key);
    return parseJsonStringAt(text, positions.front());
}

bool uniqueJsonBool(const std::string& text, const std::string& key) {
    const auto positions = keyValuePositions(text, key);
    require(positions.size() == 1U, "expected exactly one JSON key: " + key);
    return parseJsonBoolAt(text, positions.front());
}

double uniqueJsonNumber(const std::string& text, const std::string& key) {
    const auto positions = keyValuePositions(text, key);
    require(positions.size() == 1U, "expected exactly one JSON key: " + key);
    return parseJsonNumberAt(text, positions.front());
}

std::vector<std::string> allJsonStrings(const std::string& text, const std::string& key) {
    std::vector<std::string> out;
    for (const std::size_t pos : keyValuePositions(text, key)) out.push_back(parseJsonStringAt(text, pos));
    return out;
}

std::vector<double> allJsonNumbers(const std::string& text, const std::string& key) {
    std::vector<double> out;
    for (const std::size_t pos : keyValuePositions(text, key)) out.push_back(parseJsonNumberAt(text, pos));
    return out;
}

bool parseBoolText(const std::string& text, const std::string& field) {
    if (text == "true") return true;
    if (text == "false") return false;
    throw std::runtime_error("control " + field + " must be true or false");
}

double parseDoubleText(const std::string& text, const std::string& field) {
    try {
        std::size_t used = 0U;
        const double value = std::stod(text, &used);
        require(used == text.size() && std::isfinite(value), "invalid numeric control");
        return value;
    } catch (const std::exception&) {
        throw std::runtime_error("control " + field + " must be a finite number");
    }
}

std::set<std::string> expectedCriteria() {
    std::set<std::string> ids;
    for (const auto& entry : kCriterionCounts)
        for (int i = 1; i <= entry.second; ++i) ids.insert(std::string(1U, entry.first) + std::to_string(i));
    return ids;
}

std::set<char> approvedNaDomains(const AssessmentInput& input) {
    const Control& c = control(input, "approved_na_domains");
    if (c.value == "none") return {};
    require(c.evidence_status == "independent" || c.evidence_status == "verified",
            "approved_na_domains requires verified or independent auditor evidence");
    std::set<char> out;
    for (const std::string& tokenRaw : split(c.value, ',')) {
        const std::string token = trim(tokenRaw);
        require(token.size() == 1U && kDomainWeights.count(token[0]) == 1U,
                "approved_na_domains contains invalid domain: " + token);
        require(out.insert(token[0]).second, "approved_na_domains contains duplicate domain: " + token);
    }
    return out;
}

std::vector<std::string> requestedClaims(const AssessmentInput& input) {
    const std::set<std::string> allowed = {
        "none", "measured_baseline", "certified_bronze", "certified_silver", "certified_gold",
        "certified_platinum", "certified_diamond", "green_ai", "zero_carbon", "carbon_neutral",
        "net_positive", "renewable_cloud"};
    std::vector<std::string> out;
    std::set<std::string> seen;
    for (const std::string& tokenRaw : split(control(input, "requested_claims").value, ';')) {
        const std::string token = trim(tokenRaw);
        require(allowed.count(token) == 1U, "unsupported requested_claims token: " + token);
        require(seen.insert(token).second, "duplicate requested_claims token: " + token);
        out.push_back(token);
    }
    require(!out.empty(), "requested_claims must not be empty");
    if (out.size() > 1U) require(seen.count("none") == 0U, "requested_claims none cannot be combined with another claim");
    std::sort(out.begin(), out.end());
    return out;
}

int qualityOverride(const AssessmentInput& input, const std::string& id, int measured) {
    const Control& c = control(input, id);
    if (c.value == "none") return measured;
    const double raw = parseDoubleText(c.value, id);
    require(std::floor(raw) == raw && raw >= 1.0 && raw <= 4.0, "control " + id + " must be none or integer 1-4");
    const int value = static_cast<int>(raw);
    if (value > measured) require(c.evidence_status == "independent", id + " may only raise evidence quality with independent evidence");
    return value;
}

int mqFromText(const std::string& value) {
    if (value == "high") return 3;
    if (value == "medium") return 2;
    if (value == "low") return 1;
    throw std::runtime_error("unknown measurement_quality in PR89 workbook: " + value);
}

int dqFromText(const std::string& value) {
    if (value == "high") return 3;
    if (value == "medium") return 2;
    if (value == "low") return 1;
    throw std::runtime_error("unknown data_quality in PR89 workbook: " + value);
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

void capLevel(Decision& decision, int cap, const std::string& reason) {
    if (decision.candidate_level > cap) {
        decision.candidate_level = cap;
        decision.reasons.push_back(reason);
    }
}

double domainPercent(const std::map<char, DomainScore>& domains, char id) {
    const auto it = domains.find(id);
    require(it != domains.end(), "missing scoring domain");
    return it->second.percent;
}

bool claimWouldViolateIntegrity(const std::string& claim,
                                const AssessmentInput& input,
                                const ProfileInfo& profile,
                                const MeasurementInfo& measurement,
                                int preliminaryLevel) {
    if (claim == "none" || claim == "renewable_cloud") return false;
    if (claim == "net_positive") return true;
    if (claim == "zero_carbon")
        return !controlBool(input, "full_boundary_legal_claim_support") || measurement.carbon_kgco2e > kEpsilon;
    if (claim == "carbon_neutral")
        return !controlBool(input, "full_boundary_legal_claim_support") || !controlBool(input, "offsets_reported_separately");
    if (claim == "green_ai")
        return !profile.ai_ml || !controlBool(input, "green_ai_supporting_evidence") || preliminaryLevel < 4;
    const std::map<std::string, int> ranks = {
        {"measured_baseline", 1}, {"certified_bronze", 2}, {"certified_silver", 3}, {"certified_gold", 4},
        {"certified_platinum", 5}, {"certified_diamond", 6}};
    const auto it = ranks.find(claim);
    return it != ranks.end() && preliminaryLevel < it->second;
}

std::string jsonEscape(const std::string& value) {
    std::string out;
    for (char c : value) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                require(static_cast<unsigned char>(c) >= 0x20U, "control character cannot be emitted in JSON");
                out.push_back(c);
        }
    }
    return out;
}

}  // namespace

AssessmentInput loadAssessment(const std::string& path) {
    const std::string text = readBounded(path);
    std::istringstream in(text);
    std::string line;
    require(static_cast<bool>(std::getline(in, line)), "assessment input is empty");
    if (!line.empty() && line.back() == '\r') line.pop_back();
    require(line == "kind\tid\tvalue\tapplicable\tevidence_status\tevidence_ref", "invalid PR90 assessment TSV header");

    AssessmentInput input;
    std::size_t lineNumber = 1U;
    const auto criteriaSet = expectedCriteria();
    while (std::getline(in, line)) {
        ++lineNumber;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        require(line.size() <= kMaxTsvLineBytes, "assessment TSV line exceeds bound at line " + std::to_string(lineNumber));
        if (trim(line).empty()) continue;
        const auto fields = split(line, '\t');
        require(fields.size() == 6U, "assessment TSV requires 6 columns at line " + std::to_string(lineNumber));
        const std::string kind = fields[0];
        const std::string id = fields[1];
        const std::string value = fields[2];
        const std::string applicableText = fields[3];
        const std::string evidenceStatus = fields[4];
        const std::string evidenceRef = fields[5];
        require(!id.empty() && id.size() <= 64U, "invalid assessment id at line " + std::to_string(lineNumber));
        require(value.size() <= 512U && evidenceRef.size() <= 1024U, "assessment field exceeds bound at line " + std::to_string(lineNumber));
        require(!evidenceRef.empty(), "evidence_ref is required at line " + std::to_string(lineNumber));
        const bool applicable = parseBoolText(applicableText, "applicable");

        if (kind == "gate") {
            require(kGateIds.count(id) == 1U, "unknown gate id: " + id);
            require(input.gates.count(id) == 0U, "duplicate gate id: " + id);
            require(applicable, "mandatory gate cannot be marked not applicable: " + id);
            require(value == "pass" || value == "fail", "gate value must be pass or fail: " + id);
            require(kStrongEvidence.count(evidenceStatus) == 1U, "gate evidence_status is not sufficient: " + id);
            input.gates.emplace(id, Gate{value == "pass", value == "pass", evidenceStatus, evidenceRef});
        } else if (kind == "criterion") {
            require(criteriaSet.count(id) == 1U, "unknown criterion id: " + id);
            require(input.criteria.count(id) == 0U, "duplicate criterion id: " + id);
            require(kCriterionEvidence.count(evidenceStatus) == 1U, "invalid criterion evidence_status: " + id);
            Criterion criterion;
            criterion.raw_score = parseDoubleText(value, id);
            require(criterion.raw_score >= 0.0 && criterion.raw_score <= 5.0, "criterion score must be in [0,5]: " + id);
            criterion.applicable = applicable;
            criterion.evidence_status = evidenceStatus;
            criterion.evidence_ref = evidenceRef;
            if (!applicable) {
                require(std::fabs(criterion.raw_score) <= kEpsilon && evidenceStatus == "na",
                        "not-applicable criterion must have score 0 and evidence_status na: " + id);
                criterion.effective_score = 0.0;
            } else {
                require(evidenceStatus != "na", "applicable criterion cannot use evidence_status na: " + id);
                if (evidenceStatus == "missing") criterion.effective_score = 0.0;
                else if (evidenceStatus == "insufficient") criterion.effective_score = std::min(criterion.raw_score, 1.0);
                else criterion.effective_score = criterion.raw_score;
            }
            input.criteria.emplace(id, criterion);
        } else if (kind == "control") {
            require(kRequiredControls.count(id) == 1U, "unknown control id: " + id);
            require(input.controls.count(id) == 0U, "duplicate control id: " + id);
            require(applicable, "control cannot be marked not applicable: " + id);
            require(kStrongEvidence.count(evidenceStatus) == 1U, "control evidence_status is not sufficient: " + id);
            input.controls.emplace(id, Control{value, evidenceStatus, evidenceRef});
        } else {
            throw std::runtime_error("unknown assessment row kind at line " + std::to_string(lineNumber));
        }
    }

    require(input.gates.size() == kGateIds.size(), "assessment must contain exactly G1-G14");
    for (const std::string& id : kGateIds) require(input.gates.count(id) == 1U, "assessment missing gate: " + id);
    require(input.criteria.size() == criteriaSet.size(), "assessment must contain the complete 76-criterion A-K catalog");
    for (const std::string& id : criteriaSet) require(input.criteria.count(id) == 1U, "assessment missing criterion: " + id);
    require(input.controls.size() == kRequiredControls.size(), "assessment must contain the complete PR90 control set");
    for (const std::string& id : kRequiredControls) require(input.controls.count(id) == 1U, "assessment missing control: " + id);
    requestedClaims(input);
    approvedNaDomains(input);
    return input;
}

ProfileInfo validateProfile(const std::string& path) {
    const std::string text = readBounded(path);
    validateJsonShape(text, "typed C3-ECO profile evidence");
    require(uniqueJsonString(text, "schema") == "shorthand.c3eco.candidate_report.v1", "profile evidence has wrong schema");
    require(!uniqueJsonBool(text, "official_certification_granted"), "profile evidence cannot already grant certification");
    require(uniqueJsonString(text, "c3eco_profile_contract") == "shorthand.c3eco.profile.v2", "profile evidence has wrong profile contract");
    require(uniqueJsonString(text, "c3eco_profile_status") == "conformant", "profile evidence must be conformant typed profile v2");

    const std::size_t certificationPos = text.find("\"kind\":\"certification\"");
    const std::size_t functionalUnitPos = text.find("\"kind\":\"functional_unit\"");
    require(certificationPos != std::string::npos, "profile evidence lacks certification declaration");
    require(functionalUnitPos != std::string::npos, "profile evidence lacks functional_unit declaration");
    require(text.find("\"kind\":\"certification_profile\"") != std::string::npos, "profile evidence lacks certification_profile declaration");
    require(text.find("\"kind\":\"boundary\"") != std::string::npos, "profile evidence lacks boundary declaration");
    require(text.find("\"kind\":\"guardrails\"") != std::string::npos, "profile evidence lacks guardrails declaration");

    const std::string certificationTyped = objectForKeyFrom(text, "typed_fields", certificationPos);
    const std::string functionalTyped = objectForKeyFrom(text, "typed_fields", functionalUnitPos);
    ProfileInfo info;
    info.software_class = uniqueJsonString(certificationTyped, "value");
    const auto denominatorValues = keyValuePositions(functionalTyped, "denominator");
    require(denominatorValues.size() == 1U, "functional-unit typed fields must contain exactly one denominator");
    const std::string denominatorArrayWindow = functionalTyped.substr(denominatorValues.front(), std::min<std::size_t>(512U, functionalTyped.size() - denominatorValues.front()));
    const auto denominatorScalar = keyValuePositions(denominatorArrayWindow, "value");
    require(!denominatorScalar.empty(), "functional-unit denominator has no typed value");
    info.functional_unit_denominator = parseJsonNumberAt(denominatorArrayWindow, denominatorScalar.front());
    require(info.functional_unit_denominator > 0.0 && std::isfinite(info.functional_unit_denominator),
            "functional-unit denominator must be positive");
    info.ai_ml = info.software_class.rfind("S6", 0U) == 0U || text.find("\"kind\":\"ai_lifecycle\"") != std::string::npos;
    return info;
}

MeasurementInfo validateMeasurement(const std::string& path) {
    const std::string text = readBounded(path);
    validateJsonShape(text, "PR89 measurement workbook");
    require(uniqueJsonString(text, "schema") == "shorthand.c3eco.measurement_workbook.v1", "measurement evidence has wrong schema");
    require(uniqueJsonString(text, "measurement_status") == "measured_instrumented", "measurement evidence is not instrumented");
    require(!uniqueJsonBool(text, "official_certification_granted"), "measurement workbook cannot already grant certification");
    require(uniqueJsonBool(text, "base_footprint_not_reduced_by_offsets"), "measurement workbook must keep offsets outside base footprint");

    MeasurementInfo info;
    const double countValue = uniqueJsonNumber(text, "record_count");
    require(countValue >= 1.0 && std::floor(countValue) == countValue, "measurement record_count must be a positive integer");
    info.record_count = static_cast<std::size_t>(countValue);
    const auto ids = allJsonStrings(text, "record_id");
    const auto sources = allJsonStrings(text, "source_kind");
    const auto mqs = allJsonStrings(text, "measurement_quality");
    const auto dqs = allJsonStrings(text, "data_quality");
    const auto uncertainties = allJsonNumbers(text, "uncertainty_percent");
    require(ids.size() == info.record_count && sources.size() == info.record_count && mqs.size() == info.record_count &&
                dqs.size() == info.record_count && uncertainties.size() == info.record_count,
            "measurement record_count does not match record evidence");
    const std::set<std::string> allowedSources = {"physical_meter", "rapl", "accelerator_counter", "cloud_meter"};
    info.mq = 4;
    info.dq = 4;
    for (std::size_t i = 0U; i < info.record_count; ++i) {
        require(allowedSources.count(sources[i]) == 1U, "measurement workbook contains non-instrumented source");
        info.mq = std::min(info.mq, mqFromText(mqs[i]));
        info.dq = std::min(info.dq, dqFromText(dqs[i]));
        require(uncertainties[i] >= 0.0 && uncertainties[i] <= 100.0, "measurement record uncertainty is outside [0,100]");
    }
    const std::string totals = objectForKeyFrom(text, "totals", 0U);
    info.facility_energy_kwh = uniqueJsonNumber(totals, "facility_energy_kwh");
    info.carbon_kgco2e = uniqueJsonNumber(totals, "carbon_kgco2e");
    const double uncertaintyKwh = uniqueJsonNumber(totals, "uncertainty_kwh");
    require(info.facility_energy_kwh > 0.0 && info.carbon_kgco2e > 0.0 && uncertaintyKwh >= 0.0,
            "measurement totals must be positive and complete");
    info.uncertainty_percent = (uncertaintyKwh / info.facility_energy_kwh) * 100.0;
    require(std::isfinite(info.uncertainty_percent) && info.uncertainty_percent >= 0.0 && info.uncertainty_percent <= 100.0,
            "aggregate measurement uncertainty is outside [0,100]");
    return info;
}

const Control& control(const AssessmentInput& input, const std::string& id) {
    const auto it = input.controls.find(id);
    if (it == input.controls.end()) throw std::runtime_error("missing assessment control: " + id);
    return it->second;
}

bool controlBool(const AssessmentInput& input, const std::string& id) {
    return parseBoolText(control(input, id).value, id);
}

bool validIdentifierValue(const std::string& value) {
    if (value.empty() || value.size() > 96U) return false;
    for (char c : value)
        if (!(std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == '.')) return false;
    return true;
}

std::map<char, DomainScore> calculateDomains(const AssessmentInput& input, bool aiMl) {
    const std::set<char> approved = approvedNaDomains(input);
    std::map<char, DomainScore> domains;
    double activeBaseWeight = 0.0;
    for (const auto& domainEntry : kCriterionCounts) {
        const char domainId = domainEntry.first;
        DomainScore score;
        score.id = domainId;
        score.base_weight = kDomainWeights.at(domainId);
        for (int i = 1; i <= domainEntry.second; ++i) {
            const Criterion& criterion = input.criteria.at(std::string(1U, domainId) + std::to_string(i));
            if (criterion.applicable) {
                ++score.applicable_count;
                score.score_sum += criterion.effective_score;
            } else {
                ++score.na_count;
            }
        }
        if (domainId == 'G' && !aiMl)
            require(score.applicable_count == 0, "AI domain G must be N/A when the typed profile has no AI subsystem");
        if (score.applicable_count == 0) {
            require(approved.count(domainId) == 1U, std::string("fully N/A domain requires approved_na_domains evidence: ") + domainId);
            if (domainId == 'G') require(!aiMl, "AI domain G cannot be reallocated while AI is in the certified boundary");
        } else {
            score.percent = (score.score_sum / (5.0 * static_cast<double>(score.applicable_count))) * 100.0;
            activeBaseWeight += score.base_weight;
        }
        domains.emplace(domainId, score);
    }
    require(activeBaseWeight > 0.0, "all scoring domains cannot be N/A");
    for (auto& entry : domains) {
        DomainScore& score = entry.second;
        if (score.applicable_count == 0) continue;
        score.effective_weight = score.base_weight * (100.0 / activeBaseWeight);
        score.weighted_points = score.effective_weight * (score.percent / 100.0);
    }
    return domains;
}

Decision decide(const AssessmentInput& input, const ProfileInfo& profile,
                const MeasurementInfo& measurement,
                const std::map<char, DomainScore>& domains) {
    Decision decision;
    decision.mq = qualityOverride(input, "measurement_mq_override", measurement.mq);
    decision.dq = qualityOverride(input, "measurement_dq_override", measurement.dq);
    decision.uncertainty = measurement.uncertainty_percent;
    decision.current_energy_per_unit_j = measurement.facility_energy_kwh * 3600000.0 / profile.functional_unit_denominator;
    for (const auto& entry : input.gates) decision.effective_gates[entry.first] = entry.second.effective_pass;
    decision.effective_gates["G4"] = decision.effective_gates["G4"] && measurement.record_count > 0U && decision.mq >= 1;
    decision.effective_gates["G5"] = decision.effective_gates["G5"] && measurement.carbon_kgco2e > 0.0;
    decision.effective_gates["G13"] = decision.effective_gates["G13"] && profile.functional_unit_denominator > 0.0;

    const double largestOmission = parseDoubleText(control(input, "largest_unaddressed_omission_percent").value,
                                                   "largest_unaddressed_omission_percent");
    const double cumulativeOmission = parseDoubleText(control(input, "cumulative_omitted_percent").value,
                                                      "cumulative_omitted_percent");
    require(largestOmission >= 0.0 && largestOmission <= 100.0 && cumulativeOmission >= 0.0 && cumulativeOmission <= 100.0,
            "materiality omission controls must be in [0,100]");
    decision.effective_gates["G14"] = decision.effective_gates["G14"] && largestOmission < 1.0 && cumulativeOmission <= 5.0 + kEpsilon;

    double total = 0.0;
    for (const auto& entry : domains) total += entry.second.weighted_points;
    const double penalty = parseDoubleText(control(input, "penalty_points").value, "penalty_points");
    require(penalty >= 0.0 && penalty <= 100.0, "penalty_points must be in [0,100]");
    decision.total_score = std::max(0.0, total - penalty);
    require(decision.total_score <= 100.0 + 1e-7, "calculated score exceeds 100");
    decision.score_band = scoreBand(decision.total_score);
    decision.candidate_level = decision.score_band;

    if (decision.candidate_level >= 2) {
        bool allDomainsThirty = true;
        for (const auto& entry : domains)
            if (entry.second.applicable_count > 0 && entry.second.percent + kEpsilon < 30.0) allDomainsThirty = false;
        if (decision.mq < 1 || decision.dq < 1 || decision.uncertainty > 30.0 + kEpsilon ||
            domainPercent(domains, 'A') + kEpsilon < 40.0 || domainPercent(domains, 'B') + kEpsilon < 35.0 ||
            domainPercent(domains, 'K') + kEpsilon < 40.0 || !allDomainsThirty)
            capLevel(decision, 1, "bronze_floor_not_met");
    }
    if (decision.candidate_level >= 3) {
        if (decision.mq < 2 || decision.dq < 2 || decision.uncertainty > 20.0 + kEpsilon ||
            domainPercent(domains, 'A') + kEpsilon < 50.0 || domainPercent(domains, 'B') + kEpsilon < 50.0 ||
            domainPercent(domains, 'K') + kEpsilon < 50.0 || !controlBool(input, "eco_regression_control_defined"))
            capLevel(decision, 2, "silver_floor_not_met");
    }
    if (decision.candidate_level >= 4) {
        if (decision.mq < 3 || decision.dq < 3 || decision.uncertainty > 12.0 + kEpsilon ||
            domainPercent(domains, 'A') + kEpsilon < 65.0 || domainPercent(domains, 'B') + kEpsilon < 65.0 ||
            domainPercent(domains, 'K') + kEpsilon < 65.0 || domainPercent(domains, 'C') + kEpsilon < 60.0 ||
            domainPercent(domains, 'D') + kEpsilon < 60.0 || controlBool(input, "high_severity_claim_risk"))
            capLevel(decision, 3, "gold_floor_not_met");
    }
    if (decision.candidate_level >= 5) {
        if (decision.mq < 3 || decision.dq < 3 || decision.uncertainty > 8.0 + kEpsilon ||
            domainPercent(domains, 'A') + kEpsilon < 80.0 || domainPercent(domains, 'B') + kEpsilon < 75.0 ||
            domainPercent(domains, 'K') + kEpsilon < 80.0 || !controlBool(input, "independent_review") ||
            !controlBool(input, "public_report"))
            capLevel(decision, 4, "platinum_floor_not_met");
    }
    if (decision.candidate_level >= 6) {
        if (decision.mq < 4 || decision.dq < 4 || decision.uncertainty > 5.0 + kEpsilon ||
            domainPercent(domains, 'A') + kEpsilon < 90.0 || domainPercent(domains, 'B') + kEpsilon < 85.0 ||
            domainPercent(domains, 'K') + kEpsilon < 90.0 || !controlBool(input, "independent_review") ||
            !controlBool(input, "continuous_telemetry") || !controlBool(input, "public_evidence_summary") ||
            !controlBool(input, "registry_ready"))
            capLevel(decision, 5, "diamond_floor_not_met");
    }

    const std::string baseline = control(input, "baseline_energy_per_unit_j").value;
    if (baseline != "none") {
        decision.baseline_energy_per_unit_j = parseDoubleText(baseline, "baseline_energy_per_unit_j");
        require(decision.baseline_energy_per_unit_j > 0.0, "baseline_energy_per_unit_j must be none or positive");
        decision.baseline_present = true;
        decision.eco_regression_percent =
            ((decision.current_energy_per_unit_j - decision.baseline_energy_per_unit_j) / decision.baseline_energy_per_unit_j) * 100.0;
        if (decision.eco_regression_percent > 10.0 + kEpsilon && !controlBool(input, "eco_regression_justification_approved")) {
            decision.eco_regression_triggered = true;
            decision.corrective_action_required = true;
            capLevel(decision, 2, "unexplained_eco_regression_above_10_percent");
        }
    } else {
        require(!controlBool(input, "eco_regression_justification_approved"),
                "eco_regression_justification_approved cannot be true without a baseline");
    }

    for (const std::string& claim : requestedClaims(input)) {
        if (claimWouldViolateIntegrity(claim, input, profile, measurement, decision.candidate_level)) {
            decision.effective_gates["G10"] = false;
            decision.corrective_action_required = true;
            decision.reasons.push_back("requested_claim_not_supported:" + claim);
        }
    }
    for (const auto& entry : decision.effective_gates)
        if (!entry.second) decision.failed_gates.push_back(entry.first);
    decision.eligible = decision.failed_gates.empty();
    decision.score_valid = decision.eligible;
    if (!decision.eligible) decision.candidate_level = 0;
    return decision;
}

std::vector<ClaimDecision> evaluateClaims(const AssessmentInput& input,
                                          const ProfileInfo& profile,
                                          const MeasurementInfo& measurement,
                                          const Decision& decision) {
    std::vector<ClaimDecision> out;
    const std::map<std::string, int> ranks = {
        {"measured_baseline", 1}, {"certified_bronze", 2}, {"certified_silver", 3}, {"certified_gold", 4},
        {"certified_platinum", 5}, {"certified_diamond", 6}};
    for (const std::string& claim : requestedClaims(input)) {
        ClaimDecision item;
        item.requested = claim;
        if (claim == "none") {
            item.status = "no_public_claim_requested";
            item.safe_text = "No public C3-ECO claim was requested.";
        } else if (claim == "net_positive") {
            item.status = "denied_not_a_c3eco_level";
            item.safe_text = "Net-positive or regenerative is not a C3-ECO certification level; avoided impact must remain separate.";
        } else if (claim == "renewable_cloud") {
            item.status = "supplementary_only";
            item.safe_text = "Renewable-cloud hosting may be disclosed separately but does not establish C3-ECO certification.";
        } else if (claim == "carbon_neutral") {
            const bool supported = decision.eligible && controlBool(input, "full_boundary_legal_claim_support") &&
                                   controlBool(input, "offsets_reported_separately");
            item.status = supported ? "restricted_supplementary_external_review" : "denied_insufficient_support";
            item.safe_text = "Carbon-neutral wording is outside the core C3-ECO score; offsets and renewable instruments remain separate from the measured base footprint.";
        } else if (claim == "zero_carbon") {
            const bool supported = decision.eligible && controlBool(input, "full_boundary_legal_claim_support") &&
                                   measurement.carbon_kgco2e <= kEpsilon;
            item.status = supported ? "restricted_external_legal_review" : "denied_insufficient_support";
            item.safe_text = "Zero-carbon wording requires full-boundary evidence and legal claims support; this assessment tool does not authorize that claim.";
        } else if (claim == "green_ai") {
            const bool supported = decision.eligible && profile.ai_ml && controlBool(input, "green_ai_supporting_evidence") &&
                                   decision.candidate_level >= 4;
            item.status = supported ? "candidate_for_external_review" : "denied_insufficient_ai_evidence";
            item.safe_text = "Green AI is a restricted claim and requires a measured AI boundary, model-role disclosure, supporting evidence and external review.";
        } else {
            const auto it = ranks.find(claim);
            require(it != ranks.end(), "internal unsupported claim mapping");
            const bool supported = decision.eligible && decision.score_valid && decision.candidate_level >= it->second;
            item.status = supported ? "candidate_for_external_review" : "denied_level_or_gate_not_met";
            item.safe_text = supported
                ? "Candidate level evidence is ready for external certification review; official certification is not granted by ShortHand."
                : "The requested C3-ECO level is not supported by the effective gates, score and caps.";
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
    require(static_cast<bool>(out), "cannot open assessment output: " + path);
    out << std::setprecision(12);
    out << "{\n"
        << "  \"schema\":\"" << kAssessmentSchema << "\",\n"
        << "  \"assessment_id\":\"" << jsonEscape(control(input, "assessment_id").value) << "\",\n"
        << "  \"profile_contract\":\"shorthand.c3eco.profile.v2\",\n"
        << "  \"measurement_contract\":\"shorthand.c3eco.measurement_workbook.v1\",\n"
        << "  \"official_certification_granted\":false,\n"
        << "  \"production_claim\":false,\n"
        << "  \"input_paths\":{\"profile\":\"" << jsonEscape(profilePath) << "\",\"measurement\":\""
        << jsonEscape(measurementPath) << "\",\"assessment\":\"" << jsonEscape(assessmentPath) << "\"},\n"
        << "  \"profile\":{\"software_class\":\"" << jsonEscape(profile.software_class) << "\",\"ai_ml\":"
        << (profile.ai_ml ? "true" : "false") << ",\"functional_unit_denominator\":" << profile.functional_unit_denominator << "},\n"
        << "  \"measurement\":{\"record_count\":" << measurement.record_count << ",\"mq\":" << decision.mq
        << ",\"dq\":" << decision.dq << ",\"uncertainty_percent\":" << decision.uncertainty
        << ",\"facility_energy_kwh\":" << measurement.facility_energy_kwh << ",\"carbon_kgco2e\":"
        << measurement.carbon_kgco2e << "},\n"
        << "  \"eligibility\":{\"status\":\"" << (decision.eligible ? "eligible" : "not_eligible")
        << "\",\"score_valid\":" << (decision.score_valid ? "true" : "false") << ",\"failed_gates\":[";
    for (std::size_t i = 0U; i < decision.failed_gates.size(); ++i) {
        if (i) out << ',';
        out << "\"" << jsonEscape(decision.failed_gates[i]) << "\"";
    }
    out << "]},\n  \"gates\":[";
    bool first = true;
    for (const auto& entry : input.gates) {
        if (!first) out << ',';
        first = false;
        const bool effective = decision.effective_gates.at(entry.first);
        out << "\n    {\"id\":\"" << entry.first << "\",\"declared_pass\":" << (entry.second.declared_pass ? "true" : "false")
            << ",\"effective_pass\":" << (effective ? "true" : "false") << ",\"evidence_status\":\""
            << jsonEscape(entry.second.evidence_status) << "\",\"evidence_ref\":\"" << jsonEscape(entry.second.evidence_ref) << "\"}";
    }
    out << "\n  ],\n  \"domains\":[";
    first = true;
    for (const auto& entry : domains) {
        if (!first) out << ',';
        first = false;
        const DomainScore& d = entry.second;
        out << "\n    {\"id\":\"" << d.id << "\",\"base_weight\":" << d.base_weight << ",\"effective_weight\":"
            << d.effective_weight << ",\"applicable_count\":" << d.applicable_count << ",\"na_count\":" << d.na_count
            << ",\"percent\":" << d.percent << ",\"weighted_points\":" << d.weighted_points << "}";
    }
    out << "\n  ],\n  \"criteria\":[";
    first = true;
    for (const auto& entry : input.criteria) {
        if (!first) out << ',';
        first = false;
        const Criterion& c = entry.second;
        out << "\n    {\"id\":\"" << entry.first << "\",\"applicable\":" << (c.applicable ? "true" : "false")
            << ",\"raw_score\":" << c.raw_score << ",\"effective_score\":" << c.effective_score
            << ",\"evidence_status\":\"" << jsonEscape(c.evidence_status) << "\",\"evidence_ref\":\""
            << jsonEscape(c.evidence_ref) << "\"}";
    }
    out << "\n  ],\n"
        << "  \"score\":{\"diagnostic_total\":" << decision.total_score << ",\"score_band\":\"" << levelName(decision.score_band)
        << "\",\"candidate_level\":\"" << levelName(decision.candidate_level) << "\"},\n"
        << "  \"eco_regression\":{\"control_defined\":" << (controlBool(input, "eco_regression_control_defined") ? "true" : "false")
        << ",\"baseline_present\":" << (decision.baseline_present ? "true" : "false")
        << ",\"baseline_energy_per_unit_j\":" << decision.baseline_energy_per_unit_j
        << ",\"current_energy_per_unit_j\":" << decision.current_energy_per_unit_j
        << ",\"delta_percent\":" << decision.eco_regression_percent << ",\"triggered\":"
        << (decision.eco_regression_triggered ? "true" : "false") << "},\n"
        << "  \"corrective_action_required\":" << (decision.corrective_action_required ? "true" : "false") << ",\n"
        << "  \"decision_reasons\":[";
    for (std::size_t i = 0U; i < decision.reasons.size(); ++i) {
        if (i) out << ',';
        out << "\"" << jsonEscape(decision.reasons[i]) << "\"";
    }
    out << "],\n  \"claims\":[";
    for (std::size_t i = 0U; i < claims.size(); ++i) {
        if (i) out << ',';
        out << "\n    {\"requested\":\"" << jsonEscape(claims[i].requested) << "\",\"status\":\""
            << jsonEscape(claims[i].status) << "\",\"safe_text\":\"" << jsonEscape(claims[i].safe_text) << "\"}";
    }
    out << "\n  ],\n"
        << "  \"claim_safe_text\":\"Candidate assessment evidence only. ShortHand does not grant C3-ECO certification, authorize public certification marks, or convert offsets, renewable hosting, avoided impact, modelled evidence or unsupported claims into certification evidence.\",\n"
        << "  \"next_qualification\":\"PR91 owns signed auditor bundles, retention, surveillance, registry evidence and authority-ready certification workflow. PR95 owns measured ShortHand-versus-Python energy comparison.\"\n"
        << "}\n";
}

}  // namespace shorthand::c3eco
