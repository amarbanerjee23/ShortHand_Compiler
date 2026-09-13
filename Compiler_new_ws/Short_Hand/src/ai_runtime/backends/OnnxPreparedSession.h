#pragma once
// Private implementation, included only by OnnxRuntimeBackend.cpp.
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <stdexcept>
#include <thread>
namespace shorthand::ai {
#if SHORTHAND_HAS_ONNXRUNTIME
namespace {
class OnnxPreparedSession final : public PreparedInference {
public:
    OnnxPreparedSession(const ModelSpec &m,const InferenceConfiguration &c):maximum_(c.maximum_tensor_elements) {
        if (m.format!=ModelFormat::Onnx || m.precision!="float32") throw std::runtime_error("prepared_execution_requires_onnx_float32");
        if (!c.threads || c.threads>std::max(1U,std::thread::hardware_concurrency()) || c.threads>256 ||
            !maximum_ || maximum_>16U*1024U*1024U) throw std::runtime_error("invalid_prepared_configuration");
        if (m.path.empty()) throw std::runtime_error("missing_model_path");
        Ort::SessionOptions o; o.SetIntraOpNumThreads(static_cast<int>(c.threads)); o.SetInterOpNumThreads(1);
        o.SetExecutionMode(ExecutionMode::ORT_SEQUENTIAL); o.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_BASIC);
        o.AddConfigEntry("session.intra_op.allow_spinning","0"); o.AddConfigEntry("session.inter_op.allow_spinning","0");
        const std::filesystem::path path(m.path);
        session_=std::make_unique<Ort::Session>(ortEnv(),path.c_str(),o);
        if (session_->GetInputCount()!=1 || session_->GetOutputCount()!=1) throw std::runtime_error("prepared_execution_requires_single_input_and_output");
        Ort::AllocatorWithDefaultOptions allocator;
        in_name_=firstInputName(*session_,allocator); out_name_=firstOutputName(*session_,allocator);
        auto it=session_->GetInputTypeInfo(0), ot=session_->GetOutputTypeInfo(0);
        in_=spec(it.GetTensorTypeAndShapeInfo(),in_name_,m.input); out_=spec(ot.GetTensorTypeAndShapeInfo(),out_name_,m.output);
    }
    TensorSpec inputSpec() const override { return in_; }
    TensorSpec outputSpec() const override { return out_; }
    std::string runtimeVersion() const override { return OrtGetApiBase()->GetVersionString(); }
    InferenceResult run(const TensorBuffer &input) override {
        InferenceResult r; r.backend=BackendKind::OnnxRuntimeCPU; r.backend_name=r.provider_name="onnxruntime_cpu";
        r.selected_device_class="cpu"; r.selected_device_id="cpu:0"; TelemetryTimer timer("onnxruntime_cpu","prepared_inference");
        try {
            if (input.spec.element_type!=ElementType::Float32 || input.spec.shape!=in_.shape || input.f32_data.size()!=in_.element_count)
                throw std::runtime_error("prepared_input_shape_or_dtype_mismatch");
            for (float v:input.f32_data) if (!std::isfinite(v)) throw std::runtime_error("nonfinite_prepared_input");
            auto memory=Ort::MemoryInfo::CreateCpu(OrtArenaAllocator,OrtMemTypeDefault);
            auto tensor=Ort::Value::CreateTensor<float>(memory,const_cast<float *>(input.f32_data.data()),input.f32_data.size(),in_.shape.data(),in_.shape.size());
            const char *inputs[]={in_name_.c_str()}, *outputs[]={out_name_.c_str()};
            auto values=session_->Run(Ort::RunOptions{nullptr},inputs,&tensor,1,outputs,1);
            if (values.size()!=1 || !values.front().IsTensor()) throw std::runtime_error("prepared_output_not_tensor");
            auto info=values.front().GetTensorTypeAndShapeInfo();
            if (info.GetElementType()!=ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT || info.GetShape()!=out_.shape || info.GetElementCount()!=out_.element_count)
                throw std::runtime_error("prepared_output_shape_or_dtype_mismatch");
            const auto *data=values.front().GetTensorData<float>(); r.output_f32.assign(data,data+out_.element_count);
            r.status=InferenceStatus::Success; r.reason="executed";
            attachTelemetry(r,timer.finish("success",r.reason,input.f32_data.size(),r.output_f32.size()));
        } catch (const std::exception &e) {
            r.status=InferenceStatus::RuntimeError; r.reason=e.what(); r.output_f32.clear();
            attachTelemetry(r,timer.finish("runtime_error",r.reason,input.f32_data.size(),0));
        }
        return r;
    }
private:
    TensorSpec spec(const Ort::ConstTensorTypeAndShapeInfo &info,const std::string &name,const TensorSpec &declared) const {
        if (info.GetElementType()!=ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT) throw std::runtime_error("prepared_tensor_precision_not_float32");
        TensorSpec s; s.name=name; s.element_type=ElementType::Float32; s.shape=info.GetShape(); std::size_t count=1;
        if (s.shape.empty() || (!declared.shape.empty() && declared.shape.size()!=s.shape.size())) throw std::runtime_error("declared_model_rank_mismatch");
        for (std::size_t i=0;i<s.shape.size();++i) {
            if (s.shape[i]<=0) {
                if (declared.shape.size()!=s.shape.size() || declared.shape[i]<=0) throw std::runtime_error("dynamic_shape_requires_concrete_workload");
                s.shape[i]=declared.shape[i];
            }
            if (!declared.shape.empty() && declared.shape[i]!=s.shape[i]) throw std::runtime_error("declared_model_shape_mismatch");
            if (static_cast<std::uint64_t>(s.shape[i])>maximum_/count) throw std::runtime_error("prepared_tensor_size_limit");
            count*=static_cast<std::size_t>(s.shape[i]);
        }
        s.element_count=count; return s;
    }
    std::unique_ptr<Ort::Session> session_; std::size_t maximum_; TensorSpec in_,out_; std::string in_name_,out_name_;
};
}
#endif
std::unique_ptr<PreparedInference> OnnxRuntimeBackend::prepare(const ModelSpec &m,const InferenceConfiguration &c,std::string &error) {
#if SHORTHAND_HAS_ONNXRUNTIME
    try { auto p=std::make_unique<OnnxPreparedSession>(m,c); error.clear(); return p; }
    catch (const std::exception &e) { error=e.what(); }
#else
    (void)m; (void)c; error="onnxruntime_sdk_unavailable";
#endif
    return nullptr;
}
}
