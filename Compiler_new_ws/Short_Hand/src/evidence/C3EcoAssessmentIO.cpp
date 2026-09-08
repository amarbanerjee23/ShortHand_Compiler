#include "C3EcoAssessment.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace shorthand::c3eco {
namespace {

constexpr std::size_t kMaxInputBytes = 4U * 1024U * 1024U;
constexpr std::size_t kMaxTsvLineBytes = 16384U;
constexpr double kEpsilon = 1e-9;

const std::set<std::string> kGateIds = {
    "G1", "G2", "G3", "G4", "G5", "G6", "G7", "G8", "G9", "G10", "G11", "G12", "G13", "G14"};

const std::map<char, int> kCriterionCounts = {
    {'A', 8}, {'B', 8}, {'C', 8}, {'D', 8}, {'E', 6}, {'F', 7},
    {'G', 10}, {'H', 5}, {'I', 5}, {'J', 4}, {'K', 7}};

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
            if (escaped) escaped = false;
            else if (c == '\\') escaped = true;
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
        const std::size_t colon = skipWhitespace(text, pos + needle.size());
        if (colon < text.size() && text[colon] == ':') positions.push_back(skipWhitespace(text, colon + 1U));
        pos += needle.size();
    }
    return positions;
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
        require(used == token.size() && std::isfinite(value), "invalid JSON number");
        return value;
    } catch (const std::exception&) {
        throw std::runtime_error("invalid JSON number");
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

std::string containerAt(const std::string& text, std::size_t pos, char open, char close) {
    require(pos < text.size() && text[pos] == open, "expected JSON container value");
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
        else if (c == open) ++depth;
        else if (c == close) {
            --depth;
            if (depth == 0) return text.substr(pos, i - pos + 1U);
            require(depth >= 0, "malformed JSON container depth");
        }
    }
    throw std::runtime_error("truncated JSON container");
}

std::string objectForKey(const std::string& text, const std::string& key, std::size_t start) {
    const std::string needle = "\"" + key + "\"";
    const std::size_t keyPos = text.find(needle, start);
    require(keyPos != std::string::npos, "missing JSON object key: " + key);
    const std::size_t colon = skipWhitespace(text, keyPos + needle.size());
    require(colon < text.size() && text[colon] == ':', "malformed JSON object key: " + key);
    return containerAt(text, skipWhitespace(text, colon + 1U), '{', '}');
}

std::string typedFieldsForDeclaration(const std::string& text, const std::string& kind) {
    const std::string marker = "\"kind\":\"" + kind + "\"";
    const std::size_t start = text.find(marker);
    require(start != std::string::npos, "profile evidence lacks declaration: " + kind);
    const std::size_t nextKind = text.find("\"kind\":", start + marker.size());
    const std::size_t typedKey = text.find("\"typed_fields\"", start);
    require(typedKey != std::string::npos && (nextKind == std::string::npos || typedKey < nextKind),
            "profile declaration lacks typed_fields: " + kind);
    return objectForKey(text, "typed_fields", start);
}

std::string typedString(const std::string& typedFields, const std::string& field, const std::string& expectedType) {
    const auto fieldPositions = keyValuePositions(typedFields, field);
    require(fieldPositions.size() == 1U, "typed field must appear exactly once: " + field);
    const std::string array = containerAt(typedFields, fieldPositions.front(), '[', ']');
    require(uniqueJsonString(array, "type") == expectedType, "typed field has wrong type: " + field);
    return uniqueJsonString(array, "value");
}

double typedNumber(const std::string& typedFields, const std::string& field, const std::string& expectedType) {
    const auto fieldPositions = keyValuePositions(typedFields, field);
    require(fieldPositions.size() == 1U, "typed field must appear exactly once: " + field);
    const std::string array = containerAt(typedFields, fieldPositions.front(), '[', ']');
    require(uniqueJsonString(array, "type") == expectedType, "typed field has wrong type: " + field);
    return uniqueJsonNumber(array, "value");
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
    return input;
}

ProfileInfo validateProfile(const std::string& path) {
    const std::string text = readBounded(path);
    validateJsonShape(text, "typed C3-ECO profile evidence");
    require(uniqueJsonString(text, "schema") == "shorthand.c3eco.candidate_report.v1", "profile evidence has wrong schema");
    require(!uniqueJsonBool(text, "official_certification_granted"), "profile evidence cannot already grant certification");
    require(uniqueJsonString(text, "c3eco_profile_contract") == "shorthand.c3eco.profile.v2", "profile evidence has wrong profile contract");
    require(uniqueJsonString(text, "c3eco_profile_status") == "conformant", "profile evidence must be conformant typed profile v2");
    require(text.find("\"kind\":\"certification_profile\"") != std::string::npos, "profile evidence lacks certification_profile declaration");
    require(text.find("\"kind\":\"boundary\"") != std::string::npos, "profile evidence lacks boundary declaration");
    require(text.find("\"kind\":\"guardrails\"") != std::string::npos, "profile evidence lacks guardrails declaration");

    const std::string certificationTyped = typedFieldsForDeclaration(text, "certification");
    const std::string functionalTyped = typedFieldsForDeclaration(text, "functional_unit");
    ProfileInfo info;
    info.software_class = typedString(certificationTyped, "software_class", "identifier");
    info.functional_unit_denominator = typedNumber(functionalTyped, "denominator", "integer");
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

    const std::string totals = objectForKey(text, "totals", 0U);
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

}  // namespace shorthand::c3eco
