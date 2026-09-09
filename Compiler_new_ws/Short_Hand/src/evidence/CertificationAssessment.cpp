#include "C3EcoAssessment.h"
#include "C3EcoEvidenceIO.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace shorthand::c3eco {

constexpr const char *kAssessmentSchema = "shorthand.c3eco.assessment.v1";
constexpr const char *kProfileSchema = "shorthand.c3eco.candidate_report.v1";
constexpr const char *kProfileContract = "shorthand.c3eco.profile.v2";
constexpr const char *kMeasurementSchema = "shorthand.c3eco.measurement_workbook.v1";
constexpr double kEcoRegressionThresholdPercent = 10.0;
constexpr double kEpsilon = 1e-9;
constexpr std::size_t kMaxJsonBytes = 32U * 1024U * 1024U;
constexpr std::size_t kMaxJsonDepth = 64;
constexpr std::size_t kMaxTsvLineBytes = 1024U * 1024U;
constexpr std::size_t kMaxTsvBytes = 32U * 1024U * 1024U;
constexpr std::size_t kMaxTsvRows = 10000;

void require(bool condition, const std::string &message) {
    if (!condition)
        throw std::runtime_error(message);
}

std::string readFile(const std::string &path) {
    std::ifstream input(path, std::ios::binary);
    require(static_cast<bool>(input), "cannot open input: " + path);
    input.seekg(0, std::ios::end);
    const std::streamoff length = input.tellg();
    require(length >= 0, "cannot determine input size: " + path);
    require(static_cast<unsigned long long>(length) <= kMaxJsonBytes,
            "JSON input exceeds 32 MiB limit: " + path);
    input.seekg(0, std::ios::beg);
    std::string contents(static_cast<std::size_t>(length), '\0');
    if (!contents.empty())
        input.read(&contents[0], static_cast<std::streamsize>(contents.size()));
    require(static_cast<bool>(input) || input.eof(), "cannot read input: " + path);
    return contents;
}

std::string joinPath(const std::string &directory, const std::string &name) {
    if (directory.empty())
        return name;
    const char last = directory.back();
    if (last == '/' || last == '\\')
        return directory + name;
    return directory + "/" + name;
}

std::string jsonEscape(const std::string &value) {
    std::ostringstream out;
    for (unsigned char c : value) {
        switch (c) {
        case '"':
            out << "\\\"";
            break;
        case '\\':
            out << "\\\\";
            break;
        case '\b':
            out << "\\b";
            break;
        case '\f':
            out << "\\f";
            break;
        case '\n':
            out << "\\n";
            break;
        case '\r':
            out << "\\r";
            break;
        case '\t':
            out << "\\t";
            break;
        default:
            if (c < 0x20) {
                out << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                    << static_cast<unsigned int>(c) << std::dec << std::setfill(' ');
            } else {
                out << static_cast<char>(c);
            }
        }
    }
    return out.str();
}

class JsonParser {
  public:
    explicit JsonParser(std::string source) : source_(std::move(source)) {}

    Json parse() {
        skipWhitespace();
        Json result = parseValue(0);
        skipWhitespace();
        require(position_ == source_.size(), error("trailing data"));
        return result;
    }

  private:
    std::string source_;
    std::size_t position_ = 0;

    std::string error(const std::string &message) const {
        return "invalid JSON at byte " + std::to_string(position_) + ": " + message;
    }

    void skipWhitespace() {
        while (position_ < source_.size()) {
            const char c = source_[position_];
            if (c != ' ' && c != '\t' && c != '\r' && c != '\n')
                break;
            ++position_;
        }
    }

    char peek() const {
        require(position_ < source_.size(), error("unexpected end of input"));
        return source_[position_];
    }

    bool consume(char expected) {
        if (position_ < source_.size() && source_[position_] == expected) {
            ++position_;
            return true;
        }
        return false;
    }

    void expect(char expected) {
        require(consume(expected), error(std::string("expected '") + expected + "'"));
    }

    Json parseValue(std::size_t depth) {
        require(depth <= kMaxJsonDepth, error("nesting depth exceeds 64"));
        skipWhitespace();
        const char c = peek();
        if (c == '{')
            return parseObject(depth);
        if (c == '[')
            return parseArray(depth);
        if (c == '"') {
            Json value;
            value.kind = Json::Kind::String;
            value.string = parseString();
            return value;
        }
        if (c == 't')
            return parseLiteral("true", Json::Kind::Boolean, true);
        if (c == 'f')
            return parseLiteral("false", Json::Kind::Boolean, false);
        if (c == 'n')
            return parseLiteral("null", Json::Kind::Null, false);
        if (c == '-' || (c >= '0' && c <= '9'))
            return parseNumber();
        throw std::runtime_error(error("unexpected token"));
    }

    Json parseLiteral(const std::string &literal, Json::Kind kind, bool boolean) {
        require(source_.compare(position_, literal.size(), literal) == 0, error("invalid literal"));
        position_ += literal.size();
        Json value;
        value.kind = kind;
        value.boolean = boolean;
        return value;
    }

    static int hexValue(char c) {
        if (c >= '0' && c <= '9')
            return c - '0';
        if (c >= 'a' && c <= 'f')
            return c - 'a' + 10;
        if (c >= 'A' && c <= 'F')
            return c - 'A' + 10;
        return -1;
    }

    unsigned int parseUnicodeUnit() {
        require(position_ + 4 <= source_.size(), error("incomplete unicode escape"));
        unsigned int unit = 0;
        for (int index = 0; index < 4; ++index) {
            const int value = hexValue(source_[position_++]);
            require(value >= 0, error("invalid unicode escape"));
            unit = (unit << 4U) | static_cast<unsigned int>(value);
        }
        return unit;
    }

    static void appendUtf8(std::string &output, unsigned int codePoint) {
        if (codePoint <= 0x7FU) {
            output.push_back(static_cast<char>(codePoint));
        } else if (codePoint <= 0x7FFU) {
            output.push_back(static_cast<char>(0xC0U | (codePoint >> 6U)));
            output.push_back(static_cast<char>(0x80U | (codePoint & 0x3FU)));
        } else if (codePoint <= 0xFFFFU) {
            output.push_back(static_cast<char>(0xE0U | (codePoint >> 12U)));
            output.push_back(static_cast<char>(0x80U | ((codePoint >> 6U) & 0x3FU)));
            output.push_back(static_cast<char>(0x80U | (codePoint & 0x3FU)));
        } else {
            output.push_back(static_cast<char>(0xF0U | (codePoint >> 18U)));
            output.push_back(static_cast<char>(0x80U | ((codePoint >> 12U) & 0x3FU)));
            output.push_back(static_cast<char>(0x80U | ((codePoint >> 6U) & 0x3FU)));
            output.push_back(static_cast<char>(0x80U | (codePoint & 0x3FU)));
        }
    }

    std::string parseString() {
        expect('"');
        std::string output;
        while (position_ < source_.size()) {
            const unsigned char c = static_cast<unsigned char>(source_[position_++]);
            if (c == '"')
                return output;
            require(c >= 0x20, error("unescaped control character in string"));
            if (c != '\\') {
                output.push_back(static_cast<char>(c));
                continue;
            }
            require(position_ < source_.size(), error("incomplete string escape"));
            const char escape = source_[position_++];
            switch (escape) {
            case '"':
                output.push_back('"');
                break;
            case '\\':
                output.push_back('\\');
                break;
            case '/':
                output.push_back('/');
                break;
            case 'b':
                output.push_back('\b');
                break;
            case 'f':
                output.push_back('\f');
                break;
            case 'n':
                output.push_back('\n');
                break;
            case 'r':
                output.push_back('\r');
                break;
            case 't':
                output.push_back('\t');
                break;
            case 'u': {
                unsigned int codePoint = parseUnicodeUnit();
                if (codePoint >= 0xD800U && codePoint <= 0xDBFFU) {
                    require(position_ + 2 <= source_.size() && source_[position_] == '\\' &&
                                source_[position_ + 1] == 'u',
                            error("high surrogate must be followed by a low surrogate"));
                    position_ += 2;
                    const unsigned int low = parseUnicodeUnit();
                    require(low >= 0xDC00U && low <= 0xDFFFU, error("invalid low surrogate"));
                    codePoint = 0x10000U + ((codePoint - 0xD800U) << 10U) + (low - 0xDC00U);
                } else {
                    require(!(codePoint >= 0xDC00U && codePoint <= 0xDFFFU),
                            error("unexpected low surrogate"));
                }
                appendUtf8(output, codePoint);
                break;
            }
            default:
                throw std::runtime_error(error("invalid string escape"));
            }
        }
        throw std::runtime_error(error("unterminated string"));
    }

    Json parseNumber() {
        const std::size_t start = position_;
        consume('-');
        require(position_ < source_.size(), error("incomplete number"));
        if (source_[position_] == '0') {
            ++position_;
            require(position_ == source_.size() || source_[position_] < '0' ||
                        source_[position_] > '9',
                    error("leading zero in number"));
        } else {
            require(source_[position_] >= '1' && source_[position_] <= '9',
                    error("invalid number"));
            while (position_ < source_.size() && source_[position_] >= '0' &&
                   source_[position_] <= '9') {
                ++position_;
            }
        }
        if (consume('.')) {
            require(position_ < source_.size() && source_[position_] >= '0' &&
                        source_[position_] <= '9',
                    error("fraction has no digits"));
            while (position_ < source_.size() && source_[position_] >= '0' &&
                   source_[position_] <= '9') {
                ++position_;
            }
        }
        if (position_ < source_.size() &&
            (source_[position_] == 'e' || source_[position_] == 'E')) {
            ++position_;
            if (position_ < source_.size() &&
                (source_[position_] == '+' || source_[position_] == '-')) {
                ++position_;
            }
            require(position_ < source_.size() && source_[position_] >= '0' &&
                        source_[position_] <= '9',
                    error("exponent has no digits"));
            while (position_ < source_.size() && source_[position_] >= '0' &&
                   source_[position_] <= '9') {
                ++position_;
            }
        }
        Json value;
        value.kind = Json::Kind::Number;
        try {
            std::size_t used = 0;
            const std::string token = source_.substr(start, position_ - start);
            value.number = std::stod(token, &used);
            require(used == token.size() && std::isfinite(value.number), error("invalid number"));
        } catch (const std::exception &) {
            throw std::runtime_error(error("invalid number"));
        }
        return value;
    }

    Json parseArray(std::size_t depth) {
        Json value;
        value.kind = Json::Kind::Array;
        expect('[');
        skipWhitespace();
        if (consume(']'))
            return value;
        while (true) {
            value.array.push_back(parseValue(depth + 1));
            skipWhitespace();
            if (consume(']'))
                return value;
            expect(',');
            skipWhitespace();
        }
    }

    Json parseObject(std::size_t depth) {
        Json value;
        value.kind = Json::Kind::Object;
        expect('{');
        skipWhitespace();
        if (consume('}'))
            return value;
        while (true) {
            require(peek() == '"', error("object key must be a string"));
            const std::string key = parseString();
            skipWhitespace();
            expect(':');
            Json member = parseValue(depth + 1);
            require(value.object.emplace(key, std::move(member)).second,
                    error("duplicate object key: " + key));
            skipWhitespace();
            if (consume('}'))
                return value;
            expect(',');
            skipWhitespace();
        }
    }
};

Json parseJson(std::string source) { return JsonParser(std::move(source)).parse(); }

const Json &jsonMember(const Json &object, const std::string &key) {
    require(object.kind == Json::Kind::Object, "JSON value must be an object");
    const auto found = object.object.find(key);
    require(found != object.object.end(), "JSON object missing required member: " + key);
    return found->second;
}

const Json &jsonMember(const Json &object, const char *key) {
    require(object.kind == Json::Kind::Object, "JSON value must be an object");
    const auto found = object.object.find(key);
    require(found != object.object.end(),
            std::string("JSON object missing required member: ") + key);
    return found->second;
}

const Json *findJsonMember(const Json &object, const std::string &key) {
    require(object.kind == Json::Kind::Object, "JSON value must be an object");
    const auto found = object.object.find(key);
    return found == object.object.end() ? nullptr : &found->second;
}

std::string jsonString(const Json &object, const std::string &key) {
    const Json &value = jsonMember(object, key);
    require(value.kind == Json::Kind::String, "JSON member must be a string: " + key);
    return value.string;
}

bool jsonBoolean(const Json &object, const std::string &key) {
    const Json &value = jsonMember(object, key);
    require(value.kind == Json::Kind::Boolean, "JSON member must be a boolean: " + key);
    return value.boolean;
}

double jsonNumber(const Json &object, const std::string &key) {
    const Json &value = jsonMember(object, key);
    require(value.kind == Json::Kind::Number, "JSON member must be a number: " + key);
    return value.number;
}

void requireNonemptyJsonString(const Json &object, const std::string &key) {
    require(!jsonString(object, key).empty(), "JSON string member must be non-empty: " + key);
}

void requireExactKeys(const Json &object, const std::set<std::string> &keys,
                      const std::string &description) {
    require(object.kind == Json::Kind::Object, description + " must be an object");
    require(object.object.size() == keys.size(),
            description + " must contain exactly the contract keys");
    for (const std::string &key : keys) {
        require(object.object.count(key) == 1, description + " missing required member: " + key);
    }
}

bool isLeapYear(int year) { return (year % 400 == 0) || (year % 4 == 0 && year % 100 != 0); }

int daysInMonth(int year, int month) {
    static const int days[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month < 1 || month > 12)
        return 0;
    return days[month] + (month == 2 && isLeapYear(year) ? 1 : 0);
}

bool validIsoDate(const std::string &value) {
    if (value.size() != 10 || value[4] != '-' || value[7] != '-')
        return false;
    for (std::size_t index = 0; index < value.size(); ++index) {
        if (index == 4 || index == 7)
            continue;
        if (value[index] < '0' || value[index] > '9')
            return false;
    }
    const int year = std::stoi(value.substr(0, 4));
    const int month = std::stoi(value.substr(5, 2));
    const int day = std::stoi(value.substr(8, 2));
    return year >= 2000 && month >= 1 && month <= 12 && day >= 1 && day <= daysInMonth(year, month);
}

bool reconciles(double left, double right) {
    if (!std::isfinite(left) || !std::isfinite(right))
        return false;
    const double scale = std::max(1.0, std::max(std::abs(left), std::abs(right)));
    return std::abs(left - right) <= scale * kEpsilon;
}

std::vector<std::string> splitTsv(const std::string &line) {
    std::vector<std::string> fields;
    std::string field;
    for (char c : line) {
        if (c == '\t') {
            fields.push_back(field);
            field.clear();
        } else if (c != '\r') {
            field.push_back(c);
        }
    }
    fields.push_back(field);
    return fields;
}

std::vector<std::vector<std::string>> loadTsv(const std::string &path,
                                              const std::vector<std::string> &expectedHeader) {
    std::ifstream input(path, std::ios::binary);
    require(static_cast<bool>(input), "cannot open input: " + path);
    input.seekg(0, std::ios::end);
    const std::streamoff length = input.tellg();
    require(length >= 0, "cannot determine input size: " + path);
    require(static_cast<unsigned long long>(length) <= kMaxTsvBytes,
            "TSV input exceeds 32 MiB limit: " + path);
    input.seekg(0, std::ios::beg);
    std::string line;
    require(static_cast<bool>(std::getline(input, line)), "TSV input is empty: " + path);
    require(line.size() <= kMaxTsvLineBytes, "TSV header exceeds 1 MiB limit: " + path);
    require(splitTsv(line) == expectedHeader, "invalid TSV header: " + path);
    std::vector<std::vector<std::string>> rows;
    std::size_t lineNumber = 1;
    while (std::getline(input, line)) {
        ++lineNumber;
        require(line.size() <= kMaxTsvLineBytes,
                path + " line " + std::to_string(lineNumber) + ": exceeds 1 MiB limit");
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        if (line.empty())
            continue;
        std::vector<std::string> fields = splitTsv(line);
        require(fields.size() == expectedHeader.size(),
                path + " line " + std::to_string(lineNumber) + ": expected " +
                    std::to_string(expectedHeader.size()) + " columns");
        require(rows.size() < kMaxTsvRows, path + ": exceeds 10000 data-row limit");
        rows.push_back(std::move(fields));
    }
    require(input.eof(), "cannot read input: " + path);
    return rows;
}

double parseNumber(const std::string &text, const std::string &field) {
    try {
        require(!text.empty() && text.front() != ' ' && text.front() != '\t' &&
                    text.back() != ' ' && text.back() != '\t',
                "numeric field has surrounding whitespace: " + field);
        std::size_t used = 0;
        const double value = std::stod(text, &used);
        require(used == text.size() && std::isfinite(value), "invalid numeric " + field);
        return value;
    } catch (const std::exception &) {
        throw std::runtime_error("invalid numeric " + field + ": " + text);
    }
}

int parseInteger(const std::string &text, const std::string &field) {
    const double value = parseNumber(text, field);
    require(std::floor(value) == value, field + " must be an integer");
    require(value >= static_cast<double>(std::numeric_limits<int>::min()) &&
                value <= static_cast<double>(std::numeric_limits<int>::max()),
            field + " is outside the supported integer range");
    return static_cast<int>(value);
}

bool parseBoolean(const std::string &text, const std::string &field) {
    if (text == "true")
        return true;
    if (text == "false")
        return false;
    throw std::runtime_error(field + " must be true or false");
}

bool hasNonWhitespace(const std::string &value) {
    return std::any_of(value.begin(), value.end(), [](char c) {
        return c != ' ' && c != '\t' && c != '\r' && c != '\n' && c != '\f' && c != '\v';
    });
}

bool hasEvidence(const std::string &value) {
    return hasNonWhitespace(value) && value != "-" && value != "none";
}

struct MetadataValue {
    std::string value;
    std::string evidenceStatus;
    std::string evidence;
};

std::map<std::string, MetadataValue> loadMetadataValues(const std::string &path) {
    const auto rows = loadTsv(path, {"key", "value", "evidence_status", "evidence_ref"});
    const std::set<std::string> evidenceStatuses = {"documented", "measured", "verified",
                                                    "independent"};
    std::map<std::string, MetadataValue> result;
    for (const auto &row : rows) {
        require(!row[0].empty(), "metadata key is required");
        require(!row[1].empty(), "metadata value is required for " + row[0]);
        require(evidenceStatuses.count(row[2]) == 1,
                "metadata evidence_status is invalid for " + row[0]);
        require(hasEvidence(row[3]), "metadata evidence_ref is required for " + row[0]);
        require(result.emplace(row[0], MetadataValue{row[1], row[2], row[3]}).second,
                "duplicate metadata key: " + row[0]);
    }
    return result;
}

Metadata loadMetadata(const std::string &path) {
    const std::map<std::string, MetadataValue> values = loadMetadataValues(path);
    const std::set<std::string> requiredKeys = {"product_name",
                                                "product_version",
                                                "software_class",
                                                "functional_unit",
                                                "boundary",
                                                "workload",
                                                "measurement_quality",
                                                "data_quality",
                                                "uncertainty_percent",
                                                "penalty_points",
                                                "ai_in_scope",
                                                "ai_role",
                                                "training_boundary_declared",
                                                "inference_boundary_declared",
                                                "token_or_inference_metric_declared",
                                                "ai_exclusions_declared",
                                                "quality_energy_frontier_options",
                                                "client_device_impact",
                                                "eco_regression_control_defined",
                                                "independent_review",
                                                "public_report",
                                                "continuous_telemetry",
                                                "public_evidence",
                                                "high_severity_claim_risk",
                                                "high_scale_saas"};
    require(values.size() == requiredKeys.size(),
            "metadata must contain exactly the required keys");
    for (const std::string &key : requiredKeys) {
        require(values.count(key) == 1, "metadata missing required key: " + key);
    }

    const auto get = [&](const std::string &key) -> const std::string & {
        return values.at(key).value;
    };
    Metadata metadata;
    metadata.productName = get("product_name");
    metadata.productVersion = get("product_version");
    metadata.softwareClass = get("software_class");
    metadata.functionalUnit = get("functional_unit");
    metadata.boundary = get("boundary");
    metadata.workload = get("workload");
    require(hasNonWhitespace(metadata.productName) && hasNonWhitespace(metadata.productVersion) &&
                hasNonWhitespace(metadata.functionalUnit) && hasNonWhitespace(metadata.boundary) &&
                hasNonWhitespace(metadata.workload),
            "product, version, functional unit, boundary and workload must be non-blank");
    const std::string mq = get("measurement_quality");
    require(mq.size() == 3 && mq[0] == 'M' && mq[1] == 'Q' && mq[2] >= '1' && mq[2] <= '4',
            "measurement_quality must be MQ1, MQ2, MQ3 or MQ4");
    metadata.measurementQuality = mq[2] - '0';
    const std::string dq = get("data_quality");
    require(dq.size() == 3 && dq[0] == 'D' && dq[1] == 'Q' && dq[2] >= '1' && dq[2] <= '4',
            "data_quality must be DQ1, DQ2, DQ3 or DQ4");
    metadata.dataQuality = dq[2] - '0';
    if (metadata.measurementQuality == 4) {
        require(values.at("measurement_quality").evidenceStatus == "independent",
                "MQ4 requires independent evidence");
    }
    if (metadata.dataQuality == 4) {
        require(values.at("data_quality").evidenceStatus == "independent",
                "DQ4 requires independent evidence");
    }
    metadata.uncertaintyPercent = parseNumber(get("uncertainty_percent"), "uncertainty_percent");
    require(metadata.uncertaintyPercent >= 0.0 && metadata.uncertaintyPercent <= 100.0,
            "uncertainty_percent must be in [0,100]");
    metadata.penaltyPoints = parseNumber(get("penalty_points"), "penalty_points");
    require(metadata.penaltyPoints >= 0.0 && metadata.penaltyPoints <= 100.0,
            "penalty_points must be in [0,100]");
    metadata.aiInScope = parseBoolean(get("ai_in_scope"), "ai_in_scope");
    metadata.aiRole = get("ai_role");
    metadata.trainingBoundaryDeclared =
        parseBoolean(get("training_boundary_declared"), "training_boundary_declared");
    metadata.inferenceBoundaryDeclared =
        parseBoolean(get("inference_boundary_declared"), "inference_boundary_declared");
    metadata.tokenOrInferenceMetricDeclared = parseBoolean(
        get("token_or_inference_metric_declared"), "token_or_inference_metric_declared");
    metadata.aiExclusionsDeclared =
        parseBoolean(get("ai_exclusions_declared"), "ai_exclusions_declared");
    metadata.qualityEnergyFrontierOptions =
        parseInteger(get("quality_energy_frontier_options"), "quality_energy_frontier_options");
    require(metadata.qualityEnergyFrontierOptions >= 0 &&
                metadata.qualityEnergyFrontierOptions <= 100,
            "quality_energy_frontier_options must be in [0,100]");
    metadata.clientDeviceImpact = parseBoolean(get("client_device_impact"), "client_device_impact");
    metadata.ecoRegressionControlDefined =
        parseBoolean(get("eco_regression_control_defined"), "eco_regression_control_defined");
    metadata.independentReview = parseBoolean(get("independent_review"), "independent_review");
    metadata.publicReport = parseBoolean(get("public_report"), "public_report");
    metadata.continuousTelemetry =
        parseBoolean(get("continuous_telemetry"), "continuous_telemetry");
    metadata.publicEvidence = parseBoolean(get("public_evidence"), "public_evidence");
    metadata.highSeverityClaimRisk =
        parseBoolean(get("high_severity_claim_risk"), "high_severity_claim_risk");
    metadata.highScaleSaas = parseBoolean(get("high_scale_saas"), "high_scale_saas");

    require(metadata.softwareClass.size() >= 2 && metadata.softwareClass[0] == 'S',
            "software_class must use canonical form S1 through S12");
    const std::size_t suffixPosition = metadata.softwareClass.find('_');
    const std::string classNumber = metadata.softwareClass.substr(
        1, suffixPosition == std::string::npos ? std::string::npos : suffixPosition - 1);
    const int softwareClassNumber = parseInteger(classNumber, "software_class");
    require(softwareClassNumber >= 1 && softwareClassNumber <= 12,
            "software_class must use canonical form S1 through S12");
    const std::string canonicalPrefix = "S" + std::to_string(softwareClassNumber);
    require(metadata.softwareClass == canonicalPrefix ||
                metadata.softwareClass.rfind(canonicalPrefix + "_", 0) == 0,
            "software_class must use canonical form S1 through S12");
    if (suffixPosition != std::string::npos) {
        const std::set<std::string> profileAliases = {"S6_AI_GENAI", "S9_DEVELOPER_TOOLS_CI_CD",
                                                      "S12_INFRASTRUCTURE_PLATFORM"};
        require(profileAliases.count(metadata.softwareClass) == 1,
                "software_class alias is not supported by the typed profile");
    }

    const std::set<std::string> roles = {"model_provider", "application_deployer", "integrator",
                                         "auditor"};
    if (metadata.aiInScope) {
        require(roles.count(metadata.aiRole) == 1, "AI scope requires ai_role model_provider, "
                                                   "application_deployer, integrator or auditor");
        require(
            metadata.trainingBoundaryDeclared,
            "AI scope requires a training boundary declaration, including an explicit exclusion");
        require(metadata.inferenceBoundaryDeclared,
                "AI scope requires an inference boundary declaration");
        require(metadata.tokenOrInferenceMetricDeclared,
                "AI scope requires a token or inference metric declaration");
        require(metadata.aiExclusionsDeclared, "AI scope requires AI exclusions to be declared");
    } else {
        require(metadata.aiRole == "not_applicable",
                "non-AI scope requires ai_role=not_applicable");
    }
    return metadata;
}

std::map<std::string, Gate> loadGates(const std::string &path) {
    const auto rows = loadTsv(path, {"gate_id", "status", "evidence_ref"});
    std::map<std::string, Gate> gates;
    for (const auto &row : rows) {
        require(row[0].size() >= 2 && row[0][0] == 'G', "invalid mandatory gate id: " + row[0]);
        const int number = parseInteger(row[0].substr(1), "gate_id");
        require(number >= 1 && number <= 14, "mandatory gate id must be G1 through G14");
        require(row[0] == "G" + std::to_string(number),
                "mandatory gate id must use canonical form: " + row[0]);
        require(row[1] == "pass" || row[1] == "fail",
                "gate status must be pass or fail for " + row[0]);
        require(hasEvidence(row[2]), "gate evidence_ref is required for " + row[0]);
        Gate gate{row[0], row[1] == "pass", row[1] == "pass", row[2]};
        require(gates.emplace(gate.id, gate).second, "duplicate mandatory gate: " + gate.id);
    }
    require(gates.size() == 14, "exactly G1 through G14 are required");
    for (int number = 1; number <= 14; ++number) {
        require(gates.count("G" + std::to_string(number)) == 1,
                "missing mandatory gate G" + std::to_string(number));
    }
    return gates;
}

const std::map<char, int> &normativeCriterionCounts() {
    static const std::map<char, int> counts = {{'A', 8}, {'B', 8}, {'C', 8},  {'D', 8},
                                               {'E', 6}, {'F', 7}, {'G', 10}, {'H', 5},
                                               {'I', 5}, {'J', 4}, {'K', 7}};
    return counts;
}

std::vector<Criterion> loadCriteria(const std::string &path, const Metadata &metadata) {
    const auto rows =
        loadTsv(path, {"criterion_id", "domain", "raw_score", "criterion_weight", "evidence_status",
                       "applicable", "auditor_approval_ref", "evidence_ref"});
    require(!rows.empty(), "criteria.tsv must contain at least one criterion");
    std::set<std::string> ids;
    std::vector<Criterion> criteria;
    for (const auto &row : rows) {
        require(!row[0].empty() && ids.insert(row[0]).second,
                "duplicate or empty criterion_id: " + row[0]);
        require(row[1].size() == 1 && row[1][0] >= 'A' && row[1][0] <= 'K',
                "criterion domain must be A through K for " + row[0]);
        require(row[0].size() >= 2 && row[0][0] == row[1][0],
                "criterion_id must begin with its domain for " + row[0]);
        const int criterionNumber = parseInteger(row[0].substr(1), "criterion_id");
        require(row[0] == std::string(1, row[1][0]) + std::to_string(criterionNumber),
                "criterion_id must use canonical form: " + row[0]);
        require(criterionNumber >= 1 && criterionNumber <= normativeCriterionCounts().at(row[1][0]),
                "criterion_id is outside the normative C3-ECO catalog: " + row[0]);
        Criterion criterion;
        criterion.id = row[0];
        criterion.domain = row[1][0];
        criterion.rawScore = parseInteger(row[2], "raw_score for " + row[0]);
        require(criterion.rawScore >= 0 && criterion.rawScore <= 5,
                "raw_score must be in [0,5] for " + row[0]);
        criterion.weight = parseNumber(row[3], "criterion_weight for " + row[0]);
        require(criterion.weight > 0.0 && criterion.weight <= 1000000.0,
                "criterion_weight must be in (0,1000000] for " + row[0]);
        criterion.evidenceStatus = row[4];
        criterion.applicable = parseBoolean(row[5], "applicable for " + row[0]);
        criterion.approval = row[6];
        criterion.evidence = row[7];
        if (criterion.applicable) {
            require(criterion.evidenceStatus == "sufficient" ||
                        criterion.evidenceStatus == "insufficient",
                    "applicable criterion evidence_status must be sufficient or insufficient for " +
                        row[0]);
            require(hasEvidence(criterion.evidence), "evidence_ref is required for " + row[0]);
            criterion.effectiveScore = criterion.evidenceStatus == "insufficient"
                                           ? std::min(criterion.rawScore, 1)
                                           : criterion.rawScore;
        } else {
            require(criterion.evidenceStatus == "not_applicable",
                    "non-applicable criterion must use evidence_status=not_applicable for " +
                        row[0]);
            require(criterion.rawScore == 0,
                    "non-applicable criterion raw_score must be 0 for " + row[0]);
            require(hasEvidence(criterion.approval),
                    "N/A criterion requires auditor_approval_ref for " + row[0]);
            require(hasEvidence(criterion.evidence),
                    "N/A criterion requires evidence_ref for " + row[0]);
            criterion.effectiveScore = 0;
        }
        criteria.push_back(std::move(criterion));
    }
    std::size_t normativeCount = 0;
    for (const auto &domain : normativeCriterionCounts()) {
        for (int number = 1; number <= domain.second; ++number) {
            const std::string id = std::string(1, domain.first) + std::to_string(number);
            require(ids.count(id) == 1, "criteria.tsv missing normative criterion: " + id);
            ++normativeCount;
        }
    }
    require(criteria.size() == normativeCount,
            "criteria.tsv must contain exactly the 76 normative C3-ECO criteria");

    const auto domainApplicable = [&](char domain) {
        return std::any_of(criteria.begin(), criteria.end(), [domain](const Criterion &criterion) {
            return criterion.domain == domain && criterion.applicable;
        });
    };
    for (char domain = 'A'; domain <= 'K'; ++domain) {
        const bool anyApplicable = domainApplicable(domain);
        const bool anyNotApplicable =
            std::any_of(criteria.begin(), criteria.end(), [domain](const Criterion &criterion) {
                return criterion.domain == domain && !criterion.applicable;
            });
        require(!(anyApplicable && anyNotApplicable),
                std::string("criterion applicability must be all-or-nothing for domain ") + domain);
    }
    require(domainApplicable('A') && domainApplicable('B') && domainApplicable('K'),
            "core domains A, B and K cannot be marked N/A");
    if (metadata.aiInScope) {
        require(domainApplicable('G'), "AI/ML domain G cannot be N/A when AI is in scope");
    }
    if (metadata.clientDeviceImpact) {
        require(domainApplicable('H'),
                "hardware longevity domain H cannot be N/A for material client-device impact");
    }
    std::sort(criteria.begin(), criteria.end(), [](const Criterion &left, const Criterion &right) {
        if (left.domain != right.domain)
            return left.domain < right.domain;
        return std::stoi(left.id.substr(1)) < std::stoi(right.id.substr(1));
    });
    return criteria;
}

MaterialityResult loadMateriality(const std::string &path) {
    const auto rows = loadTsv(path, {"component", "share_percent", "disposition", "evidence_ref"});
    require(!rows.empty(), "materiality.tsv must contain at least one component");
    std::set<std::string> names;
    MaterialityResult result;
    for (const auto &row : rows) {
        require(!row[0].empty() && names.insert(row[0]).second,
                "duplicate or empty materiality component: " + row[0]);
        MaterialityComponent component;
        component.component = row[0];
        component.sharePercent = parseNumber(row[1], "share_percent for " + row[0]);
        require(component.sharePercent >= 0.0 && component.sharePercent <= 100.0,
                "materiality share_percent must be in [0,100] for " + row[0]);
        component.disposition = row[2];
        require(component.disposition == "included" ||
                    component.disposition == "conservative_estimate" ||
                    component.disposition == "omitted",
                "materiality disposition must be included, conservative_estimate or omitted for " +
                    row[0]);
        require(hasEvidence(row[3]), "materiality evidence_ref is required for " + row[0]);
        component.evidence = row[3];
        result.declaredSharePercent += component.sharePercent;
        if (component.disposition == "omitted") {
            result.omittedSharePercent += component.sharePercent;
            if (component.sharePercent + kEpsilon >= 1.0)
                result.individualMaterialOmission = true;
        }
        result.components.push_back(std::move(component));
    }
    require(reconciles(result.declaredSharePercent, 100.0),
            "materiality component shares must account for exactly 100 percent");
    result.cumulativeOmissionExceeded = result.omittedSharePercent > 5.0 + kEpsilon;
    std::sort(result.components.begin(), result.components.end(),
              [](const MaterialityComponent &left, const MaterialityComponent &right) {
                  return left.component < right.component;
              });
    return result;
}

std::vector<Regression> loadRegressions(const std::string &path) {
    const auto rows =
        loadTsv(path, {"metric_id", "kind", "baseline_value", "current_value", "lower_is_better",
                       "explanation", "corrective_action_ref", "evidence_ref"});
    std::set<std::string> ids;
    std::vector<Regression> regressions;
    for (const auto &row : rows) {
        require(!row[0].empty() && ids.insert(row[0]).second,
                "duplicate or empty regression metric_id: " + row[0]);
        Regression regression;
        regression.id = row[0];
        regression.kind = row[1];
        require(regression.kind == "eco" || regression.kind == "quality",
                "regression kind must be eco or quality for " + row[0]);
        regression.baseline = parseNumber(row[2], "baseline_value for " + row[0]);
        regression.current = parseNumber(row[3], "current_value for " + row[0]);
        require(regression.baseline > 0.0 && regression.current >= 0.0,
                "regression values require baseline > 0 and current >= 0 for " + row[0]);
        regression.lowerIsBetter = parseBoolean(row[4], "lower_is_better for " + row[0]);
        regression.explanation = row[5];
        regression.correctiveAction = row[6];
        require(hasEvidence(row[7]), "regression evidence_ref is required for " + row[0]);
        regression.evidence = row[7];
        const double change = regression.lowerIsBetter ? regression.current - regression.baseline
                                                       : regression.baseline - regression.current;
        regression.deteriorationPercent = (change / regression.baseline) * 100.0;
        require(std::isfinite(regression.deteriorationPercent),
                "regression deterioration is outside the supported numeric range for " + row[0]);
        if (std::abs(regression.deteriorationPercent) < kEpsilon)
            regression.deteriorationPercent = 0.0;
        if (regression.kind == "eco") {
            regression.thresholdExceeded =
                regression.deteriorationPercent > kEcoRegressionThresholdPercent + kEpsilon;
            regression.unresolved =
                regression.thresholdExceeded &&
                (!hasEvidence(regression.explanation) || !hasEvidence(regression.correctiveAction));
        } else {
            regression.thresholdExceeded = regression.deteriorationPercent > kEpsilon;
            regression.unresolved = regression.thresholdExceeded;
        }
        regressions.push_back(std::move(regression));
    }
    std::sort(regressions.begin(), regressions.end(),
              [](const Regression &left, const Regression &right) { return left.id < right.id; });
    return regressions;
}

std::vector<Claim> loadClaims(const std::string &path) {
    const auto rows =
        loadTsv(path, {"claim_id", "claim_type", "claim_text", "scope_matches",
                       "functional_unit_equivalent", "boundary_equivalent", "quality_equivalent",
                       "method_equivalent", "legal_technical_evidence", "evidence_ref"});
    require(!rows.empty(), "claims.tsv must contain at least one claim request");
    const std::set<std::string> types = {"candidate_assessment",
                                         "certified",
                                         "level",
                                         "comparative",
                                         "green_ai",
                                         "zero",
                                         "best",
                                         "climate_positive",
                                         "net_positive",
                                         "offsets_only"};
    std::set<std::string> ids;
    std::vector<Claim> claims;
    for (const auto &row : rows) {
        require(!row[0].empty() && ids.insert(row[0]).second,
                "duplicate or empty claim_id: " + row[0]);
        require(types.count(row[1]) == 1, "unsupported claim_type for " + row[0] + ": " + row[1]);
        require(!row[2].empty(), "claim_text is required for " + row[0]);
        require(hasEvidence(row[9]), "claim evidence_ref is required for " + row[0]);
        Claim claim;
        claim.id = row[0];
        claim.type = row[1];
        claim.text = row[2];
        claim.scopeMatches = parseBoolean(row[3], "scope_matches for " + row[0]);
        claim.functionalUnitEquivalent =
            parseBoolean(row[4], "functional_unit_equivalent for " + row[0]);
        claim.boundaryEquivalent = parseBoolean(row[5], "boundary_equivalent for " + row[0]);
        claim.qualityEquivalent = parseBoolean(row[6], "quality_equivalent for " + row[0]);
        claim.methodEquivalent = parseBoolean(row[7], "method_equivalent for " + row[0]);
        claim.legalTechnicalEvidence =
            parseBoolean(row[8], "legal_technical_evidence for " + row[0]);
        claim.evidence = row[9];
        claims.push_back(std::move(claim));
    }
    std::sort(claims.begin(), claims.end(),
              [](const Claim &left, const Claim &right) { return left.id < right.id; });
    return claims;
}

struct ProfileEvidence {
    std::string productName;
    std::string productVersion;
    std::string softwareClass;
    std::string functionalUnit;
    std::string boundary;
    std::string workload;
    bool aiInScope = false;
};

const Json *
linkedDeclaration(const std::map<std::pair<std::string, std::string>, const Json *> &declarations,
                  std::string kind, std::string name) {
    const auto found = declarations.find({kind, name});
    require(found != declarations.end(),
            "profile.json certification_profile link is unresolved: " + kind + " " + name);
    return found->second;
}

std::string typedDeclarationString(const Json &declaration, const std::string &field,
                                   const std::string &expectedType) {
    const Json &typedFields = jsonMember(declaration, "typed_fields");
    const Json &values = jsonMember(typedFields, field);
    require(values.kind == Json::Kind::Array && values.array.size() == 1 &&
                values.array.front().kind == Json::Kind::Object,
            "profile.json linked declaration must contain one typed value: " + field);
    const Json &value = values.array.front();
    require(jsonString(value, "type") == expectedType,
            "profile.json linked declaration has wrong typed value: " + field);
    return jsonString(value, "value");
}

ProfileEvidence validateProfile(const Json &profile) {
    require(jsonString(profile, "schema") == kProfileSchema,
            "profile.json schema must be shorthand.c3eco.candidate_report.v1");
    require(jsonString(profile, "report_status") == "candidate_assessment_only",
            "profile.json must be a candidate assessment report");
    require(jsonString(profile, "c3eco_language_contract") == "shorthand.c3eco.language.v1",
            "profile.json must use shorthand.c3eco.language.v1");
    require(jsonString(profile, "c3eco_profile_contract") == kProfileContract,
            "profile.json must use shorthand.c3eco.profile.v2");
    require(jsonString(profile, "c3eco_profile_status") == "conformant",
            "profile.json must contain a conformant typed profile");
    require(!jsonBoolean(profile, "c3eco_profile_migration_required"),
            "profile.json cannot require profile migration");
    require(!jsonBoolean(profile, "official_certification_granted"),
            "profile.json must not claim official certification");
    for (const char *unsafeFlag : {"production_claim", "level_claim_permitted", "certified"}) {
        const Json *value = findJsonMember(profile, unsafeFlag);
        if (value != nullptr) {
            require(value->kind == Json::Kind::Boolean,
                    std::string("profile.json optional claim flag must be boolean: ") + unsafeFlag);
            require(!value->boolean,
                    std::string("profile.json must not enable claim flag: ") + unsafeFlag);
        }
    }
    requireNonemptyJsonString(profile, "source_file");
    requireNonemptyJsonString(profile, "compiler_version");
    (void)jsonBoolean(profile, "minimum_c3eco_evidence_present");
    (void)jsonString(profile, "workload");
    (void)jsonString(profile, "functional_unit");
    (void)jsonString(profile, "success_criteria");
    const std::string profileMq = jsonString(profile, "measurement_quality");
    const std::string profileDq = jsonString(profile, "data_quality");
    const bool profileOnly = !jsonBoolean(profile, "minimum_c3eco_evidence_present") &&
                             jsonString(profile, "measurement_status") == "declared_budget_only";
    require((profileOnly && profileMq.empty()) ||
                (profileMq.size() == 3 && profileMq[0] == 'M' && profileMq[1] == 'Q' &&
                 profileMq[2] >= '1' && profileMq[2] <= '4'),
            "profile.json measurement_quality must be MQ1 through MQ4");
    require((profileOnly && profileDq.empty()) ||
                (profileDq.size() == 3 && profileDq[0] == 'D' && profileDq[1] == 'Q' &&
                 profileDq[2] >= '1' && profileDq[2] <= '4'),
            "profile.json data_quality must be DQ1 through DQ4");
    require(jsonNumber(profile, "carbon_factor_gco2e_per_kwh") >= 0.0,
            "profile.json carbon factor cannot be negative");
    const std::set<std::string> measurementStatuses = {"declared_budget_only", "measured",
                                                       "estimated", "unavailable"};
    require(measurementStatuses.count(jsonString(profile, "measurement_status")) == 1,
            "profile.json measurement_status is invalid");
    requireNonemptyJsonString(profile, "runtime_backend");
    const std::set<std::string> inferenceStatuses = {"not_executed", "executed",
                                                     "backend_unavailable", "fallback"};
    require(inferenceStatuses.count(jsonString(profile, "inference_status")) == 1,
            "profile.json inference_status is invalid");
    const Json &blockedItems = jsonMember(profile, "blocked_certification_items");
    require(blockedItems.kind == Json::Kind::Array,
            "profile.json blocked_certification_items must be an array");
    bool externalCertifierBlocked = false;
    for (const Json &item : blockedItems.array) {
        require(item.kind == Json::Kind::String && !item.string.empty(),
                "profile.json blocked certification items must be non-empty strings");
        if (item.string == "external_certifier_not_signed")
            externalCertifierBlocked = true;
    }
    require(externalCertifierBlocked,
            "profile.json must preserve external_certifier_not_signed as a blocker");
    requireNonemptyJsonString(profile, "claim_safe_text");
    requireNonemptyJsonString(profile, "disclaimer");

    const Json &declarations = jsonMember(profile, "c3eco_declarations");
    require(declarations.kind == Json::Kind::Array,
            "profile.json c3eco_declarations must be an array");
    std::size_t profileCount = 0;
    std::map<std::pair<std::string, std::string>, const Json *> declarationIndex;
    std::map<std::string, std::string> profileLinks;
    for (const Json &declaration : declarations.array) {
        require(declaration.kind == Json::Kind::Object,
                "profile.json declarations must be objects");
        const std::string kind = jsonString(declaration, "kind");
        requireNonemptyJsonString(declaration, "name");
        require(jsonMember(declaration, "fields").kind == Json::Kind::Object,
                "profile.json declaration fields must be an object");
        require(jsonMember(declaration, "typed_fields").kind == Json::Kind::Object,
                "profile.json declaration typed_fields must be an object");
        require(declarationIndex
                    .emplace(std::make_pair(kind, jsonString(declaration, "name")), &declaration)
                    .second,
                "profile.json declaration kind/name pairs must be unique");
        if (kind != "certification_profile")
            continue;

        const Json &fields = jsonMember(declaration, "fields");
        const Json &typedFields = jsonMember(declaration, "typed_fields");
        const std::set<std::string> requiredProfileFields = {
            "profile_version", "certification", "functional_unit", "workload",   "boundary",
            "ai_lifecycle",    "guardrails",    "valid_from",      "valid_until"};
        requireExactKeys(fields, requiredProfileFields,
                         "profile.json certification_profile fields");
        requireExactKeys(typedFields, requiredProfileFields,
                         "profile.json certification_profile typed_fields");
        std::map<std::string, std::string> typedStrings;
        for (const std::string &key : requiredProfileFields) {
            const Json &legacyValues = jsonMember(fields, key);
            const Json &typedValues = jsonMember(typedFields, key);
            require(legacyValues.kind == Json::Kind::Array && legacyValues.array.size() == 1 &&
                        legacyValues.array.front().kind == Json::Kind::String &&
                        !legacyValues.array.front().string.empty(),
                    "profile.json certification_profile field must contain one value: " + key);
            require(typedValues.kind == Json::Kind::Array && typedValues.array.size() == 1 &&
                        typedValues.array.front().kind == Json::Kind::Object,
                    "profile.json certification_profile typed field must contain one value: " +
                        key);
            const Json &typedValue = typedValues.array.front();
            requireExactKeys(typedValue, {"type", "value"},
                             "profile.json certification_profile typed value " + key);
            const std::string type = jsonString(typedValue, "type");
            const Json &value = jsonMember(typedValue, "value");
            if (key == "profile_version") {
                require(
                    type == "integer" && value.kind == Json::Kind::Number && value.number == 2.0 &&
                        legacyValues.array.front().string == "2",
                    "profile.json certification_profile profile_version must be typed integer 2");
            } else {
                require(type == ((key == "valid_from" || key == "valid_until") ? "string"
                                                                               : "identifier") &&
                            value.kind == Json::Kind::String && !value.string.empty() &&
                            value.string == legacyValues.array.front().string,
                        "profile.json certification_profile typed value is invalid: " + key);
                typedStrings.emplace(key, value.string);
            }
        }
        require(validIsoDate(typedStrings.at("valid_from")) &&
                    validIsoDate(typedStrings.at("valid_until")) &&
                    typedStrings.at("valid_from") <= typedStrings.at("valid_until"),
                "profile.json certification_profile validity dates are invalid");
        profileLinks = typedStrings;
        ++profileCount;
    }
    require(profileCount == 1,
            "profile.json must contain exactly one typed certification_profile declaration");
    const Json &certification =
        *linkedDeclaration(declarationIndex, "certification", profileLinks.at("certification"));
    for (const std::string kind :
         {"functional_unit", "workload", "boundary", "ai_lifecycle", "guardrails"}) {
        (void)linkedDeclaration(declarationIndex, kind, profileLinks.at(kind));
    }
    const Json &functionalUnit =
        *linkedDeclaration(declarationIndex, "functional_unit", profileLinks.at("functional_unit"));
    const Json &denominators =
        jsonMember(jsonMember(functionalUnit, "typed_fields"), "denominator");
    require(denominators.kind == Json::Kind::Array && denominators.array.size() == 1,
            "profile.json functional-unit denominator must contain one value");
    const Json &denominator = denominators.array.front();
    require(jsonString(denominator, "type") == "integer" && jsonNumber(denominator, "value") > 0 &&
                std::floor(jsonNumber(denominator, "value")) == jsonNumber(denominator, "value"),
            "profile.json functional-unit denominator must be a positive integer");
    ProfileEvidence evidence;
    evidence.productName = profileLinks.at("certification");
    evidence.productVersion = typedDeclarationString(certification, "version", "string");
    evidence.softwareClass = typedDeclarationString(certification, "software_class", "identifier");
    evidence.functionalUnit = profileLinks.at("functional_unit");
    evidence.boundary = profileLinks.at("boundary");
    evidence.workload = profileLinks.at("workload");
    evidence.aiInScope =
        evidence.softwareClass == "S6" || evidence.softwareClass.rfind("S6_", 0) == 0;
    return evidence;
}

struct MeasurementEvidence {
    int measurementQuality = 4;
    int dataQuality = 4;
    double uncertaintyPercent = 0.0;
};

int workbookQualityRank(const std::string &value) {
    if (value == "high")
        return 3;
    if (value == "medium")
        return 2;
    if (value == "low")
        return 1;
    throw std::runtime_error("measurement.json MQ/DQ must be high, medium or low");
}

MeasurementEvidence validateMeasurement(const Json &measurement) {
    const std::set<std::string> topLevelKeys = {"schema",
                                                "measurement_status",
                                                "official_certification_granted",
                                                "base_footprint_not_reduced_by_offsets",
                                                "allocation_policy",
                                                "record_count",
                                                "totals",
                                                "records",
                                                "claim_safe_text",
                                                "next_qualification"};
    requireExactKeys(measurement, topLevelKeys, "measurement.json");
    require(jsonString(measurement, "schema") == kMeasurementSchema,
            "measurement.json must use shorthand.c3eco.measurement_workbook.v1");
    require(jsonString(measurement, "measurement_status") == "measured_instrumented",
            "measurement.json must contain instrumented measured evidence");
    require(!jsonBoolean(measurement, "official_certification_granted"),
            "measurement.json must not claim official certification");
    require(jsonBoolean(measurement, "base_footprint_not_reduced_by_offsets"),
            "measurement.json must keep offsets outside the base footprint");
    requireNonemptyJsonString(measurement, "allocation_policy");
    requireNonemptyJsonString(measurement, "claim_safe_text");
    requireNonemptyJsonString(measurement, "next_qualification");

    const double recordCount = jsonNumber(measurement, "record_count");
    require(recordCount >= 1.0 && std::floor(recordCount) == recordCount,
            "measurement.json record_count must be a positive integer");
    const Json &totals = jsonMember(measurement, "totals");
    requireExactKeys(totals,
                     {"allocated_it_energy_j", "facility_energy_kwh", "carbon_kgco2e",
                      "uncertainty_kwh", "uncertainty_carbon_kgco2e", "cost_by_currency"},
                     "measurement.json totals");
    const double totalItEnergy = jsonNumber(totals, "allocated_it_energy_j");
    const double totalFacilityEnergy = jsonNumber(totals, "facility_energy_kwh");
    const double totalCarbon = jsonNumber(totals, "carbon_kgco2e");
    const double totalUncertainty = jsonNumber(totals, "uncertainty_kwh");
    const double totalCarbonUncertainty = jsonNumber(totals, "uncertainty_carbon_kgco2e");
    require(totalItEnergy > 0.0, "measurement.json allocated IT energy must be positive");
    require(totalFacilityEnergy > 0.0, "measurement.json facility energy must be positive");
    require(totalCarbon > 0.0, "measurement.json carbon must be positive");
    require(totalUncertainty >= 0.0 && totalCarbonUncertainty >= 0.0,
            "measurement.json uncertainty totals cannot be negative");
    const Json &totalCosts = jsonMember(totals, "cost_by_currency");
    require(totalCosts.kind == Json::Kind::Object,
            "measurement.json cost_by_currency must be an object");

    const Json &records = jsonMember(measurement, "records");
    require(records.kind == Json::Kind::Array, "measurement.json records must be an array");
    require(recordCount == static_cast<double>(records.array.size()),
            "measurement.json record_count must equal records length");
    const std::set<std::string> measuredSources = {"physical_meter", "rapl", "accelerator_counter",
                                                   "cloud_meter"};
    const std::set<std::string> qualityValues = {"high", "medium", "low"};
    const std::set<std::string> recordKeys = {"record_id",
                                              "component",
                                              "source_kind",
                                              "instrument_id",
                                              "calibration_id",
                                              "calibration_date",
                                              "measured_at",
                                              "raw_energy_j",
                                              "allocation_fraction",
                                              "allocated_it_energy_j",
                                              "pue",
                                              "facility_energy_kwh",
                                              "carbon_factor_gco2e_per_kwh",
                                              "factor_source",
                                              "factor_date",
                                              "carbon_kgco2e",
                                              "tariff_per_kwh",
                                              "tariff_currency",
                                              "tariff_source",
                                              "cost",
                                              "uncertainty_percent",
                                              "uncertainty_kwh",
                                              "uncertainty_carbon_kgco2e",
                                              "measurement_quality",
                                              "data_quality",
                                              "evidence_ref"};
    std::set<std::string> recordIds;
    std::map<std::tuple<std::string, std::string, double, std::string>, double> allocation;
    std::map<std::string, double> summedCosts;
    double summedItEnergy = 0.0;
    double summedFacilityEnergy = 0.0;
    double summedCarbon = 0.0;
    double summedUncertainty = 0.0;
    double summedCarbonUncertainty = 0.0;
    MeasurementEvidence result;
    for (const Json &record : records.array) {
        requireExactKeys(record, recordKeys, "measurement.json record");
        const std::string id = jsonString(record, "record_id");
        require(!id.empty() && recordIds.insert(id).second,
                "measurement.json record_id values must be non-empty and unique");
        requireNonemptyJsonString(record, "component");
        require(measuredSources.count(jsonString(record, "source_kind")) == 1,
                "measurement.json records must use instrumented source kinds");
        const std::string instrument = jsonString(record, "instrument_id");
        const std::string calibration = jsonString(record, "calibration_id");
        const std::string calibrationDate = jsonString(record, "calibration_date");
        const std::string measuredAt = jsonString(record, "measured_at");
        const std::string factorSource = jsonString(record, "factor_source");
        const std::string factorDate = jsonString(record, "factor_date");
        const std::string tariffCurrency = jsonString(record, "tariff_currency");
        const std::string tariffSource = jsonString(record, "tariff_source");
        const std::string evidence = jsonString(record, "evidence_ref");
        require(!instrument.empty() && !calibration.empty() && !factorSource.empty() &&
                    !tariffSource.empty() && hasEvidence(evidence),
                "measurement.json record provenance fields are required");
        require(validIsoDate(calibrationDate) && validIsoDate(factorDate) &&
                    measuredAt.size() >= 10 && validIsoDate(measuredAt.substr(0, 10)),
                "measurement.json record dates are invalid");
        require(calibrationDate <= measuredAt.substr(0, 10),
                "measurement.json calibration cannot post-date measurement");
        require(factorDate <= measuredAt.substr(0, 10),
                "measurement.json carbon factor cannot post-date measurement");

        const double rawEnergy = jsonNumber(record, "raw_energy_j");
        const double allocationFraction = jsonNumber(record, "allocation_fraction");
        const double itEnergy = jsonNumber(record, "allocated_it_energy_j");
        const double pue = jsonNumber(record, "pue");
        const double facilityEnergy = jsonNumber(record, "facility_energy_kwh");
        const double carbonFactor = jsonNumber(record, "carbon_factor_gco2e_per_kwh");
        const double carbon = jsonNumber(record, "carbon_kgco2e");
        const double tariff = jsonNumber(record, "tariff_per_kwh");
        const double cost = jsonNumber(record, "cost");
        const double uncertaintyPercent = jsonNumber(record, "uncertainty_percent");
        const double uncertainty = jsonNumber(record, "uncertainty_kwh");
        const double carbonUncertainty = jsonNumber(record, "uncertainty_carbon_kgco2e");
        require(rawEnergy > 0.0 && allocationFraction > 0.0 && allocationFraction <= 1.0,
                "measurement.json raw energy/allocation values are invalid");
        require(itEnergy > 0.0 && facilityEnergy > 0.0 && carbon > 0.0,
                "measurement.json record accounting values must be positive");
        require(pue >= 1.0 && pue <= 3.0, "measurement.json pue must be in [1,3]");
        require(carbonFactor > 0.0 && carbonFactor <= 2500.0,
                "measurement.json carbon factor is outside the bounded range");
        require(tariff >= 0.0 && tariff <= 10000.0 && tariffCurrency.size() == 3 && cost >= 0.0,
                "measurement.json tariff values are invalid");
        require(std::all_of(tariffCurrency.begin(), tariffCurrency.end(),
                            [](char c) { return c >= 'A' && c <= 'Z'; }),
                "measurement.json tariff currency must be three uppercase ASCII letters");
        require(uncertaintyPercent >= 0.0 && uncertaintyPercent <= 100.0 && uncertainty >= 0.0 &&
                    carbonUncertainty >= 0.0,
                "measurement.json uncertainty values are invalid");
        require(qualityValues.count(jsonString(record, "measurement_quality")) == 1 &&
                    qualityValues.count(jsonString(record, "data_quality")) == 1,
                "measurement.json MQ/DQ must be high, medium or low");
        result.measurementQuality =
            std::min(result.measurementQuality,
                     workbookQualityRank(jsonString(record, "measurement_quality")));
        result.dataQuality =
            std::min(result.dataQuality, workbookQualityRank(jsonString(record, "data_quality")));
        result.uncertaintyPercent = std::max(result.uncertaintyPercent, uncertaintyPercent);
        require(reconciles(itEnergy, rawEnergy * allocationFraction),
                "measurement.json allocated IT energy does not reconcile within record " + id);
        require(reconciles(facilityEnergy, (itEnergy * pue) / 3600000.0),
                "measurement.json facility energy does not reconcile within record " + id);
        require(reconciles(carbon, (facilityEnergy * carbonFactor) / 1000.0),
                "measurement.json carbon does not reconcile within record " + id);
        require(reconciles(cost, facilityEnergy * tariff),
                "measurement.json cost does not reconcile within record " + id);
        require(reconciles(uncertainty, facilityEnergy * uncertaintyPercent / 100.0),
                "measurement.json uncertainty does not reconcile within record " + id);
        require(reconciles(carbonUncertainty, carbon * uncertaintyPercent / 100.0),
                "measurement.json carbon uncertainty does not reconcile within record " + id);

        const auto allocationKey = std::make_tuple(instrument, measuredAt, rawEnergy, evidence);
        allocation[allocationKey] += allocationFraction;
        require(allocation[allocationKey] <= 1.0 + kEpsilon,
                "measurement.json double counting detected for shared instrument evidence");
        summedItEnergy += itEnergy;
        summedFacilityEnergy += facilityEnergy;
        summedCarbon += carbon;
        summedUncertainty += uncertainty;
        summedCarbonUncertainty += carbonUncertainty;
        summedCosts[tariffCurrency] += cost;
    }
    require(reconciles(summedItEnergy, totalItEnergy),
            "measurement.json allocated IT energy does not reconcile with records");
    require(reconciles(summedFacilityEnergy, totalFacilityEnergy),
            "measurement.json facility energy does not reconcile with records");
    require(reconciles(summedCarbon, totalCarbon),
            "measurement.json carbon does not reconcile with records");
    require(reconciles(summedUncertainty, totalUncertainty),
            "measurement.json uncertainty does not reconcile with records");
    require(reconciles(summedCarbonUncertainty, totalCarbonUncertainty),
            "measurement.json carbon uncertainty does not reconcile with records");
    require(totalCosts.object.size() == summedCosts.size(),
            "measurement.json cost currencies do not reconcile with records");
    for (const auto &entry : summedCosts) {
        const auto found = totalCosts.object.find(entry.first);
        require(found != totalCosts.object.end() && found->second.kind == Json::Kind::Number &&
                    found->second.number >= 0.0 && reconciles(found->second.number, entry.second),
                "measurement.json cost does not reconcile for currency " + entry.first);
    }
    return result;
}

void validateEvidenceArtifacts(const std::string &directory, Metadata &metadata) {
    const Json profile = JsonParser(readFile(joinPath(directory, "profile.json"))).parse();
    const ProfileEvidence profileEvidence = validateProfile(profile);
    const Json measurement = JsonParser(readFile(joinPath(directory, "measurement.json"))).parse();
    const MeasurementEvidence measurementEvidence = validateMeasurement(measurement);
    require(metadata.productName == profileEvidence.productName &&
                metadata.productVersion == profileEvidence.productVersion &&
                metadata.softwareClass == profileEvidence.softwareClass &&
                metadata.functionalUnit == profileEvidence.functionalUnit &&
                metadata.boundary == profileEvidence.boundary &&
                metadata.workload == profileEvidence.workload,
            "metadata identity and scope must match the linked typed profile");
    require(!profileEvidence.aiInScope || metadata.aiInScope,
            "AI software class cannot disable AI scope in metadata");
    const auto values = loadMetadataValues(joinPath(directory, "metadata.tsv"));
    if (metadata.measurementQuality > measurementEvidence.measurementQuality) {
        require(metadata.independentReview &&
                    values.at("measurement_quality").evidenceStatus == "independent",
                "raising workbook MQ requires independent review evidence");
    }
    if (metadata.dataQuality > measurementEvidence.dataQuality) {
        require(metadata.independentReview &&
                    values.at("data_quality").evidenceStatus == "independent",
                "raising workbook DQ requires independent review evidence");
    }
    metadata.uncertaintyPercent =
        std::max(metadata.uncertaintyPercent, measurementEvidence.uncertaintyPercent);
}

void writeStringArray(std::ostream &out, const std::vector<std::string> &values) {
    out << '[';
    for (std::size_t index = 0; index < values.size(); ++index) {
        if (index)
            out << ',';
        out << '"' << jsonEscape(values[index]) << '"';
    }
    out << ']';
}

void writeAssessment(const std::string &path, const Metadata &metadata,
                     const std::map<std::string, Gate> &gates,
                     const std::vector<Criterion> &criteria,
                     const std::vector<DomainScore> &domains, const MaterialityResult &materiality,
                     const std::vector<Regression> &regressions, const std::vector<Claim> &claims,
                     const AssessmentDecision &decision) {
    std::ofstream out(path, std::ios::binary);
    require(static_cast<bool>(out), "cannot open assessment output: " + path);
    out << std::setprecision(12);
    out << "{\n"
        << "  \"schema\":\"" << kAssessmentSchema << "\",\n"
        << "  \"decision_kind\":\"candidate_recommendation_only\",\n"
        << "  \"official_certification_granted\":false,\n"
        << "  \"production_claim\":false,\n"
        << "  \"comparative_energy_claim\":false,\n"
        << "  \"product\":{\"name\":\"" << jsonEscape(metadata.productName) << "\",\"version\":\""
        << jsonEscape(metadata.productVersion) << "\",\"software_class\":\""
        << jsonEscape(metadata.softwareClass) << "\",\"functional_unit\":\""
        << jsonEscape(metadata.functionalUnit) << "\",\"boundary\":\""
        << jsonEscape(metadata.boundary) << "\",\"workload\":\"" << jsonEscape(metadata.workload)
        << "\"},\n"
        << "  \"evidence_contracts\":{\"profile\":\"" << kProfileContract << "\",\"measurement\":\""
        << kMeasurementSchema << "\"},\n"
        << "  \"assessment_controls\":{\"measurement_quality\":\"MQ" << metadata.measurementQuality
        << "\",\"data_quality\":\"DQ" << metadata.dataQuality
        << "\",\"uncertainty_percent\":" << metadata.uncertaintyPercent
        << ",\"penalty_points\":" << metadata.penaltyPoints
        << ",\"client_device_impact\":" << (metadata.clientDeviceImpact ? "true" : "false")
        << ",\"eco_regression_control_defined\":"
        << (metadata.ecoRegressionControlDefined ? "true" : "false")
        << ",\"independent_review\":" << (metadata.independentReview ? "true" : "false")
        << ",\"public_report\":" << (metadata.publicReport ? "true" : "false")
        << ",\"continuous_telemetry\":" << (metadata.continuousTelemetry ? "true" : "false")
        << ",\"public_evidence\":" << (metadata.publicEvidence ? "true" : "false")
        << ",\"high_severity_claim_risk\":" << (metadata.highSeverityClaimRisk ? "true" : "false")
        << ",\"high_scale_saas\":" << (metadata.highScaleSaas ? "true" : "false") << "},\n"
        << "  \"eligibility\":{\"status\":\""
        << (decision.eligible ? "eligible_for_external_review" : "not_eligible")
        << "\",\"all_mandatory_gates_passed\":" << (decision.eligible ? "true" : "false")
        << ",\"failed_gates\":";
    writeStringArray(out, decision.failedGates);
    out << ",\"gates\":[";
    bool first = true;
    for (int number = 1; number <= 14; ++number) {
        if (!first)
            out << ',';
        first = false;
        const Gate &gate = gates.at("G" + std::to_string(number));
        out << "{\"id\":\"" << gate.id << "\",\"input_status\":\""
            << (gate.inputPass ? "pass" : "fail") << "\",\"effective_status\":\""
            << (gate.effectivePass ? "pass" : "fail") << "\",\"evidence_ref\":\""
            << jsonEscape(gate.evidence) << "\"}";
    }
    out << "]},\n"
        << "  \"scoring\":{\"normative_weights_sum\":100,\"reallocated_weights_sum\":100,"
        << "\"total_score\":" << decision.totalScore << ",\"score_ceiling_level\":\""
        << levelName(decision.scoreCeiling) << "\",\"recommended_level_for_external_review\":\""
        << levelName(decision.recommendation) << "\",\"level_claim_permitted\":false,\"domains\":[";
    first = true;
    for (const DomainScore &domain : domains) {
        if (!first)
            out << ',';
        first = false;
        out << "{\"domain\":\"" << domain.domain
            << "\",\"applicable\":" << (domain.applicable ? "true" : "false")
            << ",\"normative_weight\":" << domain.normativeWeight
            << ",\"adjusted_weight\":" << domain.adjustedWeight
            << ",\"domain_percent\":" << domain.percent << ",\"weighted_points\":" << domain.points
            << ",\"applicable_criteria\":" << domain.criterionCount << '}';
    }
    out << "],\"criteria\":[";
    first = true;
    for (const Criterion &criterion : criteria) {
        if (!first)
            out << ',';
        first = false;
        out << "{\"id\":\"" << jsonEscape(criterion.id) << "\",\"domain\":\"" << criterion.domain
            << "\",\"raw_score\":" << criterion.rawScore
            << ",\"effective_score\":" << criterion.effectiveScore
            << ",\"criterion_weight\":" << criterion.weight << ",\"evidence_status\":\""
            << criterion.evidenceStatus
            << "\",\"applicable\":" << (criterion.applicable ? "true" : "false")
            << ",\"auditor_approval_ref\":";
        if (hasEvidence(criterion.approval))
            out << '"' << jsonEscape(criterion.approval) << '"';
        else
            out << "null";
        out << ",\"evidence_ref\":\"" << jsonEscape(criterion.evidence) << "\"}";
    }
    out << "],\"evidence_caps_applied\":";
    writeStringArray(out, decision.evidenceCaps);
    out << ",\"decision_reasons\":";
    writeStringArray(out, decision.decisionReasons);
    out << "},\n"
        << "  "
           "\"materiality\":{\"individual_threshold_percent\":1,\"cumulative_omission_limit_"
           "percent\":5,"
        << "\"declared_share_percent\":" << materiality.declaredSharePercent
        << ",\"omitted_share_percent\":" << materiality.omittedSharePercent
        << ",\"individual_material_omission\":"
        << (materiality.individualMaterialOmission ? "true" : "false")
        << ",\"cumulative_omission_exceeded\":"
        << (materiality.cumulativeOmissionExceeded ? "true" : "false") << ",\"components\":[";
    first = true;
    for (const MaterialityComponent &component : materiality.components) {
        if (!first)
            out << ',';
        first = false;
        out << "{\"component\":\"" << jsonEscape(component.component)
            << "\",\"share_percent\":" << component.sharePercent << ",\"disposition\":\""
            << component.disposition << "\",\"evidence_ref\":\"" << jsonEscape(component.evidence)
            << "\"}";
    }
    out << "]},\n"
        << "  \"ai_route\":{\"ai_in_scope\":" << (metadata.aiInScope ? "true" : "false")
        << ",\"role\":\"" << jsonEscape(metadata.aiRole) << "\",\"training_boundary_declared\":"
        << (metadata.trainingBoundaryDeclared ? "true" : "false")
        << ",\"inference_boundary_declared\":"
        << (metadata.inferenceBoundaryDeclared ? "true" : "false")
        << ",\"token_or_inference_metric_declared\":"
        << (metadata.tokenOrInferenceMetricDeclared ? "true" : "false")
        << ",\"exclusions_declared\":" << (metadata.aiExclusionsDeclared ? "true" : "false")
        << ",\"quality_energy_frontier_options\":" << metadata.qualityEnergyFrontierOptions
        << "},\n"
        << "  \"claims\":[";
    first = true;
    for (const Claim &claim : claims) {
        if (!first)
            out << ',';
        first = false;
        out << "{\"id\":\"" << jsonEscape(claim.id) << "\",\"type\":\"" << claim.type
            << "\",\"text\":\"" << jsonEscape(claim.text) << "\",\"decision\":\""
            << (claim.permitted ? "permitted" : "blocked") << "\",\"reasons\":";
        writeStringArray(out, claim.reasons);
        out << ",\"evidence_ref\":\"" << jsonEscape(claim.evidence) << "\"}";
    }
    out << "],\n"
        << "  \"eco_regression\":{\"threshold_percent\":10,\"triggered\":"
        << (decision.ecoRegressionTriggered ? "true" : "false")
        << ",\"unresolved\":" << (decision.unresolvedEcoRegression ? "true" : "false")
        << ",\"corrective_action_required\":"
        << (decision.ecoRegressionTriggered ? "true" : "false") << ",\"metrics\":[";
    first = true;
    for (const Regression &regression : regressions) {
        if (!first)
            out << ',';
        first = false;
        out << "{\"id\":\"" << jsonEscape(regression.id) << "\",\"kind\":\"" << regression.kind
            << "\",\"baseline_value\":" << regression.baseline
            << ",\"current_value\":" << regression.current
            << ",\"lower_is_better\":" << (regression.lowerIsBetter ? "true" : "false")
            << ",\"deterioration_percent\":" << regression.deteriorationPercent
            << ",\"threshold_exceeded\":" << (regression.thresholdExceeded ? "true" : "false")
            << ",\"unresolved\":" << (regression.unresolved ? "true" : "false")
            << ",\"explanation\":";
        if (hasEvidence(regression.explanation))
            out << '"' << jsonEscape(regression.explanation) << '"';
        else
            out << "null";
        out << ",\"corrective_action_ref\":";
        if (hasEvidence(regression.correctiveAction))
            out << '"' << jsonEscape(regression.correctiveAction) << '"';
        else
            out << "null";
        out << ",\"evidence_ref\":\"" << jsonEscape(regression.evidence) << "\"}";
    }
    const int surveillanceMonths = (metadata.aiInScope || metadata.highScaleSaas) ? 6 : 12;
    out << "]},\n"
        << "  \"surveillance\":{\"cadence_months\":" << surveillanceMonths
        << ",\"recertification_review_required\":"
        << ((decision.ecoRegressionTriggered || decision.qualityRegression) ? "true" : "false")
        << ",\"major_change_triggers\":[\"architecture\",\"model\",\"runtime\",\"database\","
           "\"cloud_region\",\"provider\",\"hardware\",\"traffic\"]},\n"
        << "  \"claim_safe_text\":\"Candidate assessment only. A qualified external authority must "
           "review evidence before any C3-ECO certification or level claim.\",\n"
        << "  \"disclaimer\":\"This tool evaluates candidate evidence and never grants official "
           "certification.\"\n"
        << "}\n";
    out.flush();
    require(static_cast<bool>(out), "cannot write assessment output: " + path);
    out.close();
    require(static_cast<bool>(out), "cannot close assessment output: " + path);
}

std::string markdownEscape(const std::string &value) {
    std::string result;
    for (unsigned char c : value) {
        if (c < 0x20 || c == 0x7f) {
            result += ' ';
        } else {
            if (std::string("\\`*_{}[]<>()#+-.!|").find(static_cast<char>(c)) != std::string::npos)
                result += '\\';
            result += static_cast<char>(c);
        }
    }
    return result;
}

void writeAssessmentMarkdown(const std::string &path, const Metadata &metadata,
                             const std::vector<DomainScore> &domains,
                             const AssessmentDecision &decision, const std::vector<Claim> &claims) {
    std::ofstream out(path, std::ios::binary);
    require(static_cast<bool>(out), "cannot open assessment Markdown output: " + path);
    out << std::fixed << std::setprecision(2);
    out << "# C3-ECO candidate assessment\n\n"
        << "This report does **not** grant C3-ECO certification or permission to publish a level "
           "claim.\n\n"
        << "- Product identity: " << markdownEscape(metadata.productName) << "\n"
        << "- Version: " << markdownEscape(metadata.productVersion) << "\n"
        << "- Software class: " << markdownEscape(metadata.softwareClass) << "\n"
        << "- Functional unit: " << markdownEscape(metadata.functionalUnit) << "\n"
        << "- Boundary: " << markdownEscape(metadata.boundary) << "\n"
        << "- Workload: " << markdownEscape(metadata.workload) << "\n"
        << "- Mandatory gates: " << (decision.eligible ? "PASS" : "FAIL") << "\n"
        << "- Diagnostic weighted score after penalties: " << decision.totalScore << "/100\n"
        << "- Penalty points: " << metadata.penaltyPoints << "\n"
        << "- Recommendation for external review: " << levelName(decision.recommendation) << "\n"
        << "- Effective evidence quality: MQ" << metadata.measurementQuality << "/DQ"
        << metadata.dataQuality << "\n"
        << "- Conservative uncertainty: " << metadata.uncertaintyPercent << "%\n"
        << "- Surveillance cadence: " << ((metadata.aiInScope || metadata.highScaleSaas) ? 6 : 12)
        << " months\n\n"
        << "## Domain scores\n\n"
        << "| Domain | Applicable | Effective weight | Score | Weighted points |\n"
        << "| --- | --- | ---: | ---: | ---: |\n";
    for (const DomainScore &domain : domains) {
        out << "| " << domain.domain << " | " << (domain.applicable ? "yes" : "no") << " | "
            << domain.adjustedWeight << " | " << domain.percent << "% | " << domain.points
            << " |\n";
    }
    out << "\n## Requested claims\n\n";
    for (const Claim &claim : claims) {
        out << "- " << markdownEscape(claim.id) << ": "
            << (claim.permitted ? "permitted" : "blocked") << ". " << markdownEscape(claim.text)
            << "\n";
        for (const std::string &reason : claim.reasons)
            out << "  - " << markdownEscape(reason) << "\n";
    }
    out << "\n## Decision reasons\n\n";
    for (const std::string &reason : decision.decisionReasons)
        out << "- " << markdownEscape(reason) << "\n";
    for (const std::string &gate : decision.failedGates)
        out << "- Failed gate: " << gate << "\n";
    out << "\nOfficial certification granted: **false**. Production claim: **false**. "
           "Comparative energy claim: **false**. External authority review remains separate.\n";
    out.flush();
    require(static_cast<bool>(out), "cannot write assessment Markdown output: " + path);
    out.close();
    require(static_cast<bool>(out), "cannot close assessment Markdown output: " + path);
}

void assessDirectory(const std::string &directory, const std::string &output,
                     const std::string &markdownOutput) {
    Metadata metadata = loadMetadata(joinPath(directory, "metadata.tsv"));
    validateEvidenceArtifacts(directory, metadata);
    std::map<std::string, Gate> gates = loadGates(joinPath(directory, "gates.tsv"));
    const std::vector<Criterion> criteria =
        loadCriteria(joinPath(directory, "criteria.tsv"), metadata);
    const MaterialityResult materiality = loadMateriality(joinPath(directory, "materiality.tsv"));
    const std::vector<Regression> regressions =
        loadRegressions(joinPath(directory, "regressions.tsv"));
    std::vector<Claim> claims = loadClaims(joinPath(directory, "claims.tsv"));
    const std::vector<DomainScore> domains = scoreDomains(criteria);
    AssessmentDecision decision =
        decide(metadata, gates, criteria, domains, materiality, regressions);
    evaluateClaims(claims, metadata, decision);
    if (std::any_of(claims.begin(), claims.end(),
                    [](const Claim &claim) { return !claim.permitted; })) {
        gates.at("G10").effectivePass = false;
        decision = decide(metadata, gates, criteria, domains, materiality, regressions);
        evaluateClaims(claims, metadata, decision);
    }
    writeAssessment(output, metadata, gates, criteria, domains, materiality, regressions, claims,
                    decision);
    if (!markdownOutput.empty())
        writeAssessmentMarkdown(markdownOutput, metadata, domains, decision, claims);
}

int run(int argc, char **argv) {
    require(argc == 3 || argc == 4,
        "usage: shorthand_c3eco_assess <candidate-directory> <assessment-output.json> [report.md]");
    assessDirectory(argv[1], argv[2], argc == 4 ? argv[3] : "");
    return 0;
}

} // namespace shorthand::c3eco

#ifndef SHORTHAND_C3ECO_ASSESS_LIBRARY
int main(int argc, char **argv) {
    try {
        return shorthand::c3eco::run(argc, argv);
    } catch (const std::exception &error) {
        std::cerr << "c3eco assessment error: " << error.what() << '\n';
        return 2;
    }
}

#endif
