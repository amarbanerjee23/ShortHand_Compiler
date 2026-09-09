#include "C3EcoEvidenceIO.h"
#include "../module/Sha256.h"
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#if OPENSSL_VERSION_NUMBER < 0x30000000L || OPENSSL_VERSION_NUMBER >= 0x40000000L
#error "The auditor tool requires a supported OpenSSL 3.x version."
#endif
#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <utility>

namespace shorthand::c3eco::audit {
namespace fs = std::filesystem;
constexpr const char *kSchema = "shorthand.c3eco.auditor_bundle.v1";
constexpr const char *kRulesId = "shorthand.c3eco.rules.v0.6+v0.7-20260718.v1";
constexpr std::size_t kMaxFiles = 1024;
constexpr std::size_t kMaxTotalBytes = 256U * 1024U * 1024U;
const std::set<std::string> kInputs = {"profile.json", "measurement.json", "metadata.tsv",
                                       "gates.tsv",    "criteria.tsv",     "materiality.tsv",
                                       "claims.tsv",   "regressions.tsv"};
const std::set<std::string> kChanges = {"architecture", "model",    "runtime",  "database",
                                        "cloud_region", "provider", "hardware", "traffic"};

Json str(const std::string &v) {
    Json j;
    j.kind = Json::Kind::String;
    j.string = v;
    return j;
}
Json boolean(bool v) {
    Json j;
    j.kind = Json::Kind::Boolean;
    j.boolean = v;
    return j;
}
Json number(double v) {
    Json j;
    j.kind = Json::Kind::Number;
    j.number = v;
    return j;
}
Json object(std::initializer_list<std::pair<const std::string, Json>> fields) {
    Json j;
    j.kind = Json::Kind::Object;
    j.object = fields;
    return j;
}
Json array() {
    Json j;
    j.kind = Json::Kind::Array;
    return j;
}
std::string canonical(const Json &j) {
    switch (j.kind) {
    case Json::Kind::Null:
        return "null";
    case Json::Kind::Boolean:
        return j.boolean ? "true" : "false";
    case Json::Kind::String:
        return "\"" + jsonEscape(j.string) + "\"";
    case Json::Kind::Number: {
        require(std::isfinite(j.number), "non-finite JSON number");
        std::ostringstream out;
        out.imbue(std::locale::classic());
        out << std::setprecision(std::numeric_limits<double>::max_digits10) << j.number;
        return out.str();
    }
    case Json::Kind::Array: {
        std::string out = "[";
        for (const auto &v : j.array) {
            if (out.size() > 1)
                out += ',';
            out += canonical(v);
        }
        return out + ']';
    }
    case Json::Kind::Object: {
        std::string out = "{";
        for (const auto &v : j.object) {
            if (out.size() > 1)
                out += ',';
            out += canonical(str(v.first)) + ':' + canonical(v.second);
        }
        return out + '}';
    }
    }
    throw std::runtime_error("invalid JSON kind");
}
std::string hex(const unsigned char *p, std::size_t n) {
    const char *digits = "0123456789abcdef";
    std::string out;
    out.reserve(n * 2);
    for (std::size_t i = 0; i < n; ++i) {
        out += digits[p[i] >> 4];
        out += digits[p[i] & 15];
    }
    return out;
}
std::string unhex(const std::string &value, std::size_t bytes) {
    require(value.size() == bytes * 2, "invalid hexadecimal length");
    std::string result;
    for (std::size_t i = 0; i < value.size(); i += 2) {
        const auto digit = [](char c) -> unsigned {
            require((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'),
                    "invalid hexadecimal character");
            return c <= '9' ? static_cast<unsigned>(c - '0') : static_cast<unsigned>(c - 'a' + 10);
        };
        result += static_cast<char>(digit(value[i]) * 16 + digit(value[i + 1]));
    }
    return result;
}
void identifier(const std::string &value) {
    require(!value.empty() && value.size() <= 128, "invalid identifier length");
    for (unsigned char c : value)
        require((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
                    c == '_' || c == '-' || c == '.',
                "invalid identifier");
    require(value != "." && value != "..", "invalid identifier");
}
void safePath(const std::string &name) {
    require(!name.empty() && name.size() <= 512 && name.front() != '/' && name.back() != '/',
            "unsafe artifact path");
    std::istringstream input(name);
    std::string part;
    while (std::getline(input, part, '/')) {
        identifier(part);
        require(part.back() != '.' && part.find(':') == std::string::npos, "unsafe artifact path");
        std::string upper = part.substr(0, part.find('.'));
        std::transform(upper.begin(), upper.end(), upper.begin(), [](unsigned char c) {
            return c >= 'a' && c <= 'z' ? static_cast<char>(c - 'a' + 'A') : static_cast<char>(c);
        });
        require(upper != "CON" && upper != "PRN" && upper != "AUX" && upper != "NUL" &&
                    !(upper.size() == 4 &&
                      (upper.substr(0, 3) == "COM" || upper.substr(0, 3) == "LPT") &&
                      upper[3] >= '0' && upper[3] <= '9'),
                "reserved artifact path");
    }
}
bool below(const fs::path &path, const fs::path &parent) {
    auto p = path.begin();
    for (auto q = parent.begin(); q != parent.end(); ++q, ++p)
        if (p == path.end() || *p != *q)
            return false;
    return true;
}
void regular(const fs::path &p) {
    require(fs::is_regular_file(fs::symlink_status(p)),
            "artifact must be a regular non-symlink file: " + p.filename().string());
    require(fs::hard_link_count(p) == 1, "hard-linked artifact is forbidden");
}
void write(const fs::path &p, const std::string &bytes) {
    require(!fs::exists(fs::symlink_status(p)), "output already exists: " + p.filename().string());
    fs::create_directories(p.parent_path());
    std::ofstream out(p, std::ios::binary);
    require(static_cast<bool>(out), "cannot create artifact");
    out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    out.flush();
    require(static_cast<bool>(out), "cannot write artifact");
    out.close();
    require(static_cast<bool>(out), "cannot close artifact");
}
void writeJson(const fs::path &p, const Json &j) { write(p, canonical(j) + '\n'); }
Json readJson(const fs::path &p) {
    regular(p);
    return parseJson(readFile(p.string()));
}

// Private staging directories keep incomplete bundles and replay outputs invisible.
class Stage {
  public:
    fs::path path;
    explicit Stage(const fs::path &parent) {
        require(fs::is_directory(parent), "staging parent must exist");
        for (int attempt = 0; attempt < 8; ++attempt) {
            std::array<unsigned char, 16> nonce{};
            require(RAND_bytes(nonce.data(), static_cast<int>(nonce.size())) == 1,
                    "secure randomness unavailable");
            path = parent / (".shorthand-audit-" + hex(nonce.data(), nonce.size()));
            if (fs::create_directory(path)) {
                try {
                    fs::permissions(path, fs::perms::owner_all, fs::perm_options::replace);
                } catch (...) {
                    std::error_code ec;
                    fs::remove(path, ec);
                    throw;
                }
                return;
            }
        }
        throw std::runtime_error("cannot allocate private staging directory");
    }
    Stage(const Stage &) = delete;
    Stage &operator=(const Stage &) = delete;
    ~Stage() {
        std::error_code ec;
        fs::remove_all(path, ec);
    }
    void commit(const fs::path &destination) {
        require(!fs::exists(fs::symlink_status(destination)), "destination already exists");
        fs::rename(path, destination);
    }
};
using Files = std::map<std::string, std::string>;
Files files(const fs::path &root) {
    require(fs::is_directory(fs::symlink_status(root)),
            "bundle root must be a non-symlink directory");
    Files out;
    std::set<std::string> folded;
    std::size_t total = 0;
    for (const auto &entry : fs::recursive_directory_iterator(root)) {
        const auto status = entry.symlink_status();
        require(!fs::is_symlink(status), "symlink artifact is forbidden");
        const std::string name = entry.path().lexically_relative(root).generic_string();
        safePath(name);
        std::string lower = name;
        std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) {
            return c >= 'A' && c <= 'Z' ? static_cast<char>(c - 'A' + 'a') : static_cast<char>(c);
        });
        require(folded.insert(lower).second, "case-colliding artifact paths");
        if (fs::is_directory(status))
            continue;
        regular(entry.path());
        require(out.size() < kMaxFiles, "bundle exceeds 1024 files");
        const auto bytes = readFile(entry.path().string());
        require(bytes.size() <= kMaxTotalBytes - total, "bundle exceeds 256 MiB");
        total += bytes.size();
        out.emplace(name, bytes);
    }
    return out;
}
using Key = std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)>;
using Context = std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)>;
int noPassword(char *, int, int, void *) { return 0; }
Key loadKey(const fs::path &path, bool secret) {
    regular(path);
    const std::string bytes = readFile(path.string());
    require(bytes.size() < 16384, "key file too large");
    std::unique_ptr<BIO, decltype(&BIO_free)> bio(
        BIO_new_mem_buf(bytes.data(), static_cast<int>(bytes.size())), BIO_free);
    require(static_cast<bool>(bio), "cannot allocate key reader");
    Key key(secret ? PEM_read_bio_PrivateKey(bio.get(), nullptr, noPassword, nullptr)
                   : PEM_read_bio_PUBKEY(bio.get(), nullptr, noPassword, nullptr),
            EVP_PKEY_free);
    require(key && EVP_PKEY_base_id(key.get()) == EVP_PKEY_ED25519,
            "an explicit Ed25519 key is required");
    return key;
}
std::string keyId(EVP_PKEY *key) {
    std::array<unsigned char, 32> bytes{};
    std::size_t length = bytes.size();
    require(EVP_PKEY_get_raw_public_key(key, bytes.data(), &length) == 1 && length == bytes.size(),
            "cannot identify signing key");
    return crypto::sha256(std::string(reinterpret_cast<char *>(bytes.data()), bytes.size()));
}
std::string message(const std::string &manifest) { return std::string(kSchema) + "\n" + manifest; }
std::string sign(EVP_PKEY *key, const std::string &manifest) {
    Context context(EVP_MD_CTX_new(), EVP_MD_CTX_free);
    require(context && EVP_DigestSignInit(context.get(), nullptr, nullptr, nullptr, key) == 1,
            "cannot initialize Ed25519 signer");
    const auto payload = message(manifest);
    std::array<unsigned char, 64> signature{};
    std::size_t size = signature.size();
    require(EVP_DigestSign(context.get(), signature.data(), &size,
                           reinterpret_cast<const unsigned char *>(payload.data()),
                           payload.size()) == 1 &&
                size == signature.size(),
            "cannot sign manifest");
    return hex(signature.data(), signature.size());
}
void verifySignature(EVP_PKEY *key, const std::string &manifest, const std::string &signature) {
    const auto bytes = unhex(signature, 64);
    const auto payload = message(manifest);
    Context context(EVP_MD_CTX_new(), EVP_MD_CTX_free);
    require(context && EVP_DigestVerifyInit(context.get(), nullptr, nullptr, nullptr, key) == 1,
            "cannot initialize Ed25519 verifier");
    require(EVP_DigestVerify(context.get(), reinterpret_cast<const unsigned char *>(bytes.data()),
                             bytes.size(), reinterpret_cast<const unsigned char *>(payload.data()),
                             payload.size()) == 1,
            "manifest signature verification failed");
}
std::string addMonths(const std::string &date, int count) {
    require(validIsoDate(date), "invalid ISO date");
    int year = std::stoi(date.substr(0, 4));
    int month = std::stoi(date.substr(5, 2));
    month += count;
    year += (month - 1) / 12;
    month = (month - 1) % 12 + 1;
    require(year <= 9999, "date arithmetic exceeds supported range");
    const int day = std::min(std::stoi(date.substr(8, 2)), daysInMonth(year, month));
    std::ostringstream out;
    out << std::setfill('0') << std::setw(4) << year << '-' << std::setw(2) << month << '-'
        << std::setw(2) << day;
    return out.str();
}
std::string dateField(const Json &j, const std::string &key) {
    const auto value = jsonString(j, key);
    require(validIsoDate(value), "invalid date: " + key);
    return value;
}
std::string referencePath(const std::string &ref) {
    const auto path = ref.substr(0, ref.find('#'));
    safePath(path);
    require(path.rfind("evidence/", 0) == 0, "evidence reference must resolve under evidence/");
    return path;
}
void evidenceReference(const std::string &ref, const Files &input) {
    const auto path = referencePath(ref);
    require(input.count(path) == 1 && !input.at(path).empty(),
            "unresolved evidence reference: " + ref);
}
void references(const Files &input) {
    for (const auto &name : kInputs)
        require(input.count(name) == 1, "missing candidate artifact: " + name);
    for (const auto &entry : input) {
        require(kInputs.count(entry.first) || entry.first.rfind("evidence/", 0) == 0,
                "candidate artifact must be a contract input or evidence file");
    }
    for (const auto &name : kInputs) {
        if (name.size() < 4 || name.substr(name.size() - 4) != ".tsv")
            continue;
        std::istringstream in(input.at(name));
        std::string line;
        require(static_cast<bool>(std::getline(in, line)), "empty TSV");
        const auto header = splitTsv(line);
        while (std::getline(in, line)) {
            if (line.empty() || line == "\r")
                continue;
            const auto row = splitTsv(line);
            require(row.size() == header.size(), "invalid TSV width");
            for (std::size_t i = 0; i < header.size(); ++i)
                if (header[i].size() >= 4 && header[i].substr(header[i].size() - 4) == "_ref" &&
                    !row[i].empty() && row[i] != "-" && row[i] != "none")
                    evidenceReference(row[i], input);
        }
    }
    const auto measurement = parseJson(input.at("measurement.json"));
    const auto &records = jsonMember(measurement, "records");
    require(records.kind == Json::Kind::Array, "measurement records must be an array");
    for (const auto &record : records.array)
        evidenceReference(jsonString(record, "evidence_ref"), input);
}
Json rules() {
    // Canonical v0.6 gate ids stay immutable; v0.7 is an inclusion/numbering overlay.
    static const char *aliases[] = {"G1",  "G2", "G3",  "G4",  "G5/G6", "G7",
                                    "G8",  "G9", "G10", "G11", "G12",   "eligibility-prerequisite",
                                    "G13", "G14"};
    Json mapping = array();
    for (int i = 0; i < 14; ++i)
        mapping.array.push_back(object(
            {{"canonical", str("G" + std::to_string(i + 1))}, {"overlay", str(aliases[i])}}));
    return object({{"schema", str(kRulesId)},
                   {"normative_candidate", str("draft-v0.6")},
                   {"inclusion_overlay", str("draft-v0.7-2026-07-18")},
                   {"authority_review_draft", boolean(true)},
                   {"gate_mapping", mapping},
                   {"overlay_G6", str("cost_if_claimed; canonical G5 workbook and G10 claims")},
                   {"assessment_contract", str("shorthand.c3eco.assessment.v1")},
                   {"criteria_count", number(76)},
                   {"conflict_policy", str("retain canonical v0.6 ids; apply stricter applicable "
                                           "requirement; independent authority review")},
                   {"estimated_intake",
                    str("separate readiness only; never inserted into measured workbook v1")}});
}
void profileDates(const Json &profile, const Json &policy) {
    const auto &declarations = jsonMember(profile, "c3eco_declarations");
    require(declarations.kind == Json::Kind::Array, "typed profile declarations unavailable");
    bool found = false;
    for (const auto &block : declarations.array) {
        if (jsonString(block, "kind") != "certification_profile")
            continue;
        const auto &fields = jsonMember(block, "typed_fields");
        std::map<std::string, std::string> dates;
        for (const auto &key : {"valid_from", "valid_until"}) {
            const auto &values = jsonMember(fields, key);
            require(values.kind == Json::Kind::Array && values.array.size() == 1,
                    "invalid typed validity date");
            require(jsonString(values.array.front(), "type") == "string",
                    "invalid typed validity date type");
            dates[key] = dateField(values.array.front(), "value");
        }
        require(dates.size() == 2, "profile validity fields unavailable");
        require(dateField(policy, "created_on") >= dates.at("valid_from") &&
                    dateField(policy, "valid_until") <= dates.at("valid_until"),
                "bundle validity exceeds profile validity");
        found = true;
    }
    require(found, "certification_profile missing");
}
void validatePolicy(const Json &p, const Files &input) {
    requireExactKeys(p,
                     {"schema", "bundle_id", "custodian", "created_on", "valid_until",
                      "retain_until", "legal_hold", "recertification_accepted", "acceptance_ref",
                      "storage_ref", "access_review_ref", "restore_test_ref",
                      "last_surveillance_on", "surveillance_ref", "changes", "nonconformities",
                      "public_scope_approved"},
                     "audit policy");
    require(jsonString(p, "schema") == "shorthand.c3eco.audit_policy.v1",
            "unsupported audit policy schema");
    identifier(jsonString(p, "bundle_id"));
    identifier(jsonString(p, "custodian"));
    const auto created = dateField(p, "created_on");
    const auto until = dateField(p, "valid_until");
    require(created <= until && until <= addMonths(created, 12),
            "bundle validity must be at most twelve months");
    require(dateField(p, "retain_until") >= addMonths(until, 24),
            "retention must cover 24 months after validity ends");
    const auto last = dateField(p, "last_surveillance_on");
    require(last >= created && last <= until, "surveillance date outside bundle validity");
    (void)jsonBoolean(p, "legal_hold");
    (void)jsonBoolean(p, "public_scope_approved");
    require(jsonBoolean(p, "recertification_accepted"), "recertification acceptance is required");
    for (const auto &key : {"acceptance_ref", "storage_ref", "access_review_ref",
                            "restore_test_ref", "surveillance_ref"})
        evidenceReference(jsonString(p, key), input);
    const auto &changes = jsonMember(p, "changes");
    const auto &findings = jsonMember(p, "nonconformities");
    require(changes.kind == Json::Kind::Array && changes.array.size() <= 128,
            "invalid changes array");
    require(findings.kind == Json::Kind::Array && findings.array.size() <= 128,
            "invalid nonconformities array");
    std::set<std::string> ids;
    for (const auto &change : changes.array) {
        requireExactKeys(change, {"id", "kind", "observed_on", "review_ref"}, "change");
        const auto id = jsonString(change, "id");
        identifier(id);
        require(ids.insert(id).second, "duplicate change id");
        require(kChanges.count(jsonString(change, "kind")) == 1, "unknown recertification trigger");
        require(dateField(change, "observed_on") >= created &&
                    dateField(change, "observed_on") <= until,
                "change date outside validity");
        const auto ref = jsonString(change, "review_ref");
        if (ref != "-")
            evidenceReference(ref, input);
    }
    ids.clear();
    for (const auto &finding : findings.array) {
        requireExactKeys(finding, {"id", "severity", "status", "due_on", "resolution_ref"},
                         "nonconformity");
        const auto id = jsonString(finding, "id");
        identifier(id);
        require(ids.insert(id).second, "duplicate nonconformity id");
        const auto severity = jsonString(finding, "severity");
        require(severity == "minor" || severity == "major" || severity == "critical",
                "invalid nonconformity severity");
        const auto status = jsonString(finding, "status");
        require(status == "open" || status == "closed", "invalid nonconformity status");
        require(dateField(finding, "due_on") >= created, "nonconformity deadline before creation");
        const auto ref = jsonString(finding, "resolution_ref");
        require(status != "closed" || ref != "-",
                "closed nonconformity requires resolution evidence");
        if (ref != "-")
            evidenceReference(ref, input);
    }
    const auto measurement = parseJson(input.at("measurement.json"));
    for (const auto &record : jsonMember(measurement, "records").array) {
        const auto measured = jsonString(record, "measured_at").substr(0, 10);
        require(validIsoDate(measured) && measured <= created,
                "measurement is newer than bundle creation");
    }
    profileDates(parseJson(input.at("profile.json")), p);
}
Json lifecycle(const Json &policy, const Json &assessment, const std::string &asOf) {
    require(validIsoDate(asOf), "invalid verification date");
    require(asOf >= dateField(policy, "created_on"), "verification date precedes bundle creation");
    const auto last = dateField(policy, "last_surveillance_on");
    require(last <= asOf, "surveillance evidence is from the future");
    const auto &surveillance = jsonMember(assessment, "surveillance");
    const double cadence = jsonNumber(surveillance, "cadence_months");
    require(cadence == 6 || cadence == 12, "invalid assessment surveillance cadence");
    const auto next =
        std::min(addMonths(last, static_cast<int>(cadence)), dateField(policy, "valid_until"));
    bool review = jsonBoolean(surveillance, "recertification_review_required");
    for (const auto &change : jsonMember(policy, "changes").array) {
        require(dateField(change, "observed_on") <= asOf, "change evidence is from the future");
        review = review || jsonString(change, "review_ref") == "-";
    }
    bool unresolved = false;
    bool anyOpenFinding = false;
    for (const auto &finding : jsonMember(policy, "nonconformities").array)
        if (jsonString(finding, "status") == "open") {
            anyOpenFinding = true;
            unresolved = unresolved || jsonString(finding, "severity") != "minor" ||
                         dateField(finding, "due_on") <= asOf;
        }
    const bool eligible =
        jsonBoolean(jsonMember(assessment, "eligibility"), "all_mandatory_gates_passed");
    const bool expired = asOf > dateField(policy, "valid_until");
    const bool due = asOf >= next;
    const bool ready = eligible && !review && !unresolved && !expired && !due;
    const bool disposal = asOf > dateField(policy, "retain_until") &&
                          !jsonBoolean(policy, "legal_hold") && !review && !anyOpenFinding;
    return object({{"as_of", str(asOf)},
                   {"status", str(expired                             ? "expired"
                                  : review || unresolved || !eligible ? "review_required"
                                  : due                               ? "surveillance_due"
                                                                      : "candidate_review_ready")},
                   {"current_review_ready", boolean(ready)},
                   {"next_surveillance_on", str(next)},
                   {"recertification_review_required", boolean(review)},
                   {"blocking_nonconformity", boolean(unresolved)},
                   {"legal_hold", boolean(jsonBoolean(policy, "legal_hold"))},
                   {"retain_until", str(dateField(policy, "retain_until"))},
                   {"disposal_policy_eligible", boolean(disposal)},
                   {"storage_retention_independently_verified", boolean(false)}});
}
Json inventory(const Files &contents) {
    Json items = array();
    for (const auto &entry : contents)
        items.array.push_back(object({{"path", str(entry.first)},
                                      {"bytes", number(static_cast<double>(entry.second.size()))},
                                      {"sha256", str(crypto::sha256(entry.second))}}));
    return items;
}
Json makeManifest(const Files &contents, const Json &policy, EVP_PKEY *key, const std::string &kind,
                  const std::string &source = "-") {
    return object({{"schema", str(kSchema)},
                   {"kind", str(kind)},
                   {"bundle_id", jsonMember(policy, "bundle_id")},
                   {"signer_key_id", str(keyId(key))},
                   {"signature_algorithm", str("Ed25519")},
                   {"rules_id", str(kRulesId)},
                   {"created_on", jsonMember(policy, "created_on")},
                   {"valid_until", jsonMember(policy, "valid_until")},
                   {"source_manifest_sha256", str(source)},
                   {"artifacts", inventory(contents)},
                   {"official_certification_granted", boolean(false)},
                   {"level_claim_permitted", boolean(false)},
                   {"comparative_energy_claim", boolean(false)},
                   {"production_claim", boolean(false)}});
}
void seal(Stage &stage, const Json &manifest, EVP_PKEY *key) {
    const auto bytes = canonical(manifest) + '\n';
    write(stage.path / "manifest.json", bytes);
    write(stage.path / "manifest.sig", sign(key, bytes));
}
struct Verified {
    Json manifest;
    Files contents;
    Json policy;
    Json assessment;
    Json state;
    std::string digest;
};
Verified verify(const fs::path &root, EVP_PKEY *key, const std::string &asOf) {
    Files all = files(root);
    require(all.count("manifest.json") && all.count("manifest.sig"), "signed manifest is required");
    const auto manifestBytes = all.at("manifest.json");
    verifySignature(key, manifestBytes, all.at("manifest.sig"));
    const auto m = parseJson(manifestBytes);
    require(canonical(m) + '\n' == manifestBytes, "manifest is not canonical");
    requireExactKeys(m,
                     {"schema", "kind", "bundle_id", "signer_key_id", "signature_algorithm",
                      "rules_id", "created_on", "valid_until", "source_manifest_sha256",
                      "artifacts", "official_certification_granted", "level_claim_permitted",
                      "comparative_energy_claim", "production_claim"},
                     "signed manifest");
    require(jsonString(m, "schema") == kSchema && jsonString(m, "rules_id") == kRulesId,
            "unsupported bundle or rules version");
    require(jsonString(m, "signature_algorithm") == "Ed25519" &&
                jsonString(m, "signer_key_id") == keyId(key),
            "untrusted signer");
    for (const auto &flag : {"official_certification_granted", "level_claim_permitted",
                             "comparative_energy_claim", "production_claim"})
        require(!jsonBoolean(m, flag), "unsupported bundle claim");
    all.erase("manifest.json");
    all.erase("manifest.sig");
    require(canonical(jsonMember(m, "artifacts")) == canonical(inventory(all)),
            "artifact inventory or digest mismatch");
    Verified result{m, all, Json{}, Json{}, Json{}, crypto::sha256(manifestBytes)};
    require(jsonString(m, "kind") == "private", "private bundle required for independent replay");
    require(jsonString(m, "source_manifest_sha256") == "-", "invalid private lineage source");
    Files input;
    for (const auto &entry : all)
        if (entry.first.rfind("candidate/", 0) == 0)
            input.emplace(entry.first.substr(10), entry.second);
        else
            require(entry.first == "policy.json" || entry.first == "assessment.json" ||
                        entry.first == "rules.json",
                    "unexpected private artifact");
    references(input);
    require(all.count("policy.json") && all.count("assessment.json") && all.count("rules.json"),
            "private artifacts missing");
    require(all.at("rules.json") == canonical(rules()) + '\n',
            "rule mapping differs from compiled contract");
    result.policy = parseJson(all.at("policy.json"));
    validatePolicy(result.policy, input);
    for (const auto &field : {"bundle_id", "created_on", "valid_until"})
        require(jsonString(m, field) == jsonString(result.policy, field),
                "manifest policy mismatch");
    // Replay a private snapshot. Never trust the manifest's claimed assessment result.
    Stage replay(fs::temp_directory_path());
    for (const auto &entry : input)
        write(replay.path / "candidate" / entry.first, entry.second);
    assessDirectory((replay.path / "candidate").string(),
                    (replay.path / "assessment.json").string());
    require(readFile((replay.path / "assessment.json").string()) == all.at("assessment.json"),
            "assessment replay differs from signed output");
    result.assessment = parseJson(all.at("assessment.json"));
    result.state = lifecycle(result.policy, result.assessment, asOf);
    return result;
}
void pack(const fs::path &candidate, const fs::path &policyFile, const fs::path &secret,
          const fs::path &destination) {
    const auto input = files(candidate);
    references(input);
    require(!below(fs::weakly_canonical(destination), fs::canonical(candidate)),
            "output cannot be inside candidate input");
    require(!below(fs::canonical(secret), fs::canonical(candidate)),
            "signing key must be outside evidence inputs");
    const auto policy = readJson(policyFile);
    validatePolicy(policy, input);
    auto key = loadKey(secret, true);
    Stage stage(fs::absolute(destination).parent_path());
    for (const auto &entry : input)
        write(stage.path / "candidate" / entry.first, entry.second);
    writeJson(stage.path / "policy.json", policy);
    writeJson(stage.path / "rules.json", rules());
    assessDirectory((stage.path / "candidate").string(), (stage.path / "assessment.json").string());
    // Current state is checked at creation, but failures remain reviewable candidates.
    auto creationCheckDate = dateField(policy, "last_surveillance_on");
    for (const auto &change : jsonMember(policy, "changes").array)
        creationCheckDate = std::max(creationCheckDate, dateField(change, "observed_on"));
    (void)lifecycle(policy, readJson(stage.path / "assessment.json"), creationCheckDate);
    seal(stage, makeManifest(files(stage.path), policy, key.get(), "private"), key.get());
    stage.commit(fs::absolute(destination));
}
Json receipt(const Verified &v) {
    return object({{"schema", str("shorthand.c3eco.audit_verification.v1")},
                   {"bundle_id", jsonMember(v.manifest, "bundle_id")},
                   {"manifest_sha256", str(v.digest)},
                   {"signature_verified", boolean(true)},
                   {"inventory_verified", boolean(true)},
                   {"assessment_replayed", boolean(true)},
                   {"lifecycle", v.state},
                   {"official_certification_granted", boolean(false)},
                   {"level_claim_permitted", boolean(false)},
                   {"comparative_energy_claim", boolean(false)},
                   {"production_claim", boolean(false)}});
}
void exportPublic(const Verified &v, const fs::path &secret, const fs::path &destination) {
    require(jsonBoolean(v.policy, "public_scope_approved"),
            "public scope disclosure requires explicit approval");
    require(jsonBoolean(v.state, "current_review_ready"), "current candidate review is blocked");
    auto key = loadKey(secret, true);
    require(keyId(key.get()) == jsonString(v.manifest, "signer_key_id"),
            "public export requires the trusted bundle signer");
    const auto report =
        object({{"schema", str("shorthand.c3eco.public_audit_report.v1")},
                {"bundle_id", jsonMember(v.manifest, "bundle_id")},
                {"source_manifest_sha256", str(v.digest)},
                {"product", jsonMember(v.assessment, "product")},
                {"as_of", jsonMember(v.state, "as_of")},
                {"valid_until", jsonMember(v.policy, "valid_until")},
                {"next_surveillance_on", jsonMember(v.state, "next_surveillance_on")},
                {"report_status", str("candidate_evidence_only")},
                {"private_evidence_redacted", boolean(true)},
                {"assessment_replayed_before_export", boolean(true)},
                {"independent_certification_decision", boolean(false)},
                {"official_certification_granted", boolean(false)},
                {"level_claim_permitted", boolean(false)},
                {"comparative_energy_claim", boolean(false)},
                {"production_claim", boolean(false)}});
    Stage stage(fs::absolute(destination).parent_path());
    writeJson(stage.path / "report.json", report);
    // No arbitrary assessment text, paths, evidence, contacts or scores enter public Markdown.
    write(stage.path / "report.md",
          "# C3-ECO candidate evidence\n\nBundle: " + jsonString(v.manifest, "bundle_id") +
              "\n\nAssessment replayed before export. Private evidence is redacted. "
              "The approved product scope is in report.json.\n\nThis report grants no "
              "certification, level, production or comparative-energy claim.\n");
    seal(stage, makeManifest(files(stage.path), v.policy, key.get(), "public", v.digest),
         key.get());
    stage.commit(fs::absolute(destination));
}
void verifyPublic(const fs::path &root, EVP_PKEY *key, const std::string &asOf) {
    const auto all = files(root);
    require(all.size() == 4 && all.count("manifest.json") && all.count("manifest.sig") &&
                all.count("report.json") && all.count("report.md"),
            "invalid public artifact set");
    verifySignature(key, all.at("manifest.json"), all.at("manifest.sig"));
    const auto m = parseJson(all.at("manifest.json"));
    const auto r = parseJson(all.at("report.json"));
    require(canonical(m) + '\n' == all.at("manifest.json"), "noncanonical public manifest");
    requireExactKeys(m,
                     {"schema", "kind", "bundle_id", "signer_key_id", "signature_algorithm",
                      "rules_id", "created_on", "valid_until", "source_manifest_sha256",
                      "artifacts", "official_certification_granted", "level_claim_permitted",
                      "comparative_energy_claim", "production_claim"},
                     "public manifest");

    require(jsonString(m, "schema") == kSchema && jsonString(m, "kind") == "public" &&
                jsonString(m, "rules_id") == kRulesId &&
                jsonString(m, "signature_algorithm") == "Ed25519" &&
                jsonString(m, "signer_key_id") == keyId(key),
            "invalid public manifest contract");
    Files payload{{"report.json", all.at("report.json")}, {"report.md", all.at("report.md")}};
    require(canonical(jsonMember(m, "artifacts")) == canonical(inventory(payload)),
            "public artifact digest mismatch");
    requireExactKeys(r,
                     {"schema", "bundle_id", "source_manifest_sha256", "product", "as_of",
                      "valid_until", "next_surveillance_on", "report_status",
                      "private_evidence_redacted", "assessment_replayed_before_export",
                      "independent_certification_decision", "official_certification_granted",
                      "level_claim_permitted", "comparative_energy_claim", "production_claim"},
                     "public report");
    const auto &product = jsonMember(r, "product");
    requireExactKeys(
        product, {"name", "version", "software_class", "functional_unit", "boundary", "workload"},
        "public product scope");
    for (const auto &field : product.object)
        require(field.second.kind == Json::Kind::String && !field.second.string.empty(),
                "invalid public product field");
    identifier(jsonString(r, "bundle_id"));
    require(dateField(m, "created_on") <= dateField(r, "as_of") &&
                dateField(r, "valid_until") <= addMonths(dateField(m, "created_on"), 12),
            "public dates exceed signed validity");

    require(jsonString(r, "schema") == "shorthand.c3eco.public_audit_report.v1" &&
                jsonString(r, "report_status") == "candidate_evidence_only",
            "invalid public report contract");
    for (const auto &field : {"bundle_id", "valid_until", "source_manifest_sha256"})
        require(jsonString(r, field) == jsonString(m, field), "public report lineage mismatch");
    (void)unhex(jsonString(r, "source_manifest_sha256"), 32);
    for (const auto &field : {"official_certification_granted", "level_claim_permitted",
                              "comparative_energy_claim", "production_claim"})
        require(!jsonBoolean(r, field) && !jsonBoolean(m, field), "unsupported public claim");
    require(!jsonBoolean(r, "independent_certification_decision") &&
                jsonBoolean(r, "private_evidence_redacted") &&
                jsonBoolean(r, "assessment_replayed_before_export"),
            "invalid public evidence boundary");
    require(validIsoDate(asOf) && asOf >= dateField(r, "as_of") &&
                asOf <= dateField(r, "valid_until") && asOf < dateField(r, "next_surveillance_on"),
            "public report is expired or surveillance is due");
    const std::string expectedMarkdown =
        "# C3-ECO candidate evidence\n\nBundle: " + jsonString(m, "bundle_id") +
        "\n\nAssessment replayed before export. Private evidence is redacted. "
        "The approved product scope is in report.json.\n\nThis report grants no certification, "
        "level, production or comparative-energy claim.\n";
    require(all.at("report.md") == expectedMarkdown, "public Markdown violates redaction contract");
    std::cout << canonical(object({{"schema", str("shorthand.c3eco.public_verification.v1")},
                                   {"signature_verified", boolean(true)},
                                   {"private_assessment_replayed", boolean(false)},
                                   {"official_certification_granted", boolean(false)},
                                   {"level_claim_permitted", boolean(false)},
                                   {"comparative_energy_claim", boolean(false)},
                                   {"production_claim", boolean(false)}}))
              << '\n';
}
void readiness(const fs::path &source) {
    const auto input = readJson(source);
    requireExactKeys(input, {"schema", "product_id", "functional_unit", "boundary", "estimates"},
                     "readiness intake");
    require(jsonString(input, "schema") == "shorthand.c3eco.readiness_intake.v1",
            "unsupported readiness schema");
    for (const auto &field : {"product_id", "functional_unit", "boundary"})
        identifier(jsonString(input, field));
    const auto &estimates = jsonMember(input, "estimates");
    require(estimates.kind == Json::Kind::Array && estimates.array.size() <= 128,
            "invalid readiness estimates");
    for (const auto &estimate : estimates.array) {
        requireExactKeys(estimate,
                         {"component", "energy_kwh", "carbon_factor_gco2e_per_kwh",
                          "uncertainty_percent", "method", "source_ref"},
                         "readiness estimate");
        identifier(jsonString(estimate, "component"));
        require(jsonNumber(estimate, "energy_kwh") > 0 &&
                    jsonNumber(estimate, "carbon_factor_gco2e_per_kwh") > 0 &&
                    jsonNumber(estimate, "uncertainty_percent") > 0 &&
                    jsonNumber(estimate, "uncertainty_percent") <= 100,
                "invalid estimated quantity");
        require(!jsonString(estimate, "method").empty() &&
                    !jsonString(estimate, "source_ref").empty(),
                "transparent estimation provenance required");
    }
    std::cout << canonical(
                     object({{"schema", str("shorthand.c3eco.readiness_result.v1")},
                             {"product_id", jsonMember(input, "product_id")},
                             {"readiness_state",
                              str(estimates.array.empty() ? "Registered" : "Candidate")},
                             {"measurement_quality", str(estimates.array.empty() ? "MQ0" : "MQ1")},
                             {"evidence_kind", str("estimated_readiness_only")},
                             {"measured_workbook_produced", boolean(false)},
                             {"official_certification_granted", boolean(false)},
                             {"level_claim_permitted", boolean(false)},
                             {"comparative_energy_claim", boolean(false)},
                             {"production_claim", boolean(false)}}))
              << '\n';
}
int run(int argc, char **argv) {
    require(argc >= 2, "usage: shorthand_c3eco_audit "
                       "pack|verify|replay|export-public|verify-public|retention-check|readiness "
                       "(see docs/c3eco_auditor_bundle.md)");
    const std::string command = argv[1];
    if (command == "pack") {
        require(argc == 6, "pack <candidate-dir> <policy.json> <private.pem> <new-bundle-dir>");
        pack(argv[2], argv[3], argv[4], argv[5]);
        return 0;
    }
    if (command == "readiness") {
        require(argc == 3, "readiness <intake.json>");
        readiness(argv[2]);
        return 0;
    }
    require(argc == (command == "export-public" ? 7 : 5),
            "verify/replay/retention-check/verify-public <bundle> <trusted-public.pem> <as-of>; "
            "export-public adds <private.pem> <new-public-dir>");
    auto key = loadKey(argv[3], false);
    if (command == "verify-public") {
        verifyPublic(argv[2], key.get(), argv[4]);
        return 0;
    }
    require(command == "verify" || command == "replay" || command == "retention-check" ||
                command == "export-public",
            "unknown audit command");
    const auto result = verify(argv[2], key.get(), argv[4]);
    if (command == "export-public") {
        exportPublic(result, argv[5], argv[6]);
        return 0;
    }
    std::cout << canonical(receipt(result)) << '\n';
    if (command == "replay")
        return 0;
    return jsonBoolean(result.state, command == "retention-check" ? "disposal_policy_eligible"
                                                                  : "current_review_ready")
               ? 0
               : 3;
}
} // namespace shorthand::c3eco::audit
int main(int argc, char **argv) {
    try {
        return shorthand::c3eco::audit::run(argc, argv);
    } catch (const std::exception &error) {
        std::cerr << "c3eco audit error: " << error.what() << '\n';
        return 2;
    }
}
