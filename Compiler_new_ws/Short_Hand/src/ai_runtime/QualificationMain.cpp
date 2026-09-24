#include "Qualification.h"
#include "ApplicationQualification.h"
#include "energy/PowercapEnergyCollector.h"
#include <fstream>
#include <cmath>
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
        if (argc==4 && std::string(argv[1])=="application-profile") {
            write(argv[3],shorthand::ai::profileApplication(shorthand::ai::readApplicationConfiguration(argv[2]))); return 0;
        }
        if (argc==3 && std::string(argv[1])=="application-stream") {
            shorthand::ai::serveApplicationStream(shorthand::ai::readApplicationConfiguration(argv[2]),std::cin,std::cout); return 0;
        }
        if (argc==4 && (std::string(argv[1])=="application" || std::string(argv[1])=="application-serve" || std::string(argv[1])=="application-describe")) {
            const auto c=shorthand::ai::readApplicationConfiguration(argv[2]);
            if (std::string(argv[1])=="application-describe") { write(argv[3],shorthand::ai::describeApplication(c)); return 0; }
            const auto r=shorthand::ai::evaluateApplication(c,std::string(argv[1])=="application-serve"); write(argv[3],r);
            return shorthand::c3eco::jsonBoolean(r,"success")?0:2;
        }
        if ((argc==7 && std::string(argv[1])=="application-meter-window") ||
            (argc==8 && std::string(argv[1])=="meter-window")) {
            const bool replay=std::string(argv[1])=="meter-window";
            auto parse=[](const char *s) { std::size_t n=0; double v=std::stod(s,&n); shorthand::c3eco::require(n==std::string(s).size() && std::isfinite(v),"invalid_window_number"); return v; };
            const double units=parse(argv[replay?6:5]); shorthand::c3eco::require(units>=1 && units<=50000000 && std::floor(units)==units,"invalid_window_units");
            if (replay) write(argv[7],shorthand::ai::measurePhysicalWindow(argv[2],shorthand::ai::readQualificationInstrument(argv[3]),parse(argv[4]),parse(argv[5]),static_cast<std::uint64_t>(units)));
            else write(argv[6],shorthand::ai::applicationMeterWindow(shorthand::ai::readApplicationConfiguration(argv[2]),parse(argv[3]),parse(argv[4]),static_cast<std::uint64_t>(units)));
            return 0;
        }
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
        std::cerr<<"usage: shorthand_ai_qualify probe | qualify CONFIG REPORT | execute CONFIG REPORT TRUSTED_REPORT_SHA256 OUTPUT | export-workbook REPORT TRUSTED_REPORT_SHA256 ACCOUNTING OUTPUT.tsv | application[-serve|-describe|-profile] CONFIG REPORT | application-meter-window CONFIG START_UNIX END_UNIX UNITS REPORT | meter-window TRACE INSTRUMENT_JSON START_UNIX END_UNIX UNITS REPORT\n";
        return 2;
    } catch (const std::exception &e) { std::cerr<<"qualification error: "<<e.what()<<'\n'; return 2; }
}
