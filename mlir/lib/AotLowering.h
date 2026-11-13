#pragma once
#include "LoweringSupport.h"
#include "ShortHand/SemanticIR.h"
namespace shorthand::lowering {
inline std::string aotSymbol(const semantic_ir::AffineKernel &a){return "__sh_aot_v1_"+a.sha256;}
inline void emitAffineKernel(CodeBuilder &c,const semantic_ir::AffineKernel &a) {
    using namespace mlir;
    auto &b=c.b;const auto name=aotSymbol(a);if(c.module.lookupSymbol(name)) return;
    OpBuilder::InsertionGuard guard(b);
    auto constant=[&](StringRef suffix,const std::vector<float> &values){
        b.setInsertionPointToEnd(c.module.getBody());auto type=LLVM::LLVMArrayType::get(b.getF32Type(),values.size());
        auto data=DenseElementsAttr::get(RankedTensorType::get({static_cast<int64_t>(values.size())},b.getF32Type()),ArrayRef<float>(values));
        auto global=b.create<LLVM::GlobalOp>(c.loc,type,true,LLVM::Linkage::Private,name+suffix.str(),Attribute());
        auto *block=new Block;global.getInitializerRegion().push_back(block);b.setInsertionPointToStart(block);
        auto value=b.create<LLVM::ConstantOp>(c.loc,type,data);b.create<LLVM::ReturnOp>(c.loc,value.getResult());return global;
    };
    auto weights=constant("_weights",a.weights),bias=constant("_bias",a.bias);
    auto fn=c.function(name,b.getI32Type(),{c.ptr(),b.getI64Type(),c.ptr(),b.getI64Type(),c.ptr(),b.getI64Type()});
    auto *entry=c.entryBlock(fn);auto block=[&](){auto *v=new Block;fn.getBody().push_back(v);return v;};
    auto *denied=block(),*outer=block(),*inner=block(),*multiply=block(),*store=block(),*commit=block();
    outer->addArgument(b.getI64Type(),c.loc);inner->addArgument(b.getI64Type(),c.loc);inner->addArgument(b.getF32Type(),c.loc);
    b.setInsertionPointToStart(entry);auto input=entry->getArgument(0),output=entry->getArgument(2),workspace=entry->getArgument(4);
    auto status=c.call("sh_aot_v1_prepare",b.getI32Type(),{input,entry->getArgument(1),output,entry->getArgument(3),workspace,entry->getArgument(5),c.integer(a.batch*a.inputs,64),c.integer(a.batch*a.outputs,64)}).getResult();
    auto wp=b.create<LLVM::AddressOfOp>(c.loc,weights),bp=b.create<LLVM::AddressOfOp>(c.loc,bias);auto zero=c.integer(0,64);
    b.create<LLVM::CondBrOp>(c.loc,b.create<LLVM::ICmpOp>(c.loc,LLVM::ICmpPredicate::ne,status,c.integer(0)),denied,ValueRange{},outer,ValueRange{zero});
    b.setInsertionPointToStart(denied);b.create<LLVM::ReturnOp>(c.loc,status);
    b.setInsertionPointToStart(outer);auto index=outer->getArgument(0);
    auto row=b.create<LLVM::UDivOp>(c.loc,index,c.integer(a.outputs,64));auto column=b.create<LLVM::URemOp>(c.loc,index,c.integer(a.outputs,64));auto begin=b.create<LLVM::MulOp>(c.loc,row,c.integer(a.inputs,64));
    auto fpzero=b.create<LLVM::ConstantOp>(c.loc,b.getF32Type(),b.getF32FloatAttr(0));
    b.create<LLVM::CondBrOp>(c.loc,b.create<LLVM::ICmpOp>(c.loc,LLVM::ICmpPredicate::ult,index,c.integer(a.batch*a.outputs,64)),inner,ValueRange{zero,fpzero},commit,ValueRange{});
    b.setInsertionPointToStart(inner);auto k=inner->getArgument(0),sum=inner->getArgument(1);
    b.create<LLVM::CondBrOp>(c.loc,b.create<LLVM::ICmpOp>(c.loc,LLVM::ICmpPredicate::ult,k,c.integer(a.inputs,64)),multiply,store);
    b.setInsertionPointToStart(multiply);auto xi=b.create<LLVM::AddOp>(c.loc,begin,k);auto wi=b.create<LLVM::AddOp>(c.loc,b.create<LLVM::MulOp>(c.loc,k,c.integer(a.outputs,64)),column);
    auto x=b.create<LLVM::LoadOp>(c.loc,b.getF32Type(),c.gep(b.getF32Type(),input,{xi.getResult()}));auto w=b.create<LLVM::LoadOp>(c.loc,b.getF32Type(),c.gep(b.getF32Type(),wp,{wi.getResult()}));
    // Strict increasing-k FP32 reduction; no fast-math, FMA or reassociation.
    auto product=b.create<LLVM::FMulOp>(c.loc,x,w);auto next=b.create<LLVM::FAddOp>(c.loc,sum,product);
    b.create<LLVM::BrOp>(c.loc,ValueRange{b.create<LLVM::AddOp>(c.loc,k,c.integer(1,64)),next},inner);
    b.setInsertionPointToStart(store);auto bv=b.create<LLVM::LoadOp>(c.loc,b.getF32Type(),c.gep(b.getF32Type(),bp,{column.getResult()}));auto result=b.create<LLVM::FAddOp>(c.loc,sum,bv);
    b.create<LLVM::StoreOp>(c.loc,result,c.gep(b.getF32Type(),workspace,{index}));b.create<LLVM::BrOp>(c.loc,ValueRange{b.create<LLVM::AddOp>(c.loc,index,c.integer(1,64))},outer);
    b.setInsertionPointToStart(commit);auto committed=c.call("sh_aot_v1_commit",b.getI32Type(),{output,entry->getArgument(3),workspace,entry->getArgument(5)}).getResult();b.create<LLVM::ReturnOp>(c.loc,committed);
}
}
