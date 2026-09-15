#include "Compiler_new_ws/Short_Hand/src/ai_runtime/AI_Runtime.h"
#include <algorithm>
#include <atomic>
#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using namespace shorthand::ai;

namespace {
void require(bool value,const char *reason) { if (!value) throw std::runtime_error(reason); }

HardwareRoutingPolicy cpuPolicy() {
    HardwareRoutingPolicy policy;
    policy.preference={DeviceClass::CPU};
    policy.override_device=DeviceClass::CPU;
    return policy;
}

ModelSpec model(const std::string &path,const std::string &task,const std::vector<std::int64_t> &input,
                const std::vector<std::int64_t> &output) {
    ModelSpec m;
    m.name="pr98_"+task; m.path=path; m.format=ModelFormat::Onnx; m.task=task;
    // FP32 is the public tensor boundary. The quantized fixture performs INT8/UINT8
    // operations internally and dequantizes before returning to the host boundary.
    m.precision="float32"; m.allow_fallback=false; m.backend_preference={BackendKind::OnnxRuntimeCPU};
    m.input.element_type=ElementType::Float32; m.input.shape=input; m.input.element_count=productOfShape(input);
    m.output.element_type=ElementType::Float32; m.output.shape=output; m.output.element_count=productOfShape(output);
    return m;
}

std::vector<float> execute(AIRuntime &runtime,const ModelSpec &m,const std::vector<float> &values,bool live) {
    InferenceConfiguration c; c.threads=1; c.maximum_tensor_elements=4096;
    std::string error; auto session=runtime.prepare(m,c,error);
    if (!live) {
        require(!session,"SDK-off family preparation reported success");
        require(!error.empty(),"SDK-off family preparation lacked explicit reason");
        return {};
    }
    require(bool(session),("family prepare failed: "+error).c_str());
    require(session->inputSpec().shape==m.input.shape,"family input shape changed");
    require(session->outputSpec().shape==m.output.shape,"family output shape changed");
    TensorBuffer input; input.spec=session->inputSpec(); input.f32_data=values;
    auto first=session->run(input), second=session->run(input);
    require(first.status==InferenceStatus::Success && second.status==InferenceStatus::Success,"family execution failed");
    require(first.output_f32==second.output_f32,"family execution is nondeterministic");
    require(first.output_f32.size()==m.output.element_count,"family output size mismatch");
    require(std::all_of(first.output_f32.begin(),first.output_f32.end(),[](float v){return std::isfinite(v);}),"nonfinite family output");
    return first.output_f32;
}
}

int main(int argc,char **argv) {
    try {
        require(argc==5,"usage: test_benchmark_runtime LIVE RETRIEVAL DETECTION QUANTIZED");
        const bool live=std::string(argv[1])=="1";
        AIRuntime runtime(std::make_shared<SystemHardwareProbe>(),cpuPolicy());
        bool advertised_int8=false;
        for (const auto &cap:runtime.capabilities()) if (cap.kind==BackendKind::OnnxRuntimeCPU)
            advertised_int8=std::find(cap.supported_precisions.begin(),cap.supported_precisions.end(),"int8")!=cap.supported_precisions.end();
        require(advertised_int8,"ONNX Runtime CPU capability lost INT8 support declaration");

        const std::vector<float> batch={-1.0f,-.5f,0.0f,.5f, 1.0f,.75f,-.25f,.25f};
        const auto retrieval=model(argv[2],"retrieval",{2,4},{2,3});
        const auto detection=model(argv[3],"detection",{2,4},{2,12});
        const auto quantized=model(argv[4],"quantized_inference",{2,4},{2,4});
        auto retrieval_output=execute(runtime,retrieval,batch,live);
        auto detection_output=execute(runtime,detection,batch,live);
        auto quantized_output=execute(runtime,quantized,batch,live);
        if (!live) {
            std::cout<<"PASS PR98 family runtime negative qualification live_onnx=0\n";
            return 0;
        }
        require(retrieval_output.size()==6 && detection_output.size()==24,"family shape contract failed");
        for (std::size_t n=0;n<batch.size();++n)
            require(std::abs(double(quantized_output[n])-batch[n])<=0.125001,"INT8 internal roundtrip exceeded quantization bound");

        // Concurrent serving-style reuse of one prepared retrieval session. Every
        // worker uses private input/output buffers; shared session execution must
        // remain stable and finite.
        InferenceConfiguration c; c.threads=1; c.maximum_tensor_elements=4096;
        std::string error; auto shared=runtime.prepare(retrieval,c,error); require(bool(shared),"shared retrieval prepare failed");
        std::atomic<unsigned> failures{0}; std::vector<std::thread> workers;
        for (unsigned worker=0;worker<4;++worker) workers.emplace_back([&] {
            TensorBuffer input; input.spec=shared->inputSpec(); input.f32_data=batch;
            for (unsigned n=0;n<16;++n) {
                auto result=shared->run(input);
                if (result.status!=InferenceStatus::Success || result.output_f32!=retrieval_output) ++failures;
            }
        });
        for (auto &worker:workers) worker.join();
        require(failures.load()==0,"concurrent prepared inference regression");
        std::cout<<"PASS PR98 CPU retrieval detection INT8-internal batched and concurrent prepared inference live_onnx=1\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr<<"PR98 benchmark runtime failure: "<<e.what()<<'\n';
        return 2;
    }
}
