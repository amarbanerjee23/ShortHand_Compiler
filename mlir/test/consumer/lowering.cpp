#include "ShortHand/Conversion/Lowering.h"
#include <fstream>
#include <iostream>
#include <sstream>
namespace sir=shorthand::semantic_ir;
using T=sir::ScalarType; using E=sir::Expression; using S=sir::Statement; using C=sir::Composite;
E integer(int n) { E e; e.integer=n; return e; }
E variable(std::string n,T t,std::string id={}) { E e; e.kind=E::Kind::Variable; e.name=std::move(n); e.type=t; e.composite=std::move(id); return e; }
E construct(std::string id,std::vector<E> fields) { E e; e.kind=E::Kind::Construct; e.type=T::Composite; e.composite=std::move(id); e.operands=std::move(fields); return e; }
E project(E v,int field,T t) { E e; e.kind=E::Kind::Project; e.type=t; e.integer=field; e.operands.push_back(std::move(v)); return e; }
void append(sir::ProgramIR &p,S::Kind k,std::vector<E> expressions={},std::size_t d=0) { S s; s.kind=k; s.expressions=std::move(expressions); s.declaration=d; p.body.body.push_back(std::move(s)); }
sir::ProgramIR apiProgram() {
    sir::ProgramIR p; p.composites.push_back({C::Kind::Record,"Evidence",{T::Int32},{"count"}}); p.globals.push_back({"value",T::Composite,0,{},"Evidence"});
    sir::Function f; f.name="adjust"; f.result=T::Composite; f.result_composite="Evidence"; f.parameters.push_back({"item",T::Composite,0,{},"Evidence"});
    S update; update.kind=S::Kind::Update; update.expressions={variable("item",T::Composite,"Evidence"),integer(42)};
    S ret; ret.kind=S::Kind::Return; ret.expressions={variable("item",T::Composite,"Evidence")}; f.body.body={update,ret}; p.functions.push_back(f);
    E call; call.kind=E::Kind::Call; call.name="adjust"; call.type=T::Composite; call.composite="Evidence"; call.operands={construct("Evidence",{integer(1)})};
    append(p,S::Kind::Assign,{variable("value",T::Composite,"Evidence"),call}); append(p,S::Kind::Print,{project(variable("value",T::Composite,"Evidence"),0,T::Int32)}); return p;
}
sir::ProgramIR aiProgram(const std::string &path) {
    sir::ProgramIR p; sir::ModelOp m("identity"); m.format=sir::ModelFormat::Onnx; m.path=path; m.task="identity"; m.precision=sir::ElementType::Float32;
    m.input_shape.dims={1}; m.output_shape.dims={1}; m.backend_preference={sir::BackendKind::OnnxRuntimeCPU}; m.quality_guardrail="exact output preserved"; p.models.push_back(m);
    p.tensors.emplace_back("input",sir::ElementType::Float32,sir::TensorShape{{1}}); p.tensors.back().values={42}; p.tensors.emplace_back("output",sir::ElementType::Float32,sir::TensorShape{{1}});
    p.inferences.emplace_back("identity","input","output"); append(p,S::Kind::Tensor,{},0); append(p,S::Kind::Tensor,{},1); append(p,S::Kind::Infer);
    E output; output.kind=E::Kind::TensorIndex; output.type=T::Float64; output.name="output"; output.operands={integer(0)}; append(p,S::Kind::Print,{output});
    sir::GreenAIContractOp c("workload"); c.functional_unit="1 successful inference"; c.success_criteria="exact output"; c.quality_guardrail="output == 42"; c.boundary={"compute","memory"};
    c.measurement_quality="MQ1"; c.data_quality="DQ1"; c.carbon_factor=171.09; c.claims_mode="evidence_only"; p.contracts.push_back(c);
    sir::GreenAIMeasurementOp measure("workload"); measure.backend="onnxruntime_cpu"; measure.inferences=1; measure.watts=10; measure.seconds=0.1; p.measurements.push_back(measure); append(p,S::Kind::Measurement); return p;
}
int mutations() {
    auto valid=apiProgram(); std::string error; if(!shorthand::lowering::verifySemanticIR(valid,error)) return 1; unsigned rejected=0;
    auto reject=[&](const sir::ProgramIR &p) { if(shorthand::lowering::verifySemanticIR(p,error)||error.empty()) return false; std::ostringstream out; out<<"unchanged"; if(shorthand::lowering::emitProgram(p,out,true,error)||out.str()!="unchanged") return false; ++rejected; return true; };
    auto p=valid; p.body.body[0].expressions[1].composite="Unknown"; if(!reject(p)) return 2;
    p=valid; p.composites[0].fields[0]=T::Composite; if(!reject(p)) return 3;
    p=valid; p.body.body[1].expressions[0].integer=-1; if(!reject(p)) return 4;
    p=valid; p.functions[0].body.body[0].expressions[1].type=T::Bool; if(!reject(p)) return 5;
    p=valid; p.functions[0].name="printf"; if(!reject(p)) return 6;
    p=valid; p.globals[0].name="short_ai_infer_f32"; if(!reject(p)) return 7;
    p=valid; p.functions[0].parameters[0].composite="Unknown"; if(!reject(p)) return 8;
    p=aiProgram("m.onnx"); p.tensors[0].shape.dims={INT64_MAX,2}; if(!reject(p)) return 9;
    p=aiProgram("m.onnx"); p.tensors[0].values={1e300}; if(!reject(p)) return 10;
    p=aiProgram("m.onnx"); p.body.body[0].declaration=99; if(!reject(p)) return 11;
    p=valid; E nested=integer(1); for(unsigned i=0;i<260;++i) { E next; next.kind=E::Kind::Unary; next.opcode=1; next.operands.push_back(std::move(nested)); nested=std::move(next); } p.body.body[1].expressions={nested}; if(!reject(p)) return 12;
    if(rejected!=11) return 13; std::cout<<"PASS installed SemanticIR mutation and bounded work checks\n"; return 0;
}
int main(int argc,char **argv) {
    if(argc==2&&std::string(argv[1])=="verify") return mutations(); if(argc<4) return 64;
    std::string mode=argv[1]; auto p=mode=="ai"?aiProgram(argc>4?argv[4]:"m.onnx"):apiProgram();
    if(mode=="missing-return") p.functions[0].body.body.clear();
    if(mode=="bad-tag"||mode=="inactive") { p=sir::ProgramIR{}; p.composites.push_back({C::Kind::Option,"Maybe",{T::Int32},{"value"}}); auto v=construct("Maybe",{integer(mode=="bad-tag"?2:0),integer(9)}); append(p,S::Kind::Print,{project(v,mode=="bad-tag"?0:1,T::Int32)}); }
    std::ostringstream out; std::string error; if(!shorthand::lowering::emitProgram(p,out,std::string(argv[3])!="mlir",error,std::string(argv[3])!="O0")) { std::cerr<<error; return 1; }
    std::ofstream file(argv[2]); file<<out.str(); return file.good()?0:1;
}
