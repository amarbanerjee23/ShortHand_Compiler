// Real ONNX regression, invoked by runtime-profile with the pinned CPU SDK.
#include "Compiler_new_ws/Short_Hand/src/ai_runtime/backends/OnnxRuntimeBackend.h"
#include "Compiler_new_ws/Short_Hand/src/ai_runtime/AI_Telemetry.h"
#include <future>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace shorthand::ai;
void check(bool ok) { if (!ok) throw std::runtime_error("prepared profile regression"); }
int main(int argc,char **argv) {
    if (argc!=2) return 2;
    ModelSpec model; model.path=argv[1]; model.format=ModelFormat::Onnx; model.precision="float32";
    model.input.shape={16,64}; model.output.shape={16,10};
    OnnxRuntimeBackend backend; std::string error;
    auto session=backend.prepare(model,InferenceConfiguration{},error);
    if (!session) throw std::runtime_error(error);
    TensorBuffer input; input.spec=session->inputSpec(); input.f32_data.assign(16*64,.25f);
    const auto reference=session->run(input);
    check(reference.status==InferenceStatus::Success);

    // Serialization is externally visible evidence. The faster implementation
    // must remain byte-compatible with the historical JSON contract.
    TelemetryRecord telemetry;
    telemetry.component="comp\"x";
    telemetry.backend="back\\end";
    telemetry.model="line\nmodel";
    telemetry.status="success";
    telemetry.reason="why";
    telemetry.latency_ns=17;
    telemetry.input_elements=2;
    telemetry.output_elements=3;
    telemetry.measured_energy_available=true;
    telemetry.measured_energy_kwh=.125;
    check(telemetryToJson(telemetry)==
        R"({"component":"comp\"x","backend":"back\\end","model":"line\nmodel","status":"success","reason":"why","latency_ns":17,"input_elements":2,"output_elements":3,"measured_energy_available":true,"measured_energy_kwh":0.125})");
    check(telemetryToOtlpLikeSpanJson(telemetry)==
        R"({"name":"shorthand.ai.infer","kind":"SPAN_KIND_INTERNAL","attributes":{"ai.system":"shorthand","ai.backend":"back\\end","ai.model.name":"line\nmodel","ai.inference.status":"success","ai.inference.reason":"why","ai.input.elements":2,"ai.output.elements":3,"ai.latency.ns":17,"ai.energy.measured":true,"ai.energy.kwh":0.125}})");
    auto run=[&] {
        PreparedInferenceProfile p;
        const auto result=session->runProfiled(input,p);
        check(result.status==InferenceStatus::Success && p.success && p.total_ns>0);
        check(result.output_f32==reference.output_f32 && result.reason==reference.reason);
        check(!result.telemetry_json_fragment.empty() && result.input_elements==input.f32_data.size());
        check(p.total_ns==p.setup_ns+p.input_validation_ns+p.tensor_setup_ns+p.session_run_ns+p.output_copy_ns+p.telemetry_ns);
    };
    run();
    std::vector<std::future<void>> workers;
    for (int n=0;n<4;++n) workers.push_back(std::async(std::launch::async,[&]{for(int i=0;i<16;++i)run();}));
    for (auto &worker:workers) worker.get();
    for (bool invalid_shape:{false,true}) {
        auto bad=input;
        if (invalid_shape) bad.spec.shape={1,64};
        else bad.f32_data.back()=std::numeric_limits<float>::quiet_NaN();
        PreparedInferenceProfile p; p.success=true; p.total_ns=123;
        const auto ordinary=session->run(bad), profiled=session->runProfiled(bad,p);
        check(profiled.status!=InferenceStatus::Success && profiled.reason==ordinary.reason);
        check(!p.success && p.total_ns==0 && p.session_run_ns==0 && profiled.output_f32.empty());
    }
    std::cout<<"PASS real ONNX profiling: output/telemetry parity, errors, cleared samples, concurrent calls\n";
}
