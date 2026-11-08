#pragma once
#include "LoweringSupport.h"
#include "ShortHand/IR/ShortHandOps.h"

namespace shorthand::lowering {
// One frozen-ABI lowering for tensor SSA operations and source-owned buffers.
// Check receives an error predicate; its caller owns the surrounding CFG.
template<class Check>
void inferTensorBuffers(CodeBuilder &c, ir::ModelOp model, mlir::Value inputBuffer,
                        mlir::Value outputBuffer, Check check) {
    using namespace mlir;
    auto signature=cast<ir::ModelType>(model.getSignature());
    auto input=signature.getInput(),output=signature.getOutput();
    auto shape=[](RankedTensorType t) {
        std::string result;
        for(auto dim:t.getShape()) { if(!result.empty()) result+=','; result+=std::to_string(dim); }
        return result;
    };
    auto strings=[&](StringRef name,ArrayRef<std::string> text) {
        SmallVector<Value> args; for(const auto &value:text) args.push_back(c.string(value));
        return c.call(name,c.b.getI32Type(),args).getResult();
    };
    auto status=[&](Value value,StringRef reason) {
        check(c.b.create<LLVM::ICmpOp>(c.loc,LLVM::ICmpPredicate::ne,value,c.integer(0)),reason);
    };
    std::string preference=model.getBackend().getName().str();
    if(auto prefs=model->getAttrOfType<ArrayAttr>("shorthand.backend_preference")) {
        preference.clear(); for(auto pref:prefs) { if(!preference.empty()) preference+=','; preference+=cast<StringAttr>(pref).str(); }
    }
    status(strings("short_ai_register_model",{model.getSymName().str(),model.getFormat().str(),model.getPath().str(),model.getTask().str(),"float32",shape(input),shape(output),preference}),"model registration failed");
    std::string inputName=model.getSymName().str()+"_input_"+shape(input),outputName=model.getSymName().str()+"_output_"+shape(output);
    status(strings("short_ai_register_tensor",{inputName,"float32",shape(input),std::to_string(input.getRank()),std::to_string(input.getNumElements())}),"input registration failed");
    status(strings("short_ai_register_tensor",{outputName,"float32",shape(output),std::to_string(output.getRank()),std::to_string(output.getNumElements())}),"output registration failed");
    Value count=c.slot(c.b.getI32Type());
    c.b.create<LLVM::StoreOp>(c.loc,c.integer(0),count);
    status(c.call("short_ai_infer_f32",c.b.getI32Type(),{c.string(model.getSymName()),c.string(inputName),inputBuffer,c.integer(input.getNumElements()),
        c.string(outputName),outputBuffer,c.integer(output.getNumElements()),count}).getResult(),"AI runtime did not execute inference");
    auto actual=c.b.create<LLVM::LoadOp>(c.loc,c.b.getI32Type(),count);
    check(c.b.create<LLVM::ICmpOp>(c.loc,LLVM::ICmpPredicate::ne,actual,c.integer(output.getNumElements())),"AI runtime returned an unexpected tensor size");
}
}
