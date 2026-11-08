#include "PrepareAot.h"
#include "StaticAffineModel.h"
#include <filesystem>
#include <fstream>
#include <functional>
#include <exception>
#include <stdexcept>
namespace shorthand::aot {
bool prepare(semantic_ir::ProgramIR &program,bool hybrid,std::string &error) {
    namespace sir=semantic_ir;
    auto fail=[&](const std::string &reason){error="AOT: "+reason;return false;};
    try {
        if(program.models.size()>16) return fail("at most 16 models supported");
        if(!hybrid && (!program.contracts.empty() || !program.measurements.empty())) return fail("runtime measurement contracts require compile-hybrid");
        std::size_t nodes=0;
        std::function<bool(const sir::Statement &,unsigned)> legacy=[&](const sir::Statement &s,unsigned depth){
            if(++nodes>100000 || depth>=256) throw std::runtime_error("source work/depth limit exceeded");
            if(s.kind==sir::Statement::Kind::LegacyInfer) return true;
            for(const auto &v:s.body) if(legacy(v,depth+1)) return true;
            for(const auto &v:s.alternative) if(legacy(v,depth+1)) return true;
            return false;
        };
        if(legacy(program.body,0)) return fail("legacy string inference is outside explicit tensor AOT modes");
        for(const auto &f:program.functions) if(legacy(f.body,0)) return fail("legacy string inference is outside explicit tensor AOT modes");
        auto candidate=program; // failed preparation cannot leave partial state
        candidate.aot_mode=hybrid?sir::AotMode::Hybrid:sir::AotMode::Only;
        std::size_t total=0;
        for(auto &m:candidate.models) {
            if(m.format!=sir::ModelFormat::Onnx || m.precision!=sir::ElementType::Float32 || m.backend_preference!=std::vector<sir::BackendKind>{sir::BackendKind::OnnxRuntimeCPU}) return fail("FP32 ONNX with exactly onnxruntime_cpu preference required: "+m.name);
            if(m.path.empty() || m.path.find('\0')!=std::string::npos) return fail("invalid model path: "+m.name);
            std::error_code ec;
            if(!std::filesystem::is_regular_file(m.path,ec) || ec) return fail("readable regular model file required: "+m.name);
            const auto size=std::filesystem::file_size(m.path,ec);
            if(ec || !size || size>16*1024*1024 || total+size>64*1024*1024) return fail("16 MiB per-model / 64 MiB total snapshot limit: "+m.name);
            std::ifstream file(m.path,std::ios::binary);std::vector<unsigned char> bytes(size);
            if(!file.read(reinterpret_cast<char *>(bytes.data()),bytes.size()) || file.peek()!=std::char_traits<char>::eof()) return fail("snapshot read failed or size changed: "+m.name);
            total+=bytes.size();auto result=inspectStaticAffine(std::move(bytes));
            if(result.disposition==ImportDisposition::Rejected || (!hybrid && result.disposition!=ImportDisposition::Supported)) return fail(m.name+": "+result.reason);
            if(result.disposition==ImportDisposition::RequiresBackendValidation) continue;
            const auto &a=result.model;
            if(m.input_shape.dims!=std::vector<int64_t>{static_cast<int64_t>(a.batch()),static_cast<int64_t>(a.inputs())} || m.output_shape.dims!=std::vector<int64_t>{static_cast<int64_t>(a.batch()),static_cast<int64_t>(a.outputs())}) return fail("source/model signature mismatch: "+m.name);
            m.aot=sir::AffineKernel{a.sha256(),a.batch(),a.inputs(),a.outputs(),a.weights(),a.bias()};
        }
        program=std::move(candidate);return true;
    } catch(const std::exception &e){return fail(e.what());}
}
}
