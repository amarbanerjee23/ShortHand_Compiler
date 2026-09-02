#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

namespace {
constexpr const char* kSchema = "shorthand.c3eco.measurement_workbook.v1";
constexpr double kEpsilon = 1e-9;

struct Row {
    std::string record_id;
    std::string component;
    std::string source_kind;
    std::string instrument_id;
    std::string calibration_id;
    std::string calibration_date;
    std::string measured_at;
    double raw_energy_j = 0.0;
    double allocation_fraction = 0.0;
    double pue = 0.0;
    double carbon_factor = 0.0;
    std::string factor_source;
    std::string factor_date;
    double tariff_per_kwh = 0.0;
    std::string tariff_currency;
    std::string tariff_source;
    double uncertainty_percent = 0.0;
    std::string measurement_quality;
    std::string data_quality;
    std::string evidence_ref;
};

struct Derived {
    Row row;
    double allocated_it_j = 0.0;
    double facility_j = 0.0;
    double facility_kwh = 0.0;
    double carbon_kg = 0.0;
    double cost = 0.0;
    double uncertainty_kwh = 0.0;
    double uncertainty_carbon_kg = 0.0;
};

std::vector<std::string> splitTsv(const std::string& line) {
    std::vector<std::string> out;
    std::string cur;
    for (char c : line) {
        if (c == '\t') { out.push_back(cur); cur.clear(); }
        else { cur.push_back(c); }
    }
    out.push_back(cur);
    return out;
}

std::string trim(const std::string& s) {
    const auto b = s.find_first_not_of(" \r\n");
    if (b == std::string::npos) return "";
    const auto e = s.find_last_not_of(" \r\n");
    return s.substr(b, e - b + 1);
}

double parseDouble(const std::string& text, const std::string& field, size_t line) {
    try {
        size_t used = 0;
        const double v = std::stod(text, &used);
        if (used != text.size() || !std::isfinite(v)) throw std::runtime_error("bad");
        return v;
    } catch (...) {
        throw std::runtime_error("line " + std::to_string(line) + ": invalid numeric " + field);
    }
}

bool leap(int y) { return (y % 400 == 0) || (y % 4 == 0 && y % 100 != 0); }

int daysInMonth(int y, int m) {
    static const int d[] = {0,31,28,31,30,31,30,31,31,30,31,30,31};
    if (m == 2) return d[m] + (leap(y) ? 1 : 0);
    return (m >= 1 && m <= 12) ? d[m] : 0;
}

bool validDate(const std::string& s) {
    if (s.size() != 10 || s[4] != '-' || s[7] != '-') return false;
    for (size_t i = 0; i < s.size(); ++i) {
        if (i == 4 || i == 7) continue;
        if (s[i] < '0' || s[i] > '9') return false;
    }
    const int y = std::stoi(s.substr(0,4));
    const int m = std::stoi(s.substr(5,2));
    const int d = std::stoi(s.substr(8,2));
    return y >= 2000 && m >= 1 && m <= 12 && d >= 1 && d <= daysInMonth(y,m);
}

std::string datePart(const std::string& measured) {
    if (measured.size() < 10) return "";
    return measured.substr(0,10);
}

std::string jsonEsc(const std::string& s) {
    std::string o;
    for (char c : s) {
        switch (c) {
            case '"': o += "\\\""; break;
            case '\\': o += "\\\\"; break;
            case '\n': o += "\\n"; break;
            case '\r': o += "\\r"; break;
            case '\t': o += "\\t"; break;
            default: o += c;
        }
    }
    return o;
}

std::string csvEsc(const std::string& s) {
    if (s.find_first_of(",\"\r\n") == std::string::npos) return s;
    std::string o = "\"";
    for (char c : s) { if (c == '"') o += "\"\""; else o += c; }
    o += "\"";
    return o;
}

void require(bool ok, const std::string& msg) {
    if (!ok) throw std::runtime_error(msg);
}

std::vector<Row> load(const std::string& path) {
    std::ifstream in(path);
    require(static_cast<bool>(in), "cannot open input: " + path);
    std::string line;
    require(static_cast<bool>(std::getline(in, line)), "measurement input is empty");
    auto header = splitTsv(trim(line));
    const std::vector<std::string> expected = {
        "record_id","component","source_kind","instrument_id","calibration_id","calibration_date",
        "measured_at","raw_energy_j","allocation_fraction","pue","carbon_factor_gco2e_per_kwh",
        "factor_source","factor_date","tariff_per_kwh","tariff_currency","tariff_source",
        "uncertainty_percent","measurement_quality","data_quality","evidence_ref"
    };
    require(header == expected, "invalid measurement TSV header");
    std::vector<Row> rows;
    std::set<std::string> ids;
    size_t lineNo = 1;
    while (std::getline(in, line)) {
        ++lineNo;
        line = trim(line);
        if (line.empty()) continue;
        auto f = splitTsv(line);
        require(f.size() == expected.size(), "line " + std::to_string(lineNo) + ": expected 20 columns");
        Row r;
        size_t i = 0;
        r.record_id=f[i++]; r.component=f[i++]; r.source_kind=f[i++]; r.instrument_id=f[i++];
        r.calibration_id=f[i++]; r.calibration_date=f[i++]; r.measured_at=f[i++];
        r.raw_energy_j=parseDouble(f[i++],"raw_energy_j",lineNo);
        r.allocation_fraction=parseDouble(f[i++],"allocation_fraction",lineNo);
        r.pue=parseDouble(f[i++],"pue",lineNo);
        r.carbon_factor=parseDouble(f[i++],"carbon_factor_gco2e_per_kwh",lineNo);
        r.factor_source=f[i++]; r.factor_date=f[i++];
        r.tariff_per_kwh=parseDouble(f[i++],"tariff_per_kwh",lineNo);
        r.tariff_currency=f[i++]; r.tariff_source=f[i++];
        r.uncertainty_percent=parseDouble(f[i++],"uncertainty_percent",lineNo);
        r.measurement_quality=f[i++]; r.data_quality=f[i++]; r.evidence_ref=f[i++];

        require(!r.record_id.empty() && ids.insert(r.record_id).second,
                "line " + std::to_string(lineNo) + ": duplicate/empty record_id");
        require(!r.component.empty(), "line " + std::to_string(lineNo) + ": component is required");
        static const std::set<std::string> allowedSources = {
            "physical_meter","rapl","accelerator_counter","cloud_meter"
        };
        require(allowedSources.count(r.source_kind) == 1,
                "line " + std::to_string(lineNo) + ": source_kind must be real instrumentation, not modelled/declared");
        require(!r.instrument_id.empty() && !r.calibration_id.empty() && !r.factor_source.empty() &&
                !r.tariff_source.empty() && !r.evidence_ref.empty(),
                "line " + std::to_string(lineNo) + ": provenance fields are required");
        require(validDate(r.calibration_date), "line " + std::to_string(lineNo) + ": invalid calibration_date");
        require(validDate(r.factor_date), "line " + std::to_string(lineNo) + ": invalid factor_date");
        const std::string mdate = datePart(r.measured_at);
        require(validDate(mdate), "line " + std::to_string(lineNo) + ": measured_at must begin with YYYY-MM-DD");
        require(r.calibration_date <= mdate, "line " + std::to_string(lineNo) + ": calibration is after measurement");
        require(r.factor_date <= mdate, "line " + std::to_string(lineNo) + ": carbon factor is after measurement");
        require(r.raw_energy_j > 0.0, "line " + std::to_string(lineNo) + ": raw_energy_j must be positive");
        require(r.allocation_fraction > 0.0 && r.allocation_fraction <= 1.0,
                "line " + std::to_string(lineNo) + ": allocation_fraction must be in (0,1]");
        require(r.pue >= 1.0 && r.pue <= 3.0,
                "line " + std::to_string(lineNo) + ": pue must be in [1,3]");
        require(r.carbon_factor > 0.0 && r.carbon_factor <= 2500.0,
                "line " + std::to_string(lineNo) + ": carbon factor outside bounded range");
        require(r.tariff_per_kwh >= 0.0 && r.tariff_per_kwh <= 10000.0,
                "line " + std::to_string(lineNo) + ": tariff outside bounded range");
        require(r.tariff_currency.size() == 3,
                "line " + std::to_string(lineNo) + ": tariff_currency must be a three-letter code");
        require(r.uncertainty_percent >= 0.0 && r.uncertainty_percent <= 100.0,
                "line " + std::to_string(lineNo) + ": uncertainty_percent must be in [0,100]");
        static const std::set<std::string> q = {"high","medium","low"};
        require(q.count(r.measurement_quality) == 1 && q.count(r.data_quality) == 1,
                "line " + std::to_string(lineNo) + ": MQ/DQ must be high, medium or low");
        rows.push_back(r);
    }
    require(!rows.empty(), "measurement input has no records");

    std::map<std::tuple<std::string,std::string,double,std::string>, double> allocation;
    for (const auto& r : rows) {
        auto key = std::make_tuple(r.instrument_id, r.measured_at, r.raw_energy_j, r.evidence_ref);
        allocation[key] += r.allocation_fraction;
        require(allocation[key] <= 1.0 + kEpsilon,
                "double counting detected: allocation sum exceeds 1.0 for shared instrument evidence");
    }
    std::sort(rows.begin(), rows.end(), [](const Row& a, const Row& b) { return a.record_id < b.record_id; });
    return rows;
}

std::vector<Derived> derive(const std::vector<Row>& rows) {
    std::vector<Derived> out;
    for (const auto& r : rows) {
        Derived d;
        d.row = r;
        d.allocated_it_j = r.raw_energy_j * r.allocation_fraction;
        d.facility_j = d.allocated_it_j * r.pue;
        d.facility_kwh = d.facility_j / 3600000.0;
        d.carbon_kg = (d.facility_kwh * r.carbon_factor) / 1000.0;
        d.cost = d.facility_kwh * r.tariff_per_kwh;
        d.uncertainty_kwh = d.facility_kwh * (r.uncertainty_percent / 100.0);
        d.uncertainty_carbon_kg = d.carbon_kg * (r.uncertainty_percent / 100.0);
        out.push_back(d);
    }
    return out;
}

void writeCsv(const std::string& path, const std::vector<Derived>& rows) {
    std::ofstream out(path);
    require(static_cast<bool>(out), "cannot open CSV output: " + path);
    out << "record_id,component,source_kind,instrument_id,calibration_id,measured_at,raw_energy_j,allocation_fraction,allocated_it_energy_j,pue,facility_energy_kwh,carbon_factor_gco2e_per_kwh,carbon_kgco2e,tariff_per_kwh,tariff_currency,cost,uncertainty_percent,uncertainty_kwh,uncertainty_carbon_kgco2e,measurement_quality,data_quality,evidence_ref\n";
    out << std::setprecision(12);
    for (const auto& d : rows) {
        const auto& r = d.row;
        out << csvEsc(r.record_id) << ',' << csvEsc(r.component) << ',' << csvEsc(r.source_kind) << ','
            << csvEsc(r.instrument_id) << ',' << csvEsc(r.calibration_id) << ',' << csvEsc(r.measured_at) << ','
            << r.raw_energy_j << ',' << r.allocation_fraction << ',' << d.allocated_it_j << ',' << r.pue << ','
            << d.facility_kwh << ',' << r.carbon_factor << ',' << d.carbon_kg << ',' << r.tariff_per_kwh << ','
            << csvEsc(r.tariff_currency) << ',' << d.cost << ',' << r.uncertainty_percent << ','
            << d.uncertainty_kwh << ',' << d.uncertainty_carbon_kg << ',' << csvEsc(r.measurement_quality) << ','
            << csvEsc(r.data_quality) << ',' << csvEsc(r.evidence_ref) << '\n';
    }
}

void writeJson(const std::string& path, const std::vector<Derived>& rows) {
    std::ofstream out(path);
    require(static_cast<bool>(out), "cannot open JSON output: " + path);
    double itJ = 0.0, facilityKwh = 0.0, carbonKg = 0.0, uncertaintyKwh = 0.0, uncertaintyCarbon = 0.0;
    std::map<std::string,double> cost;
    for (const auto& d : rows) {
        itJ += d.allocated_it_j;
        facilityKwh += d.facility_kwh;
        carbonKg += d.carbon_kg;
        uncertaintyKwh += d.uncertainty_kwh;
        uncertaintyCarbon += d.uncertainty_carbon_kg;
        cost[d.row.tariff_currency] += d.cost;
    }
    out << std::setprecision(12);
    out << "{\n  \"schema\":\"" << kSchema << "\",\n"
        << "  \"measurement_status\":\"measured_instrumented\",\n"
        << "  \"official_certification_granted\":false,\n"
        << "  \"base_footprint_not_reduced_by_offsets\":true,\n"
        << "  \"allocation_policy\":\"shared instrument readings must sum to <=1.0\",\n"
        << "  \"record_count\":" << rows.size() << ",\n"
        << "  \"totals\":{\"allocated_it_energy_j\":" << itJ << ",\"facility_energy_kwh\":" << facilityKwh
        << ",\"carbon_kgco2e\":" << carbonKg << ",\"uncertainty_kwh\":" << uncertaintyKwh
        << ",\"uncertainty_carbon_kgco2e\":" << uncertaintyCarbon << ",\"cost_by_currency\":{";
    bool first = true;
    for (const auto& kv : cost) {
        if (!first) out << ',';
        first = false;
        out << "\"" << jsonEsc(kv.first) << "\":" << kv.second;
    }
    out << "}},\n  \"records\":[";
    for (size_t i = 0; i < rows.size(); ++i) {
        if (i) out << ',';
        const auto& d = rows[i];
        const auto& r = d.row;
        out << "\n    {\"record_id\":\"" << jsonEsc(r.record_id) << "\",\"component\":\"" << jsonEsc(r.component)
            << "\",\"source_kind\":\"" << jsonEsc(r.source_kind) << "\",\"instrument_id\":\"" << jsonEsc(r.instrument_id)
            << "\",\"calibration_id\":\"" << jsonEsc(r.calibration_id) << "\",\"calibration_date\":\"" << jsonEsc(r.calibration_date)
            << "\",\"measured_at\":\"" << jsonEsc(r.measured_at) << "\",\"raw_energy_j\":" << r.raw_energy_j
            << ",\"allocation_fraction\":" << r.allocation_fraction << ",\"allocated_it_energy_j\":" << d.allocated_it_j
            << ",\"pue\":" << r.pue << ",\"facility_energy_kwh\":" << d.facility_kwh
            << ",\"carbon_factor_gco2e_per_kwh\":" << r.carbon_factor << ",\"factor_source\":\"" << jsonEsc(r.factor_source)
            << "\",\"factor_date\":\"" << jsonEsc(r.factor_date) << "\",\"carbon_kgco2e\":" << d.carbon_kg
            << ",\"tariff_per_kwh\":" << r.tariff_per_kwh << ",\"tariff_currency\":\"" << jsonEsc(r.tariff_currency)
            << "\",\"tariff_source\":\"" << jsonEsc(r.tariff_source) << "\",\"cost\":" << d.cost
            << ",\"uncertainty_percent\":" << r.uncertainty_percent << ",\"uncertainty_kwh\":" << d.uncertainty_kwh
            << ",\"uncertainty_carbon_kgco2e\":" << d.uncertainty_carbon_kg
            << ",\"measurement_quality\":\"" << jsonEsc(r.measurement_quality) << "\",\"data_quality\":\"" << jsonEsc(r.data_quality)
            << "\",\"evidence_ref\":\"" << jsonEsc(r.evidence_ref) << "\"}";
    }
    out << "\n  ],\n"
        << "  \"claim_safe_text\":\"Measured accounting evidence only. This workbook does not grant C3-ECO certification and does not establish comparative energy superiority.\",\n"
        << "  \"next_qualification\":\"PR90 scoring and claims remain separate; comparative ShortHand/Python energy qualification remains PR95.\"\n}\n";
}
}

int main(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "usage: shorthand_c3eco_measure <input.tsv> <output.csv> <output.json>\n";
        return 64;
    }
    try {
        auto rows = derive(load(argv[1]));
        writeCsv(argv[2], rows);
        writeJson(argv[3], rows);
        std::cout << "PASS: " << kSchema << " records=" << rows.size() << "\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "FAIL: " << e.what() << "\n";
        return 2;
    }
}
