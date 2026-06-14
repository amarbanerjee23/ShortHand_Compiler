#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace greenai {

static const char *TOOL_VERSION = "1.1.0-cpp";
static const char *SCHEMA = "green-ai-evidence/v1";
static const char *CERT_NOTE = "Evidence report only; this tool does not grant certification.";

struct Value {
    enum Type { NIL, BOOL, NUMBER, STRING, ARRAY, OBJECT } type;
    bool b;
    double n;
    std::string s;
    std::vector<Value> a;
    std::map<std::string, Value> o;

    Value() : type(NIL), b(false), n(0.0) {}
    static Value boolean(bool v) { Value x; x.type = BOOL; x.b = v; return x; }
    static Value number(double v) { Value x; x.type = NUMBER; x.n = v; return x; }
    static Value str(const std::string &v) { Value x; x.type = STRING; x.s = v; return x; }
    static Value array() { Value x; x.type = ARRAY; return x; }
    static Value object() { Value x; x.type = OBJECT; return x; }
};

struct Block {
    std::string kind;
    std::string name;
    Value body;
};

struct Token {
    std::string kind;
    std::string text;
    size_t off;
    int line;
    int col;
};

static std::string toString(double v) {
    std::ostringstream os;
    if (std::fabs(v - std::round(v)) < 1e-12) os << std::fixed << std::setprecision(0) << v;
    else os << std::setprecision(12) << v;
    return os.str();
}

static std::string escapeJson(const std::string &in) {
    std::ostringstream os;
    for (size_t i = 0; i < in.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(in[i]);
        switch (c) {
            case '"': os << "\\\""; break;
            case '\\': os << "\\\\"; break;
            case '\n': os << "\\n"; break;
            case '\r': os << "\\r"; break;
            case '\t': os << "\\t"; break;
            default:
                if (c < 32) os << "\\u00" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(c) << std::dec;
                else os << in[i];
        }
    }
    return os.str();
}

static std::string json(const Value &v) {
    std::ostringstream os;
    switch (v.type) {
        case Value::NIL: os << "null"; break;
        case Value::BOOL: os << (v.b ? "true" : "false"); break;
        case Value::NUMBER: os << toString(v.n); break;
        case Value::STRING: os << '"' << escapeJson(v.s) << '"'; break;
        case Value::ARRAY:
            os << "[";
            for (size_t i = 0; i < v.a.size(); ++i) { if (i) os << ","; os << json(v.a[i]); }
            os << "]";
            break;
        case Value::OBJECT: {
            os << "{";
            bool first = true;
            for (std::map<std::string, Value>::const_iterator it = v.o.begin(); it != v.o.end(); ++it) {
                if (!first) os << ",";
                first = false;
                os << '"' << escapeJson(it->first) << "":" << json(it->second);
            }
            os << "}";
            break;
        }
    }
    return os.str();
}

static std::string pretty(const Value &v, int indent = 0) {
    if (v.type != Value::OBJECT && v.type != Value::ARRAY) return json(v);
    std::ostringstream os;
    std::string pad(indent, ' '), pad2(indent + 2, ' ');
    if (v.type == Value::ARRAY) {
        os << "[";
        for (size_t i = 0; i < v.a.size(); ++i) os << (i ? ",\n" : "\n") << pad2 << pretty(v.a[i], indent + 2);
        if (!v.a.empty()) os << "\n" << pad;
        os << "]";
    } else {
        os << "{";
        bool first = true;
        for (std::map<std::string, Value>::const_iterator it = v.o.begin(); it != v.o.end(); ++it) {
            os << (first ? "\n" : ",\n") << pad2 << '"' << escapeJson(it->first) << "": " << pretty(it->second, indent + 2);
            first = false;
        }
        if (!v.o.empty()) os << "\n" << pad;
        os << "}";
    }
    return os.str();
}

struct Parser {
    explicit Parser(const std::string &t) : text(t), p(0) { lex(); }
    std::string text;
    std::vector<Token> toks;
    size_t p;

    std::runtime_error error(const std::string &msg, const Token &tok) const {
        std::ostringstream os;
        os << msg << " at line " << tok.line << ", column " << tok.col << ", byte " << tok.off;
        return std::runtime_error(os.str());
    }

    void lex() {
        int line = 1, col = 1;
        for (size_t i = 0; i < text.size();) {
            char c = text[i];
            if (std::isspace(static_cast<unsigned char>(c))) {
                if (c == '\n') { ++line; col = 1; } else ++col;
                ++i;
                continue;
            }
            if (c == '#') {
                while (i < text.size() && text[i] != '\n') { ++i; ++col; }
                continue;
            }

            Token tok; tok.off = i; tok.line = line; tok.col = col;
            if (c == '"') {
                tok.kind = "STRING";
                ++i; ++col;
                std::ostringstream ss;
                while (i < text.size() && text[i] != '"') {
                    if (text[i] == '\\' && i + 1 < text.size()) {
                        char e = text[++i];
                        if (e == 'n') ss << '\n';
                        else if (e == 't') ss << '\t';
                        else ss << e;
                        ++i; col += 2;
                    } else {
                        ss << text[i++]; ++col;
                    }
                }
                if (i >= text.size()) throw error("unterminated string", tok);
                ++i; ++col;
                tok.text = ss.str();
                toks.push_back(tok);
                continue;
            }
            if (std::isdigit(static_cast<unsigned char>(c)) || c == '-') {
                tok.kind = "NUMBER";
                size_t start = i;
                if (c == '-') { ++i; ++col; }
                bool seen_digit = false;
                while (i < text.size() && (std::isdigit(static_cast<unsigned char>(text[i])) || text[i] == '.')) {
                    if (std::isdigit(static_cast<unsigned char>(text[i]))) seen_digit = true;
                    ++i; ++col;
                }
                if (!seen_digit) throw error("invalid numeric literal", tok);
                tok.text = text.substr(start, i - start);
                toks.push_back(tok);
                continue;
            }
            if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
                tok.kind = "IDENT";
                size_t start = i;
                while (i < text.size() && (std::isalnum(static_cast<unsigned char>(text[i])) || text[i] == '_' || text[i] == '-' || text[i] == '.')) { ++i; ++col; }
                tok.text = text.substr(start, i - start);
                toks.push_back(tok);
                continue;
            }
            if (std::string("{}:[],").find(c) != std::string::npos) {
                tok.kind = "PUNC"; tok.text = std::string(1, c);
                toks.push_back(tok); ++i; ++col;
                continue;
            }
            throw error("unexpected character", tok);
        }
        Token eof; eof.kind = "EOF"; eof.text = ""; eof.off = text.size(); eof.line = line; eof.col = col;
        toks.push_back(eof);
    }

    Token peek() const { return toks[p]; }
    Token advance() { return toks[p++]; }
    void expect(const std::string &x) { Token t = advance(); if (t.text != x) throw error("expected '" + x + "', found '" + t.text + "'", t); }

    std::vector<Block> parse() {
        std::vector<Block> blocks;
        while (peek().kind != "EOF") blocks.push_back(parseBlock());
        return blocks;
    }

    Block parseBlock() {
        std::vector<std::string> header;
        while (peek().text != "{") {
            Token t = advance();
            if (t.kind != "IDENT" && t.kind != "STRING") throw error("expected block header", t);
            header.push_back(t.text);
        }
        expect("{");
        Value body = parseBody();
        expect("}");
        Block b; b.body = body;
        if (header.empty()) throw std::runtime_error("empty block header");
        if (header.size() == 1) b.kind = header[0];
        else {
            for (size_t i = 0; i + 1 < header.size(); ++i) { if (i) b.kind += "_"; b.kind += header[i]; }
            b.name = header.back();
        }
        return b;
    }

    Value parseBody() {
        Value obj = Value::object();
        while (peek().text != "}") {
            Token k = advance();
            if (k.kind != "IDENT") throw error("expected key", k);
            if (peek().text == ":") { advance(); obj.o[k.text] = parseValue(); }
            else if (peek().text == "{") { advance(); obj.o[k.text] = parseBody(); expect("}"); }
            else throw error("expected ':' or '{' after key '" + k.text + "'", peek());
        }
        return obj;
    }

    Value parseValue() {
        Token t = advance();
        if (t.kind == "STRING") return Value::str(t.text);
        if (t.kind == "NUMBER") return Value::number(std::strtod(t.text.c_str(), 0));
        if (t.kind == "IDENT") {
            if (t.text == "true") return Value::boolean(true);
            if (t.text == "false") return Value::boolean(false);
            return Value::str(t.text);
        }
        if (t.text == "[") {
            Value arr = Value::array();
            if (peek().text != "]") {
                while (true) {
                    arr.a.push_back(parseValue());
                    if (peek().text != ",") break;
                    advance();
                }
            }
            expect("]");
            return arr;
        }
        if (t.text == "{") { Value obj = parseBody(); expect("}"); return obj; }
        throw error("unexpected value", t);
    }
};

static std::vector<Block> loadManifest(const std::string &path) {
    std::ifstream f(path.c_str());
    if (!f) throw std::runtime_error("cannot open " + path);
    std::stringstream ss; ss << f.rdbuf();
    return Parser(ss.str()).parse();
}

static std::map<std::string, std::vector<Block> > byKind(const std::vector<Block> &blocks) {
    std::map<std::string, std::vector<Block> > out;
    for (size_t i = 0; i < blocks.size(); ++i) out[blocks[i].kind].push_back(blocks[i]);
    return out;
}

static Value object() { return Value::object(); }
static const Value *get(const Value &v, const std::string &key) {
    if (v.type != Value::OBJECT) return 0;
    std::map<std::string, Value>::const_iterator it = v.o.find(key);
    return it == v.o.end() ? 0 : &it->second;
}
static Value firstBody(const std::map<std::string, std::vector<Block> > &by, const std::string &kind) {
    std::map<std::string, std::vector<Block> >::const_iterator it = by.find(kind);
    return it == by.end() || it->second.empty() ? object() : it->second[0].body;
}
static double num(const Value *v, double d = 0.0) {
    if (!v) return d;
    if (v->type == Value::NUMBER) return v->n;
    if (v->type == Value::STRING) return std::strtod(v->s.c_str(), 0);
    return d;
}
static std::string strv(const Value *v, const std::string &d = "") {
    if (!v) return d;
    if (v->type == Value::STRING) return v->s;
    if (v->type == Value::NUMBER) return toString(v->n);
    if (v->type == Value::BOOL) return v->b ? "true" : "false";
    return d;
}
static bool boolv(const Value *v, bool d = false) { return v && v->type == Value::BOOL ? v->b : d; }

static Value blockArray(const std::map<std::string, std::vector<Block> > &by, const std::string &kind) {
    Value arr = Value::array();
    std::map<std::string, std::vector<Block> >::const_iterator it = by.find(kind);
    if (it == by.end()) return arr;
    for (size_t i = 0; i < it->second.size(); ++i) {
        Value o = it->second[i].body;
        if (!it->second[i].name.empty()) o.o["name"] = Value::str(it->second[i].name);
        arr.a.push_back(o);
    }
    return arr;
}

static const std::set<std::string> MQ = {"MQ0", "MQ1", "MQ2", "MQ3", "MQ4"};
static const std::set<std::string> DQ = {"DQ0", "DQ1", "DQ2", "DQ3", "DQ4"};

static const std::map<std::string, std::pair<std::string, std::string> > budgetMap = {
    {"energy_per_inference_j_max", {"energy_per_inference_j", "J"}},
    {"energy_per_1000_inferences_wh_max", {"energy_per_1000_inferences_wh", "Wh"}},
    {"energy_per_1000_tokens_wh_max", {"energy_per_1000_tokens_wh", "Wh"}},
    {"training_energy_kwh_max", {"training_energy_kwh", "kWh"}},
    {"fine_tuning_energy_kwh_max", {"fine_tuning_energy_kwh", "kWh"}},
    {"evaluation_energy_kwh_max", {"evaluation_energy_kwh", "kWh"}},
    {"carbon_per_1000_inferences_gco2e_max", {"carbon_per_1000_inferences_gco2e", "gCO2e"}},
    {"carbon_per_1000_tokens_gco2e_max", {"carbon_per_1000_tokens_gco2e", "gCO2e"}},
    {"carbon_per_training_run_kgco2e_max", {"carbon_per_training_run_kgco2e", "kgCO2e"}},
    {"p95_latency_ms_max", {"p95_latency_ms", "ms"}},
    {"p99_latency_ms_max", {"p99_latency_ms", "ms"}},
    {"memory_peak_mb_max", {"memory_peak_mb", "MB"}},
    {"gpu_memory_peak_mb_max", {"gpu_memory_peak_mb", "MB"}},
    {"cpu_seconds_per_unit_max", {"cpu_seconds_per_unit", "s"}},
    {"network_mb_per_task_max", {"network_mb_per_task", "MB"}},
    {"storage_gb_month_max", {"storage_gb_month", "GB-month"}},
    {"token_budget_per_task_max", {"tokens_per_task", "tokens"}},
    {"retrieval_top_k_max", {"retrieval_top_k", "documents"}},
    {"cross_region_transfer_gb_max", {"cross_region_transfer_gb", "GB"}},
    {"update_payload_mb_max", {"update_payload_mb", "MB"}},
    {"idle_overhead_watts_max", {"idle_overhead_watts", "W"}},
    {"model_size_mb_max", {"model_size_mb", "MB"}}
};

static const std::vector<std::string> energyFields = {
    "training_energy_kwh", "fine_tuning_energy_kwh", "evaluation_energy_kwh", "inference_energy_kwh",
    "idle_energy_kwh", "network_energy_kwh", "storage_energy_kwh", "client_energy_kwh",
    "ci_cd_energy_kwh", "update_energy_kwh", "third_party_ai_api_energy_kwh"
};

static const std::vector<std::string> additiveCarbonFields = {
    "embodied_carbon_gco2e", "network_carbon_gco2e", "storage_carbon_gco2e", "third_party_ai_api_carbon_gco2e"
};

static const std::vector<std::string> regressionFields = {
    "energy_per_functional_unit_kwh", "carbon_per_functional_unit_gco2e", "training_energy_kwh",
    "inference_energy_kwh", "memory_peak_mb", "gpu_memory_peak_mb", "network_mb_per_task",
    "tokens_per_task", "input_tokens_per_task", "output_tokens_per_task", "p95_latency_ms",
    "p95_latency_energy_j", "model_size_mb", "storage_gb_month", "cross_region_transfer_gb"
};

static void addByMode(std::vector<std::string> &warnings, std::vector<std::string> &errors, const std::string &mode, const std::string &msg) {
    if (mode == "strict") errors.push_back(msg);
    else warnings.push_back(msg);
}

static double functionalUnitCount(const Value &green, const Value &measurements) {
    double c = num(get(measurements, "functional_unit_count"), 0.0);
    if (c > 0) return c;
    std::string fu = strv(get(green, "functional_unit"));
    size_t p = fu.find('_');
    if (p != std::string::npos) {
        double v = std::strtod(fu.substr(0, p).c_str(), 0);
        if (v > 0) return v;
    }
    return 1.0;
}

static double operationalCarbonGco2e(double energyKwh, double gridFactor) {
    return energyKwh * gridFactor * 1000.0;
}

static Value stringArray(const std::vector<std::string> &items) {
    Value arr = Value::array();
    for (size_t i = 0; i < items.size(); ++i) arr.a.push_back(Value::str(items[i]));
    return arr;
}

static void validate(const std::map<std::string, std::vector<Block> > &by, const Value &green, const Value &carbon,
                     const Value &budgets, const std::string &mode, std::vector<std::string> &warnings,
                     std::vector<std::string> &errors) {
    if (mode != "off" && mode != "advisory" && mode != "strict") errors.push_back("green.green_mode must be off, advisory, or strict");
    if (mode == "off") return;

    std::vector<std::pair<std::string, std::string> > required = {
        {"green", "functional_unit"}, {"green", "success_criteria"}, {"green", "boundary"},
        {"green", "measurement_quality"}, {"green", "data_quality"}, {"carbon", "accounting_method"},
        {"carbon", "region"}, {"carbon", "grid_factor_kgco2e_per_kwh"}, {"carbon", "offsets_allowed_in_core_score"}
    };
    for (size_t i = 0; i < required.size(); ++i) {
        const Value &obj = required[i].first == "green" ? green : carbon;
        if (!get(obj, required[i].second)) addByMode(warnings, errors, mode, "missing required field " + required[i].first + "." + required[i].second);
    }
    if (budgets.o.empty()) addByMode(warnings, errors, mode, "missing required budgets block");

    if (boolv(get(carbon, "offsets_allowed_in_core_score"), false)) {
        errors.push_back("offsets_allowed_in_core_score must be false; offsets are never subtracted from core footprint");
    }

    std::string accounting = strv(get(carbon, "accounting_method"));
    if (!accounting.empty() && accounting != "location_based" && accounting != "market_based" && accounting != "both") {
        errors.push_back("carbon.accounting_method must be location_based, market_based, or both");
    }

    std::string mq = strv(get(green, "measurement_quality"));
    std::string dq = strv(get(green, "data_quality"));
    if (!mq.empty() && MQ.count(mq) == 0) errors.push_back("invalid measurement_quality");
    if (!dq.empty() && DQ.count(dq) == 0) errors.push_back("invalid data_quality");
    if (mode == "strict" && (mq == "MQ0" || dq == "DQ0")) errors.push_back("MQ0/DQ0 are not certification-ready in strict mode");
    if (mode == "strict" && mq == "MQ4" && !get(green, "audit_evidence")) errors.push_back("MQ4 requires audit_evidence references");

    if ((by.count("model") || by.count("infer_endpoint") || by.count("train_model")) && !by.count("target")) {
        addByMode(warnings, errors, mode, "model/deployment declared without hardware/runtime deployment target");
    }

    std::map<std::string, std::vector<Block> >::const_iterator it;
    it = by.find("train_model");
    if (it != by.end()) for (size_t i = 0; i < it->second.size(); ++i) if (!get(it->second[i].body, "green_train")) addByMode(warnings, errors, mode, "train model " + it->second[i].name + " missing green_train");
    it = by.find("infer_endpoint");
    if (it != by.end()) for (size_t i = 0; i < it->second.size(); ++i) if (!get(it->second[i].body, "green_infer")) addByMode(warnings, errors, mode, "infer endpoint " + it->second[i].name + " missing green_infer");
    it = by.find("llm_task");
    if (it != by.end()) for (size_t i = 0; i < it->second.size(); ++i) {
        const Value *gl = get(it->second[i].body, "green_llm");
        if (!gl || !get(*gl, "token_budget_per_task")) addByMode(warnings, errors, mode, "llm task " + it->second[i].name + " missing green_llm.token_budget_per_task");
        if (!get(it->second[i].body, "max_input_tokens") || !get(it->second[i].body, "max_output_tokens")) addByMode(warnings, errors, mode, "llm task " + it->second[i].name + " missing max input/output token limits");
    }
    it = by.find("rag_pipeline");
    if (it != by.end()) for (size_t i = 0; i < it->second.size(); ++i) {
        const Value *gr = get(it->second[i].body, "green_rag");
        if (!gr || !get(*gr, "retrieve_top_k_max")) addByMode(warnings, errors, mode, "rag pipeline " + it->second[i].name + " missing retrieve_top_k_max");
        if (!gr || (!boolv(get(*gr, "embedding_cache_enabled"), false) && !get(*gr, "embedding_cache_disabled_reason"))) addByMode(warnings, errors, mode, "rag pipeline " + it->second[i].name + " missing embedding cache evidence or reason");
    }
    it = by.find("model");
    if (it != by.end()) for (size_t i = 0; i < it->second.size(); ++i) {
        const Value *gm = get(it->second[i].body, "green_model");
        double sz = gm ? num(get(*gm, "model_size_mb"), 0.0) : 0.0;
        if (sz >= 500.0) {
            if (!gm || !get(*gm, "model_necessity_justification")) addByMode(warnings, errors, mode, "large model " + it->second[i].name + " missing model necessity justification");
            if (!gm || !boolv(get(*gm, "smaller_model_evaluated"), false)) warnings.push_back("large model " + it->second[i].name + " has no smaller_model_evaluated evidence");
        }
    }

    for (std::map<std::string, Value>::const_iterator b = budgets.o.begin(); b != budgets.o.end(); ++b) {
        if (budgetMap.count(b->first) == 0) warnings.push_back("unknown budget key budgets." + b->first);
        if (b->second.type == Value::NUMBER && b->second.n <= 0) errors.push_back("budget " + b->first + " must be positive");
    }
}

static Value makeReport(const std::vector<Block> &blocks, const std::string &file, const std::string &strictOverride, std::vector<std::string> *outErrors = 0) {
    std::map<std::string, std::vector<Block> > by = byKind(blocks);
    Value green = firstBody(by, "green"), carbon = firstBody(by, "carbon"), budgets = firstBody(by, "budgets"), measurements = firstBody(by, "measurements");
    std::string mode = strictOverride.empty() ? strv(get(green, "green_mode"), "advisory") : strictOverride;
    std::vector<std::string> warnings, errors;
    validate(by, green, carbon, budgets, mode, warnings, errors);

    double count = functionalUnitCount(green, measurements);
    double totalKwh = 0.0;
    Value componentEnergy = Value::object();
    for (size_t i = 0; i < energyFields.size(); ++i) {
        double v = num(get(measurements, energyFields[i]), 0.0);
        componentEnergy.o[energyFields[i]] = Value::number(v);
        totalKwh += v;
    }

    bool hasGrid = get(carbon, "grid_factor_kgco2e_per_kwh") != 0;
    double grid = num(get(carbon, "grid_factor_kgco2e_per_kwh"), 0.0);
    double opCarbon = hasGrid ? operationalCarbonGco2e(totalKwh, grid) : 0.0;
    if (!hasGrid && mode != "off") warnings.push_back("missing grid factor; carbon values are not certification-ready");

    Value additive = Value::object();
    double additiveCarbon = 0.0;
    for (size_t i = 0; i < additiveCarbonFields.size(); ++i) {
        double v = num(get(measurements, additiveCarbonFields[i]), 0.0);
        additive.o[additiveCarbonFields[i]] = Value::number(v);
        additiveCarbon += v;
    }
    double totalCarbon = opCarbon + additiveCarbon;
    double perUnitKwh = count > 0 ? totalKwh / count : 0.0;
    double perUnitCarbon = count > 0 ? totalCarbon / count : 0.0;

    Value derived = Value::object();
    derived.o["total_operational_energy_kwh"] = Value::number(totalKwh);
    derived.o["operational_carbon_gco2e"] = hasGrid ? Value::number(opCarbon) : Value();
    derived.o["total_carbon_gco2e"] = hasGrid ? Value::number(totalCarbon) : Value();
    derived.o["energy_per_functional_unit_kwh"] = Value::number(perUnitKwh);
    derived.o["carbon_per_functional_unit_gco2e"] = hasGrid ? Value::number(perUnitCarbon) : Value();
    derived.o["energy_per_inference_j"] = Value::number(perUnitKwh * 3600000.0);
    derived.o["energy_per_1000_inferences_wh"] = Value::number(perUnitKwh * 1000.0 * 1000.0);
    derived.o["energy_per_1000_tokens_wh"] = Value::number(perUnitKwh * 1000.0 * 1000.0);
    derived.o["carbon_per_1000_inferences_gco2e"] = hasGrid ? Value::number(perUnitCarbon * 1000.0) : Value();
    derived.o["carbon_per_1000_tokens_gco2e"] = hasGrid ? Value::number(perUnitCarbon * 1000.0) : Value();
    derived.o["carbon_per_training_run_kgco2e"] = hasGrid ? Value::number(totalCarbon / 1000.0) : Value();

    for (size_t i = 0; i < energyFields.size(); ++i) if (get(measurements, energyFields[i])) derived.o[energyFields[i]] = *get(measurements, energyFields[i]);
    std::vector<std::string> passthrough = {"memory_peak_mb", "gpu_memory_peak_mb", "network_mb_per_task", "tokens_per_task", "input_tokens_per_task", "output_tokens_per_task", "p95_latency_ms", "p99_latency_ms", "p95_latency_energy_j", "model_size_mb", "storage_gb_month", "cross_region_transfer_gb", "retrieval_top_k", "idle_overhead_watts", "update_payload_mb", "cpu_seconds_per_unit"};
    for (size_t i = 0; i < passthrough.size(); ++i) if (get(measurements, passthrough[i])) derived.o[passthrough[i]] = *get(measurements, passthrough[i]);

    Value budgetResults = Value::array();
    for (std::map<std::string, Value>::const_iterator it = budgets.o.begin(); it != budgets.o.end(); ++it) {
        Value r = Value::object();
        r.o["budget"] = Value::str(it->first);
        r.o["limit"] = it->second;
        std::map<std::string, std::pair<std::string, std::string> >::const_iterator meta = budgetMap.find(it->first);
        if (meta == budgetMap.end()) {
            r.o["actual"] = Value(); r.o["unit"] = Value::str(""); r.o["status"] = Value::str("unknown");
        } else {
            r.o["unit"] = Value::str(meta->second.second);
            const Value *actual = get(derived, meta->second.first);
            if (!actual) actual = get(measurements, meta->second.first);
            if (actual && actual->type == Value::NUMBER && it->second.type == Value::NUMBER) {
                r.o["actual"] = *actual;
                r.o["status"] = Value::str(actual->n <= it->second.n ? "pass" : "fail");
            } else {
                r.o["actual"] = Value();
                r.o["status"] = Value::str("not_evaluated");
            }
        }
        budgetResults.a.push_back(r);
    }

    Value componentCarbon = Value::object();
    componentCarbon.o["operational_energy_kwh"] = componentEnergy;
    componentCarbon.o["operational_carbon_gco2e"] = hasGrid ? Value::number(opCarbon) : Value();
    componentCarbon.o["additive_carbon_gco2e"] = additive;
    componentCarbon.o["total_additive_carbon_gco2e"] = Value::number(additiveCarbon);
    componentCarbon.o["double_counting_note"] = Value::str("Additive carbon components must be supplied only when not already included in operational energy.");

    Value report = Value::object();
    std::time_t now = std::time(0);
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&now));
    report.o["report_schema_version"] = Value::str(SCHEMA);
    report.o["generated_at"] = Value::str(buf);
    report.o["tool_name"] = Value::str("green_ai_tool");
    report.o["tool_version"] = Value::str(TOOL_VERSION);
    report.o["product"] = Value::str(strv(get(green, "product"), "ShortHand Green AI application"));
    report.o["source_file"] = Value::str(file);
    report.o["dsl_file"] = Value::str(file);
    report.o["green_mode"] = Value::str(mode);
    report.o["certification_profile"] = Value::str(strv(get(green, "certification_profile")));
    report.o["functional_unit"] = Value::str(strv(get(green, "functional_unit")));
    report.o["functional_unit_count"] = Value::number(count);
    report.o["success_criteria"] = get(green, "success_criteria") ? *get(green, "success_criteria") : Value::object();
    report.o["boundary"] = get(green, "boundary") ? *get(green, "boundary") : Value::object();
    report.o["carbon"] = carbon;
    report.o["measurement_quality"] = Value::str(strv(get(green, "measurement_quality")));
    report.o["data_quality"] = Value::str(strv(get(green, "data_quality")));
    report.o["budgets"] = budgets;
    report.o["budget_results"] = budgetResults;
    report.o["models"] = blockArray(by, "model");
    report.o["training"] = blockArray(by, "train_model");
    report.o["inference"] = blockArray(by, "infer_endpoint");
    report.o["llm_tasks"] = blockArray(by, "llm_task");
    report.o["rag_pipelines"] = blockArray(by, "rag_pipeline");
    report.o["model_routes"] = blockArray(by, "model_route");
    report.o["targets"] = blockArray(by, "target");
    report.o["data_pipelines"] = blockArray(by, "data_pipeline");
    report.o["measurements"] = measurements;
    report.o["derived"] = derived;
    report.o["component_carbon"] = componentCarbon;
    report.o["location_based_carbon_gco2e"] = hasGrid ? Value::number(opCarbon) : Value();
    report.o["market_based_carbon_gco2e"] = get(carbon, "market_based_carbon_gco2e") ? *get(carbon, "market_based_carbon_gco2e") : Value();
    report.o["offsets_reported_separately"] = get(carbon, "offsets_gco2e") ? *get(carbon, "offsets_gco2e") : Value::number(0.0);
    report.o["warnings"] = stringArray(warnings);
    report.o["errors"] = stringArray(errors);
    report.o["assumptions"] = get(green, "assumptions") ? *get(green, "assumptions") : Value::array();
    Value unknown = Value::array();
    if (!hasGrid) unknown.a.push_back(Value::str("grid_factor_kgco2e_per_kwh"));
    report.o["unsupported_or_unmeasured_components"] = unknown;
    report.o["audit_evidence_references"] = get(green, "audit_evidence") ? *get(green, "audit_evidence") : Value::array();
    report.o["certification_note"] = Value::str(CERT_NOTE);

    if (outErrors) *outErrors = errors;
    return report;
}

static double findNumberInJson(const std::string &txt, const std::string &key, bool &ok) {
    std::string quoted = "\"" + key + "\"";
    size_t p = txt.find(quoted);
    if (p == std::string::npos) { ok = false; return 0.0; }
    p = txt.find(':', p);
    if (p == std::string::npos) { ok = false; return 0.0; }
    ++p;
    while (p < txt.size() && std::isspace(static_cast<unsigned char>(txt[p]))) ++p;
    char *end = 0;
    double v = std::strtod(txt.c_str() + p, &end);
    ok = end != txt.c_str() + p;
    return v;
}

static Value reportFromJsonFile(const std::string &path) {
    std::ifstream f(path.c_str());
    if (!f) throw std::runtime_error("cannot open " + path);
    std::stringstream ss; ss << f.rdbuf();
    std::string txt = ss.str();
    Value report = Value::object();
    Value derived = Value::object();
    Value measurements = Value::object();
    for (size_t i = 0; i < regressionFields.size(); ++i) {
        bool ok = false;
        double v = findNumberInJson(txt, regressionFields[i], ok);
        if (ok) { derived.o[regressionFields[i]] = Value::number(v); measurements.o[regressionFields[i]] = Value::number(v); }
    }
    report.o["derived"] = derived;
    report.o["measurements"] = measurements;
    report.o["errors"] = Value::array();
    report.o["warnings"] = Value::array();
    return report;
}

static Value checkRegression(const Value &current, const std::string &baselinePath, double thresholdPercent) {
    std::ifstream f(baselinePath.c_str());
    if (!f) throw std::runtime_error("cannot open baseline " + baselinePath);
    std::stringstream ss; ss << f.rdbuf();
    std::string baseline = ss.str();

    Value result = Value::object();
    result.o["passed"] = Value::boolean(true);
    result.o["regressions"] = Value::array();
    result.o["warnings"] = get(current, "warnings") ? *get(current, "warnings") : Value::array();
    result.o["errors"] = get(current, "errors") ? *get(current, "errors") : Value::array();

    const Value *curDerived = get(current, "derived");
    const Value *curMeasurements = get(current, "measurements");
    for (size_t i = 0; i < regressionFields.size(); ++i) {
        bool okBase = false;
        double base = findNumberInJson(baseline, regressionFields[i], okBase);
        const Value *cur = curDerived ? get(*curDerived, regressionFields[i]) : 0;
        if (!cur && curMeasurements) cur = get(*curMeasurements, regressionFields[i]);
        if (okBase && cur && cur->type == Value::NUMBER && base > 0.0) {
            double change = (cur->n - base) / base * 100.0;
            if (change > thresholdPercent) {
                Value r = Value::object();
                r.o["field"] = Value::str(regressionFields[i]);
                r.o["baseline"] = Value::number(base);
                r.o["current"] = Value::number(cur->n);
                r.o["change_percent"] = Value::number(change);
                r.o["threshold_percent"] = Value::number(thresholdPercent);
                r.o["status"] = Value::str("fail");
                r.o["remediation_hint"] = Value::str("Investigate Green AI regression in " + regressionFields[i] + "; consider smaller models, batching, caching, quantization, locality, or lower-carbon runtime targets.");
                result.o["regressions"].a.push_back(r);
                result.o["passed"] = Value::boolean(false);
            }
        }
    }
    if (result.o["errors"].type == Value::ARRAY && !result.o["errors"].a.empty()) result.o["passed"] = Value::boolean(false);
    return result;
}

static void usage() {
    std::cerr << "usage: green_ai_tool validate|report|check <file.greenai|report.json> [--output file] [--strict off|advisory|strict] [--baseline file] [--threshold-percent n]\n";
}

} // namespace greenai

int main(int argc, char **argv) {
    using namespace greenai;
    try {
        if (argc < 3) { usage(); return 2; }
        std::string cmd = argv[1], file = argv[2], output, strict, baseline;
        double threshold = 10.0;
        for (int i = 3; i < argc; ++i) {
            std::string a = argv[i];
            if (a == "--output" && i + 1 < argc) output = argv[++i];
            else if (a == "--strict" && i + 1 < argc) strict = argv[++i];
            else if (a == "--baseline" && i + 1 < argc) baseline = argv[++i];
            else if (a == "--threshold-percent" && i + 1 < argc) threshold = std::strtod(argv[++i], 0);
            else { std::cerr << "unknown or incomplete argument: " << a << "\n"; return 2; }
        }

        bool isJson = file.size() >= 5 && file.substr(file.size() - 5) == ".json";
        std::vector<std::string> errors;
        Value report = (cmd == "check" && isJson) ? reportFromJsonFile(file) : makeReport(loadManifest(file), file, strict, &errors);

        if (cmd == "validate") {
            const Value *warnings = get(report, "warnings");
            const Value *errs = get(report, "errors");
            if (warnings) for (size_t i = 0; i < warnings->a.size(); ++i) std::cerr << "warning: " << warnings->a[i].s << "\n";
            if (errs) for (size_t i = 0; i < errs->a.size(); ++i) std::cerr << "error: " << errs->a[i].s << "\n";
            return errors.empty() ? 0 : 1;
        }
        if (cmd == "report") {
            std::string out = pretty(report) + "\n";
            if (output.empty()) std::cout << out;
            else { std::ofstream f(output.c_str()); if (!f) throw std::runtime_error("cannot write " + output); f << out; }
            return errors.empty() ? 0 : 1;
        }
        if (cmd == "check") {
            if (baseline.empty()) throw std::runtime_error("--baseline is required");
            Value result = checkRegression(report, baseline, threshold);
            std::string out = pretty(result) + "\n";
            if (output.empty()) std::cout << out;
            else { std::ofstream f(output.c_str()); if (!f) throw std::runtime_error("cannot write " + output); f << out; }
            return result.o["passed"].b ? 0 : 1;
        }
        throw std::runtime_error("unknown command " + cmd);
    } catch (const std::exception &e) {
        std::cerr << "green_ai_tool: " << e.what() << "\n";
        return 1;
    }
}
