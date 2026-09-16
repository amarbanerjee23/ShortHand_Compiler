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
constexpr const char* kSchema = "shorthand.c3eco.lifecycle_cloud_boundary.v1";
constexpr double kEpsilon = 1e-9;
const std::vector<std::string> kHeader = {
    "record_id","lifecycle_phase","component","shared_resource_id","allocation_basis",
    "allocation_fraction","operational_carbon_kgco2e","energy_workbook_ref","data_bytes",
    "storage_gb_hours","network_gb","cloud_region","carbon_factor_gco2e_per_kwh",
    "factor_source","factor_date","embodied_asset_id","embodied_carbon_kgco2e",
    "asset_lifetime_hours","attributed_usage_hours","evidence_ref"};

struct Row {
  std::string id, phase, component, resource, basis, workbook, region, factor_source, factor_date, asset, evidence;
  double allocation = 0, operational = 0, data_bytes = 0, storage = 0, network = 0, factor = 0, embodied = 0, lifetime = 0, usage = 0;
};
struct Derived { Row row; double allocated_operational = 0, allocated_embodied = 0, total = 0; };

void require(bool ok, const std::string& message) { if (!ok) throw std::runtime_error(message); }
std::string trim(const std::string& s) { const auto b=s.find_first_not_of(" \r\n"); if(b==std::string::npos)return ""; const auto e=s.find_last_not_of(" \r\n"); return s.substr(b,e-b+1); }
std::vector<std::string> split(const std::string& s) { std::vector<std::string> out; std::string cur; for(char c:s){if(c=='\t'){out.push_back(cur);cur.clear();}else cur+=c;} out.push_back(cur); return out; }
double number(const std::string& s, const std::string& field, size_t line) { try { size_t used=0; double v=std::stod(s,&used); if(used!=s.size()||!std::isfinite(v)) throw std::runtime_error("bad"); return v; } catch(...) { throw std::runtime_error("line "+std::to_string(line)+": invalid numeric "+field); } }
bool validDate(const std::string& s) { if(s.size()!=10||s[4]!='-'||s[7]!='-')return false; for(size_t i=0;i<s.size();++i)if(i!=4&&i!=7&&(s[i]<'0'||s[i]>'9'))return false; int y=std::stoi(s.substr(0,4)),m=std::stoi(s.substr(5,2)),d=std::stoi(s.substr(8,2)); static const int days[]={0,31,28,31,30,31,30,31,31,30,31,30,31}; bool leap=(y%400==0)||(y%4==0&&y%100!=0); return y>=2000&&m>=1&&m<=12&&d>=1&&d<=days[m]+(m==2&&leap); }
std::string esc(const std::string& s) { std::string o; for(char c:s){if(c=='"')o+="\\\"";else if(c=='\\')o+="\\\\";else if(c=='\n')o+="\\n";else if(c=='\r')o+="\\r";else if(c=='\t')o+="\\t";else o+=c;} return o; }

std::vector<Row> load(const std::string& path) {
  std::ifstream in(path); require(static_cast<bool>(in), "cannot open input: "+path); std::string line;
  require(static_cast<bool>(std::getline(in,line)), "boundary input is empty"); require(split(trim(line))==kHeader, "invalid lifecycle/cloud boundary TSV header");
  std::set<std::string> ids; std::vector<Row> rows; size_t line_no=1;
  const std::set<std::string> phases={"data_ingest","data_storage","data_egress","model_lifecycle","cloud_compute","hardware_lifecycle"};
  const std::set<std::string> bases={"dedicated","tenant_request","byte","gb_hour","network_gb","usage_hour"};
  while(std::getline(in,line)) { ++line_no; line=trim(line); if(line.empty())continue; auto f=split(line); require(f.size()==kHeader.size(),"line "+std::to_string(line_no)+": expected 20 columns"); Row r; size_t i=0;
    r.id=f[i++];r.phase=f[i++];r.component=f[i++];r.resource=f[i++];r.basis=f[i++];r.allocation=number(f[i++],"allocation_fraction",line_no);r.operational=number(f[i++],"operational_carbon_kgco2e",line_no);r.workbook=f[i++];r.data_bytes=number(f[i++],"data_bytes",line_no);r.storage=number(f[i++],"storage_gb_hours",line_no);r.network=number(f[i++],"network_gb",line_no);r.region=f[i++];r.factor=number(f[i++],"carbon_factor_gco2e_per_kwh",line_no);r.factor_source=f[i++];r.factor_date=f[i++];r.asset=f[i++];r.embodied=number(f[i++],"embodied_carbon_kgco2e",line_no);r.lifetime=number(f[i++],"asset_lifetime_hours",line_no);r.usage=number(f[i++],"attributed_usage_hours",line_no);r.evidence=f[i++];
    require(!r.id.empty()&&ids.insert(r.id).second,"line "+std::to_string(line_no)+": duplicate/empty record_id"); require(phases.count(r.phase)==1,"line "+std::to_string(line_no)+": unsupported lifecycle_phase"); require(!r.component.empty()&&!r.resource.empty()&&!r.evidence.empty(),"line "+std::to_string(line_no)+": component, shared_resource_id and evidence_ref are required"); require(bases.count(r.basis)==1,"line "+std::to_string(line_no)+": unsupported allocation_basis"); require(r.allocation>0&&r.allocation<=1,"line "+std::to_string(line_no)+": allocation_fraction must be in (0,1]"); require(r.operational>=0&&r.data_bytes>=0&&r.storage>=0&&r.network>=0,"line "+std::to_string(line_no)+": operational and data activity values must be non-negative"); require(!r.workbook.empty()&&!r.region.empty()&&!r.factor_source.empty()&&validDate(r.factor_date),"line "+std::to_string(line_no)+": measured workbook, region and dated factor provenance are required"); require(r.factor>0&&r.factor<=2500,"line "+std::to_string(line_no)+": carbon factor outside bounded range");
    if(r.phase=="hardware_lifecycle") { require(!r.asset.empty()&&r.embodied>0&&r.lifetime>0&&r.usage>0&&r.usage<=r.lifetime,"line "+std::to_string(line_no)+": hardware lifecycle requires bounded embodied asset evidence"); }
    else { require(r.asset.empty()&&r.embodied==0&&r.lifetime==0&&r.usage==0,"line "+std::to_string(line_no)+": non-hardware record must not allocate embodied hardware carbon"); }
    rows.push_back(r);
  }
  require(!rows.empty(),"boundary input has no records"); std::map<std::string,double> allocations; std::set<std::string> observed;
  for(const auto& r:rows){ allocations[r.resource]+=r.allocation; require(allocations[r.resource]<=1+kEpsilon,"double counting detected: allocation sum exceeds 1.0 for shared_resource_id"); observed.insert(r.phase); }
  for(const auto& phase:phases) require(observed.count(phase)==1,"boundary is incomplete: missing required lifecycle_phase "+phase);
  std::sort(rows.begin(),rows.end(),[](const Row&a,const Row&b){return a.id<b.id;}); return rows;
}

void writeJson(const std::string& path, const std::vector<Row>& rows) {
  std::ofstream out(path); require(static_cast<bool>(out),"cannot open JSON output: "+path); std::vector<Derived> derived; double op=0, embodied=0, data=0, storage=0, network=0;
  for(const auto&r:rows){ Derived d; d.row=r; d.allocated_operational=r.operational*r.allocation; d.allocated_embodied=r.phase=="hardware_lifecycle"?r.embodied*(r.usage/r.lifetime)*r.allocation:0; d.total=d.allocated_operational+d.allocated_embodied; op+=d.allocated_operational;embodied+=d.allocated_embodied;data+=r.data_bytes*r.allocation;storage+=r.storage*r.allocation;network+=r.network*r.allocation;derived.push_back(d); }
  out<<std::setprecision(12)<<"{\n  \"schema\":\""<<kSchema<<"\",\n  \"boundary_status\":\"candidate_evidence_complete_for_declared_scope\",\n  \"official_certification_granted\":false,\n  \"comparative_energy_claim\":false,\n  \"production_claim\":false,\n  \"allocation_policy\":\"shared_resource_id allocations must sum to <=1.0\",\n  \"totals\":{\"allocated_operational_carbon_kgco2e\":"<<op<<",\"allocated_embodied_carbon_kgco2e\":"<<embodied<<",\"total_carbon_kgco2e\":"<<op+embodied<<",\"allocated_data_bytes\":"<<data<<",\"allocated_storage_gb_hours\":"<<storage<<",\"allocated_network_gb\":"<<network<<"},\n  \"records\":[";
  for(size_t i=0;i<derived.size();++i){if(i)out<<',';const auto&d=derived[i];const auto&r=d.row;out<<"\n    {\"record_id\":\""<<esc(r.id)<<"\",\"lifecycle_phase\":\""<<esc(r.phase)<<"\",\"shared_resource_id\":\""<<esc(r.resource)<<"\",\"allocation_fraction\":"<<r.allocation<<",\"energy_workbook_ref\":\""<<esc(r.workbook)<<"\",\"cloud_region\":\""<<esc(r.region)<<"\",\"factor_source\":\""<<esc(r.factor_source)<<"\",\"factor_date\":\""<<esc(r.factor_date)<<"\",\"allocated_operational_carbon_kgco2e\":"<<d.allocated_operational<<",\"allocated_embodied_carbon_kgco2e\":"<<d.allocated_embodied<<",\"total_carbon_kgco2e\":"<<d.total<<",\"evidence_ref\":\""<<esc(r.evidence)<<"\"}";}
  out<<"\n  ],\n  \"claim_safe_text\":\"Declared-boundary accounting evidence only; this report does not grant certification or establish comparative energy superiority.\"\n}\n";
}
}
int main(int argc,char**argv){if(argc!=3){std::cerr<<"usage: shorthand_c3eco_lifecycle_boundary <input.tsv> <output.json>\n";return 64;}try{auto rows=load(argv[1]);writeJson(argv[2],rows);std::cout<<"PASS: "<<kSchema<<" records="<<rows.size()<<"\n";return 0;}catch(const std::exception&e){std::cerr<<"FAIL: "<<e.what()<<"\n";return 2;}}
