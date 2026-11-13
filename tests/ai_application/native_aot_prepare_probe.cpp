#include "aot/PrepareAot.h"
#include <cassert>
#include <iostream>
#include <fstream>
#include <cstdio>
namespace sir=shorthand::semantic_ir;
sir::ProgramIR program(const char *path){
    sir::ProgramIR p;sir::ModelOp m("affine");m.format=sir::ModelFormat::Onnx;m.path=path;m.precision=sir::ElementType::Float32;
    m.input_shape.dims={1,1};m.output_shape.dims={1,1};m.backend_preference={sir::BackendKind::OnnxRuntimeCPU};p.models.push_back(m);return p;
}
int main(int argc,char **argv){
    assert(argc==3);std::string error;
    auto accepted=program(argv[1]);assert(shorthand::aot::prepare(accepted,false,error));assert(accepted.models[0].aot && accepted.aot_mode==sir::AotMode::Only);
    auto reject=[&](sir::ProgramIR p){const auto before=p.models.size();assert(!shorthand::aot::prepare(p,false,error));assert(!error.empty() && p.aot_mode==sir::AotMode::Runtime && p.models.size()==before);for(const auto &m:p.models) assert(!m.aot);};
    auto p=program(argv[1]);p.models[0].input_shape.dims={2,1};reject(p);
    p=program(argv[1]);p.models[0].path=argv[2];reject(p);assert(shorthand::aot::prepare(p,true,error));assert(!p.models[0].aot && p.aot_mode==sir::AotMode::Hybrid);
    p=program(argv[1]);auto second=p.models[0];second.name="missing";second.path="/missing/short_hand_aot_fixture.onnx";p.models.push_back(second);reject(p);
    p=program(argv[1]);p.models[0].path.push_back('\0');p.models[0].path+="ignored";reject(p);
    p=program(argv[1]);const auto copied=p.models[0];p.models.assign(17,copied);reject(p);
    p=program(argv[1]);p.body.kind=sir::Statement::Kind::LegacyInfer;reject(p);
    p=program(argv[1]);sir::Statement deep;for(int i=0;i<260;++i){sir::Statement parent;parent.body.push_back(std::move(deep));deep=std::move(parent);}p.body=deep;reject(p);
    p=program(argv[1]);{std::ofstream large("too-large.onnx",std::ios::binary);large.seekp(16*1024*1024);large.put('x');assert(large.good());}p.models[0].path="too-large.onnx";reject(p);std::remove("too-large.onnx");
    std::cout<<"PASS native AOT preparation: atomic rejection, path/signature/size, hybrid and work bounds\n";
}
