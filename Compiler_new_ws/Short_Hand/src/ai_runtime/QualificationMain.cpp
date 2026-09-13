#include "Qualification.h"
#include "energy/PowercapEnergyCollector.h"
#include <fstream>
#include <iostream>
namespace {
void write(const std::string &path,const shorthand::c3eco::Json &j) {
    const auto text=shorthand::ai::qualificationJson(j)+"\n";
    shorthand::c3eco::require(text.size()<=32U*1024U*1024U,"report_size_limit");
    std::ofstream out(path); shorthand::c3eco::require(bool(out),"report_output_open_failed");
    out<<text; out.close(); shorthand::c3eco::require(bool(out),"report_write_failed");
}
}
int main(int argc,char **argv) {
    try {
        if (argc==2 && std::string(argv[1])=="probe") {
            shorthand::energy::PowercapEnergyCollector meter; auto start=meter.begin();
            std::cout<<(start.available?"rapl_available=true":"rapl_available=false")<<" reason="<<start.reason<<'\n';
            std::cout<<"gpu_energy_qualification=unavailable cpu_only=true\n"; return 0;
        }
        if (argc==4 && std::string(argv[1])=="qualify") {
            auto c=shorthand::ai::readQualificationConfiguration(argv[2]); auto r=shorthand::ai::qualifyWorkload(c); write(argv[3],r);
            if (shorthand::c3eco::jsonMember(r,"selected_candidate").kind==shorthand::c3eco::Json::Kind::Null) {
                std::cerr<<"qualification failed: "<<shorthand::c3eco::jsonString(r,"selection_reason")<<'\n'; return 2;
            }
            std::cout<<"PASS qualification: "<<shorthand::c3eco::jsonString(r,"selection_reason")<<'\n'; return 0;
        }
        if (argc==6 && std::string(argv[1])=="execute") {
            auto c=shorthand::ai::readQualificationConfiguration(argv[2]); write(argv[5],shorthand::ai::executeQualifiedProfile(c,argv[3],argv[4])); return 0;
        }
        if (argc==6 && std::string(argv[1])=="export-workbook") {
            shorthand::ai::exportQualificationWorkbook(argv[2],argv[3],argv[4],argv[5]); return 0;
        }
        std::cerr<<"usage: shorthand_ai_qualify probe | qualify CONFIG REPORT | execute CONFIG REPORT TRUSTED_REPORT_SHA256 OUTPUT | export-workbook REPORT TRUSTED_REPORT_SHA256 ACCOUNTING OUTPUT.tsv\n";
        return 2;
    } catch (const std::exception &e) { std::cerr<<"qualification error: "<<e.what()<<'\n'; return 2; }
}
