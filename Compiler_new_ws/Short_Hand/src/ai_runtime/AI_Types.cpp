#include "AI_Types.h"
#include <algorithm>
#include <cerrno>
#include <cctype>
#include <cstdlib>
#include <sstream>

namespace shorthand::ai {
namespace {
std::string norm(std::string s){ std::transform(s.begin(),s.end(),s.begin(),[](unsigned char c){ return c=='-'?'_':(char)std::tolower(c); }); return s; }
std::string trim(const std::string &s){ const auto first=s.find_first_not_of(" \t\r\n"); if(first==std::string::npos) return ""; const auto last=s.find_last_not_of(" \t\r\n"); return s.substr(first,last-first+1); }
bool parseInt64NoThrow(const std::string &token, int64_t &out){ std::string t=trim(token); if(t.empty()) return false; char *end=nullptr; errno=0; long long value=std::strtoll(t.c_str(), &end, 10); if(errno!=0 || end==t.c_str() || *end!='\0') return false; out=static_cast<int64_t>(value); return true; }
bool parseFloatNoThrow(const std::string &token, float &out){ std::string t=trim(token); if(t.empty()) return false; char *end=nullptr; errno=0; float value=std::strtof(t.c_str(), &end); if(errno!=0 || end==t.c_str() || *end!='\0') return false; out=value; return true; }
}
ElementType parseElementType(const std::string &v){ auto s=norm(v); if(s=="float"||s=="float32"||s=="f32") return ElementType::Float32; if(s=="float16"||s=="fp16"||s=="f16") return ElementType::Float16; if(s=="bfloat16"||s=="bf16") return ElementType::BFloat16; if(s=="int8") return ElementType::Int8; if(s=="int4") return ElementType::Int4; if(s=="int32"||s=="int") return ElementType::Int32; return ElementType::Unknown; }
ModelFormat parseModelFormat(const std::string &v){ auto s=norm(v); if(s=="onnx") return ModelFormat::Onnx; if(s=="engine"||s=="tensorrt"||s=="tensorrt_engine") return ModelFormat::TensorRTEngine; if(s=="torchscript"||s=="torch") return ModelFormat::TorchScript; if(s=="openvino_ir"||s=="openvino") return ModelFormat::OpenVINOIR; if(s=="gguf") return ModelFormat::GGUF; return ModelFormat::Unknown; }
BackendKind parseBackendKind(const std::string &v){ auto s=norm(v); if(s=="tensorrt") return BackendKind::TensorRT; if(s=="onnxruntime_tensorrt") return BackendKind::OnnxRuntimeTensorRT; if(s=="onnxruntime_cuda") return BackendKind::OnnxRuntimeCUDA; if(s=="onnxruntime"||s=="onnxruntime_cpu") return BackendKind::OnnxRuntimeCPU; if(s=="openvino") return BackendKind::OpenVINO; if(s=="libtorch") return BackendKind::LibTorch; if(s=="llamacpp"||s=="llama_cpp"||s=="llama.cpp") return BackendKind::LlamaCpp; return BackendKind::Fallback; }
std::string backendKindToString(BackendKind k){ switch(k){ case BackendKind::TensorRT:return "tensorrt"; case BackendKind::OnnxRuntimeTensorRT:return "onnxruntime_tensorrt"; case BackendKind::OnnxRuntimeCUDA:return "onnxruntime_cuda"; case BackendKind::OnnxRuntimeCPU:return "onnxruntime_cpu"; case BackendKind::OpenVINO:return "openvino"; case BackendKind::LibTorch:return "libtorch"; case BackendKind::LlamaCpp:return "llamacpp"; case BackendKind::Fallback:return "fallback";} return "fallback"; }
std::string modelFormatToString(ModelFormat f){ switch(f){ case ModelFormat::Onnx:return "onnx"; case ModelFormat::TensorRTEngine:return "engine"; case ModelFormat::TorchScript:return "torchscript"; case ModelFormat::OpenVINOIR:return "openvino_ir"; case ModelFormat::GGUF:return "gguf"; case ModelFormat::Unknown:return "unknown";} return "unknown"; }
std::string inferenceStatusToString(InferenceStatus s){ switch(s){ case InferenceStatus::Success:return "success"; case InferenceStatus::NotExecuted:return "not_executed"; case InferenceStatus::BackendUnavailable:return "backend_unavailable"; case InferenceStatus::InvalidInput:return "invalid_input"; case InferenceStatus::RuntimeError:return "runtime_error";} return "runtime_error"; }
std::vector<int64_t> parseShapeCsv(const std::string &csv){ std::vector<int64_t> r; if(norm(trim(csv))=="dynamic") return r; std::stringstream ss(csv); std::string t; while(std::getline(ss,t,',')){ int64_t value=0; if(!parseInt64NoThrow(t,value)) return {}; r.push_back(value); } return r; }
std::vector<float> parseFloatCsv(const std::string &csv){ std::vector<float> r; std::stringstream ss(csv); std::string t; while(std::getline(ss,t,',')){ float value=0.0f; if(!parseFloatNoThrow(t,value)) return {}; r.push_back(value); } return r; }
size_t productOfShape(const std::vector<int64_t> &s){ size_t p=1; for(auto d:s){ if(d<=0) return 0; p*=static_cast<size_t>(d); } return s.empty()?0:p; }
bool validateShape(const std::vector<int64_t> &s){ return !s.empty() && productOfShape(s)>0; }
bool validateInputMatchesShape(const TensorBuffer &i){ const auto n=productOfShape(i.spec.shape); if(n==0) return false; if(!i.f32_data.empty()) return i.f32_data.size()==n; if(!i.i8_data.empty()) return i.i8_data.size()==n; return true; }
bool backendSupportsFormat(const BackendCapabilities &c, ModelFormat f){ switch(f){ case ModelFormat::Onnx:return c.supports_onnx; case ModelFormat::TensorRTEngine:return c.supports_engine; case ModelFormat::TorchScript:return c.supports_torchscript; case ModelFormat::OpenVINOIR:return c.supports_openvino_ir; case ModelFormat::GGUF:return c.supports_gguf; case ModelFormat::Unknown:return false;} return false; }
} // namespace shorthand::ai
