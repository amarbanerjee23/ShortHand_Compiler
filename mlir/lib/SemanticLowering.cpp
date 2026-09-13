#include "ShortHand/Conversion/Lowering.h"
#include "ShortHand/IR/ShortHandOps.h"
#include "LoweringSupport.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Target/LLVMIR/Export.h"
#include "mlir/Target/LLVMIR/Dialect/Builtin/BuiltinToLLVMIRTranslation.h"
#include "mlir/Target/LLVMIR/Dialect/LLVMIR/LLVMToLLVMIRTranslation.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"
#include <map>
#include <ostream>

using namespace mlir;
namespace shorthand::lowering {
namespace {
namespace sir=semantic_ir;
using T=sir::ScalarType;
using E=sir::Expression;
using S=sir::Statement;
StringRef backend(sir::BackendKind kind) {
    switch(kind) {
    case sir::BackendKind::Fallback:return "fallback";
    case sir::BackendKind::OnnxRuntimeCPU:return "onnxruntime_cpu";
    case sir::BackendKind::OnnxRuntimeCUDA:return "onnxruntime_cuda";
    case sir::BackendKind::OnnxRuntimeTensorRT:return "onnxruntime_tensorrt";
    case sir::BackendKind::TensorRT:return "tensorrt";
    case sir::BackendKind::OpenVINO:return "openvino";
    case sir::BackendKind::LibTorch:return "libtorch";
    case sir::BackendKind::LlamaCpp:return "llamacpp";
    }
    return "unknown";
}
StringRef format(sir::ModelFormat kind) {
    switch(kind) {
    case sir::ModelFormat::Onnx:return "onnx";
    case sir::ModelFormat::TensorRTEngine:return "tensorrt_engine";
    case sir::ModelFormat::TorchScript:return "torchscript";
    case sir::ModelFormat::OpenVINOIR:return "openvino_ir";
    case sir::ModelFormat::GGUF:return "gguf";
    default:return "unknown";
    }
}
struct Generator {
    const sir::ProgramIR &p;
    OpBuilder b;
    OwningOpRef<ModuleOp> module;
    CodeBuilder c;
    struct Binding { Value pointer; Type storage; T type; int64_t extent; std::string composite; };
    std::map<std::string,ir::CompositeType> composites;
    std::map<std::string,sir::Variable> globals;
    std::vector<std::map<std::string,Binding>> scopes;
    std::map<std::string,ir::ModelOp> models;
    std::map<std::string,RankedTensorType> tensors;
    std::map<std::string,ir::GreenAIContractOp> contracts;
    std::vector<std::map<std::string,Block *>> labels;
    std::vector<Block *> breaks,continues;
    LLVM::LLVMFuncOp function;
    bool invalid=false;
    Generator(MLIRContext &ctx,const sir::ProgramIR &program):p(program),b(&ctx),
        module(ModuleOp::create(b.getUnknownLoc())),c{b,*module,b.getUnknownLoc()} {}
    Location location(const sir::SourceRange &r) {
        return r.isKnown()?Location(FileLineColLoc::get(b.getContext(),r.begin.file,r.begin.line,r.begin.column)):b.getUnknownLoc();
    }
    Type type(T t,const std::string &identity="") {
        switch(t) {
        case T::Int32:case T::Bool:return b.getI32Type();
        case T::Float64:return b.getF64Type();
        case T::String:return c.ptr();
        case T::Void:return c.voidTy();
        case T::Composite: {
            auto ct=composites.at(identity); SmallVector<Type> fields;
            if(ct.getKind()=="slice") fields={c.ptr(),b.getI64Type()};
            else { if(ct.getKind()!="record") fields.push_back(b.getI32Type()); for(auto field:ct.getFields()) fields.push_back(cast<TypeAttr>(field).getValue()); }
            return LLVM::LLVMStructType::getLiteral(b.getContext(),fields);
        }
        }
        return {};
    }
    Type element(sir::ElementType t) {
        switch(t) {
        case sir::ElementType::Float32:return b.getF32Type();
        case sir::ElementType::Float16:return b.getF16Type();
        case sir::ElementType::BFloat16:return b.getBF16Type();
        case sir::ElementType::Int4:return b.getIntegerType(4);
        case sir::ElementType::Int8:return b.getI8Type();
        case sir::ElementType::Int32:return b.getI32Type();
        default:return {};
        }
    }
    RankedTensorType tensorType(const sir::TensorShape &s,sir::ElementType e) {
        auto t=element(e); return t?RankedTensorType::get(s.dims,t):RankedTensorType();
    }
    Type storage(const sir::Variable &v) { return v.extent?Type(LLVM::LLVMArrayType::get(type(v.type,v.composite),v.extent)):type(v.type,v.composite); }
    Type tensorStorage(RankedTensorType t) { return LLVM::LLVMArrayType::get(t.getElementType(),t.getNumElements()); }
    template<class Op> Op op(TypeRange results,ValueRange args,ArrayRef<NamedAttribute> attrs) {
        OperationState state(c.loc,Op::getOperationName()); state.addTypes(results); state.addOperands(args); state.addAttributes(attrs);
        return cast<Op>(b.create(state));
    }
    NamedAttribute attr(StringRef name,Attribute value) { return b.getNamedAttr(name,value); }
    Value zero(Type t) { return b.create<LLVM::ZeroOp>(c.loc,t); }
    void global(StringRef name,Type t) {
        OpBuilder::InsertionGuard guard(b); b.setInsertionPointToEnd(module->getBody());
        auto g=b.create<LLVM::GlobalOp>(c.loc,t,false,LLVM::Linkage::Internal,name,Attribute());
        auto *entry=new Block; g.getInitializerRegion().push_back(entry); b.setInsertionPointToStart(entry);
        b.create<LLVM::ReturnOp>(c.loc,zero(t));
    }
    Binding lookup(StringRef name) {
        for(auto it=scopes.rbegin();it!=scopes.rend();++it) { auto found=it->find(name.str()); if(found!=it->end()) return found->second; }
        const auto &v=globals.at(name.str());
        return {b.create<LLVM::AddressOfOp>(c.loc,module->lookupSymbol<LLVM::GlobalOp>(name)),storage(v),v.type,v.extent,v.composite};
    }
    Block *block() { auto *result=new Block; function.getBody().push_back(result); return result; }
    bool terminated() { auto *at=b.getInsertionBlock(); return !at->empty()&&at->back().hasTrait<OpTrait::IsTerminator>(); }
    void branch(Block *to) { if(!terminated()) b.create<LLVM::BrOp>(c.loc,ValueRange{},to); }
    Value boolValue(Value v) { return b.create<LLVM::ICmpOp>(c.loc,LLVM::ICmpPredicate::ne,v,c.integer(0)); }
    Value boolean(Value v) { return b.create<LLVM::ZExtOp>(c.loc,b.getI32Type(),v); }
    void failureIf(Value condition,StringRef code,StringRef text) {
        Block *bad=block(),*next=block(); b.create<LLVM::CondBrOp>(c.loc,condition,bad,next);
        b.setInsertionPointToStart(bad); c.failureBody(code,text); b.setInsertionPointToStart(next);
    }
    Value pointer(const E &e) {
        // Index evaluation precedes variable access, matching the reference backend.
        Value index; if(e.kind==E::Kind::Index) index=expr(e.operands[0]);
        auto v=lookup(e.name); if(e.kind==E::Kind::Variable) return v.pointer;
        Value negative=b.create<LLVM::ICmpOp>(c.loc,LLVM::ICmpPredicate::slt,index,c.integer(0));
        Value high=b.create<LLVM::ICmpOp>(c.loc,LLVM::ICmpPredicate::sge,index,c.integer(v.extent));
        failureIf(b.create<LLVM::OrOp>(c.loc,negative,high),"SHD7002","array index is outside declared bounds");
        return b.create<LLVM::GEPOp>(c.loc,c.ptr(),v.storage,v.pointer,ArrayRef<LLVM::GEPArg>{0,index},true);
    }
    Value safeString(Value value) {
        auto isNull=b.create<LLVM::ICmpOp>(c.loc,LLVM::ICmpPredicate::eq,value,zero(c.ptr()));
        return b.create<LLVM::SelectOp>(c.loc,isNull,c.string(""),value);
    }
    Value binary(const E &e,Value l,Value r) {
        T t=e.operands[0].type; int op=e.opcode;
        if(t==T::String) {
            auto compare=c.call("strcmp",b.getI32Type(),{safeString(l),safeString(r)}).getResult();
            return boolean(b.create<LLVM::ICmpOp>(c.loc,op==10?LLVM::ICmpPredicate::eq:LLVM::ICmpPredicate::ne,compare,c.integer(0)));
        }
        if(t==T::Float64) {
            if(op==1) return b.create<LLVM::FAddOp>(c.loc,l,r);
            if(op==2) return b.create<LLVM::FSubOp>(c.loc,l,r);
            if(op==3) return b.create<LLVM::FMulOp>(c.loc,l,r);
            if(op==4) {
                failureIf(b.create<LLVM::FCmpOp>(c.loc,LLVM::FCmpPredicate::oeq,r,c.floating(0)),"SHD7001","float division by zero");
                return b.create<LLVM::FDivOp>(c.loc,l,r);
            }
            const LLVM::FCmpPredicate predicates[]={LLVM::FCmpPredicate::olt,LLVM::FCmpPredicate::ogt,
                LLVM::FCmpPredicate::ole,LLVM::FCmpPredicate::oge,LLVM::FCmpPredicate::oeq,LLVM::FCmpPredicate::une};
            return boolean(b.create<LLVM::FCmpOp>(c.loc,predicates[op-6],l,r));
        }
        if(op==1) return b.create<LLVM::AddOp>(c.loc,l,r);
        if(op==2) return b.create<LLVM::SubOp>(c.loc,l,r);
        if(op==3) return b.create<LLVM::MulOp>(c.loc,l,r);
        if(op==4||op==5) {
            auto z=b.create<LLVM::ICmpOp>(c.loc,LLVM::ICmpPredicate::eq,r,c.integer(0));
            auto min=b.create<LLVM::ICmpOp>(c.loc,LLVM::ICmpPredicate::eq,l,c.integer(INT32_MIN));
            auto neg=b.create<LLVM::ICmpOp>(c.loc,LLVM::ICmpPredicate::eq,r,c.integer(-1));
            failureIf(b.create<LLVM::OrOp>(c.loc,z,b.create<LLVM::AndOp>(c.loc,min,neg)),"SHD7001","integer division/remainder domain error");
            return op==4?Value(b.create<LLVM::SDivOp>(c.loc,l,r)):Value(b.create<LLVM::SRemOp>(c.loc,l,r));
        }
        if(op==12) return boolean(b.create<LLVM::OrOp>(c.loc,boolValue(l),boolValue(r)));
        if(op==13) return boolean(b.create<LLVM::AndOp>(c.loc,boolValue(l),boolValue(r)));
        const LLVM::ICmpPredicate predicates[]={LLVM::ICmpPredicate::slt,LLVM::ICmpPredicate::sgt,
            LLVM::ICmpPredicate::sle,LLVM::ICmpPredicate::sge,LLVM::ICmpPredicate::eq,LLVM::ICmpPredicate::ne};
        return boolean(b.create<LLVM::ICmpOp>(c.loc,predicates[op-6],l,r));
    }
    Value expr(const E &e) {
        c.loc=location(e.source_range);
        switch(e.kind) {
        case E::Kind::Integer:case E::Kind::Boolean:return c.integer(e.integer);
        case E::Kind::Float:return c.floating(e.decimal);
        case E::Kind::String:return c.string(e.name);
        case E::Kind::Variable:case E::Kind::Index:return b.create<LLVM::LoadOp>(c.loc,type(e.type,e.composite),pointer(e));
        case E::Kind::Unary: {
            auto value=expr(e.operands[0]);
            return e.type==T::Float64?Value(b.create<LLVM::FNegOp>(c.loc,value)):Value(b.create<LLVM::SubOp>(c.loc,c.integer(0),value));
        }
        case E::Kind::Binary: { auto l=expr(e.operands[0]); auto r=expr(e.operands[1]); return binary(e,l,r); }
        case E::Kind::Call: {
            SmallVector<Value> args; for(const auto &a:e.operands) args.push_back(expr(a));
            auto call=b.create<LLVM::CallOp>(c.loc,module->lookupSymbol<LLVM::LLVMFuncOp>(e.name),args);
            return call.getNumResults()?call.getResult():Value();
        }
        case E::Kind::Construct: {
            SmallVector<Value> fields; for(const auto &field:e.operands) fields.push_back(expr(field));
            auto value=op<ir::ValueOp>(TypeRange{composites.at(e.composite)},fields,{}).getResult();
            return b.create<UnrealizedConversionCastOp>(c.loc,TypeRange{type(e.type,e.composite)},ValueRange{value}).getResult(0);
        }
        case E::Kind::Project: {
            const auto &source=e.operands[0]; auto loaded=expr(source);
            auto value=b.create<UnrealizedConversionCastOp>(c.loc,TypeRange{composites.at(source.composite)},ValueRange{loaded}).getResult(0);
            return op<ir::ProjectOp>(TypeRange{type(e.type)},ValueRange{value},{attr("index",b.getI64IntegerAttr(e.integer))}).getResult();
        }
        case E::Kind::Slice: {
            auto offset=expr(e.operands[0]),length=expr(e.operands[1]); auto owner=lookup(e.name);
            Value offset64=b.create<LLVM::SExtOp>(c.loc,b.getI64Type(),offset),length64=b.create<LLVM::SExtOp>(c.loc,b.getI64Type(),length);
            auto negative=b.create<LLVM::OrOp>(c.loc,b.create<LLVM::ICmpOp>(c.loc,LLVM::ICmpPredicate::slt,offset,c.integer(0)),b.create<LLVM::ICmpOp>(c.loc,LLVM::ICmpPredicate::slt,length,c.integer(0)));
            auto end=b.create<LLVM::AddOp>(c.loc,offset64,length64);
            auto high=b.create<LLVM::ICmpOp>(c.loc,LLVM::ICmpPredicate::sgt,end,c.integer(owner.extent,64));
            failureIf(b.create<LLVM::OrOp>(c.loc,negative,high),"SHD7002","slice exceeds its owner array");
            auto pointer=b.create<LLVM::GEPOp>(c.loc,c.ptr(),owner.storage,owner.pointer,ArrayRef<LLVM::GEPArg>{0,offset64},true);
            auto value=op<ir::ValueOp>(TypeRange{composites.at(e.composite)},ValueRange{pointer,length64},{}).getResult();
            return b.create<UnrealizedConversionCastOp>(c.loc,TypeRange{type(e.type,e.composite)},ValueRange{value}).getResult(0);
        }
        case E::Kind::SliceIndex: {
            auto index=expr(e.operands[0]); auto owner=lookup(e.name); auto ct=composites.at(owner.composite);
            auto loaded=b.create<LLVM::LoadOp>(c.loc,owner.storage,owner.pointer);
            auto value=b.create<UnrealizedConversionCastOp>(c.loc,TypeRange{ct},ValueRange{loaded}).getResult(0);
            auto pointer=op<ir::ProjectOp>(TypeRange{c.ptr()},ValueRange{value},{attr("index",b.getI64IntegerAttr(0))}).getResult();
            auto length=op<ir::ProjectOp>(TypeRange{b.getI64Type()},ValueRange{value},{attr("index",b.getI64IntegerAttr(1))}).getResult();
            Value index64=b.create<LLVM::SExtOp>(c.loc,b.getI64Type(),index);
            failureIf(b.create<LLVM::ICmpOp>(c.loc,LLVM::ICmpPredicate::uge,index64,length),"SHD7002","slice index is outside bounds");
            auto address=b.create<LLVM::GEPOp>(c.loc,c.ptr(),type(e.type),pointer,ArrayRef<LLVM::GEPArg>{index64},true);
            return b.create<LLVM::LoadOp>(c.loc,type(e.type),address);
        }
        case E::Kind::TensorIndex: {
            auto index=expr(e.operands[0]); auto tensor=tensors.at(e.name);
            failureIf(b.create<LLVM::ICmpOp>(c.loc,LLVM::ICmpPredicate::uge,index,c.integer(tensor.getNumElements())),"SHD7002","tensor index is outside bounds");
            auto ptr=b.create<LLVM::AddressOfOp>(c.loc,module->lookupSymbol<LLVM::GlobalOp>("__sh_tensor_"+e.name));
            auto address=b.create<LLVM::GEPOp>(c.loc,c.ptr(),tensorStorage(tensor),ptr,ArrayRef<LLVM::GEPArg>{0,index},true);
            auto value=b.create<LLVM::LoadOp>(c.loc,b.getF32Type(),address);
            return b.create<LLVM::FPExtOp>(c.loc,b.getF64Type(),value);
        }
        }
        return {};
    }
    void declare(const sir::Variable &v) {
        Type t=storage(v); Value slot=c.slot(t); b.create<LLVM::StoreOp>(c.loc,zero(t),slot);
        scopes.back()[v.name]={slot,t,v.type,v.extent,v.composite};
    }
    void statements(const std::vector<S> &list) { for(const auto &s:list) if(!terminated()||s.kind==S::Kind::Label) stmt(s); }
    void stmt(const S &s) {
        c.loc=location(s.source_range);
        switch(s.kind) {
        case S::Kind::Block: {
            if(s.lexical_scope) scopes.push_back({});
            std::map<std::string,Block *> table;
            for(const auto &child:s.body) if(child.kind==S::Kind::Label) table[child.name]=block();
            labels.push_back(std::move(table)); statements(s.body); labels.pop_back();
            if(s.lexical_scope) scopes.pop_back(); break;
        }
        case S::Kind::Declare:for(const auto &v:s.variables) declare(v);break;
        case S::Kind::Assign: { Value value=expr(s.expressions[1]); auto target=pointer(s.expressions[0]); b.create<LLVM::StoreOp>(c.loc,value,target); break; }
        case S::Kind::Update: {
            const auto &record=s.expressions[0]; auto loaded=expr(record),field=expr(s.expressions[1]); auto ct=composites.at(record.composite);
            auto value=b.create<UnrealizedConversionCastOp>(c.loc,TypeRange{ct},ValueRange{loaded}).getResult(0);
            auto updated=op<ir::UpdateOp>(TypeRange{ct},ValueRange{value,field},{attr("index",b.getI64IntegerAttr(s.declaration))}).getResult();
            auto stored=b.create<UnrealizedConversionCastOp>(c.loc,TypeRange{type(record.type,record.composite)},ValueRange{updated}).getResult(0);
            b.create<LLVM::StoreOp>(c.loc,stored,pointer(record)); break;
        }
        case S::Kind::Expression:expr(s.expressions[0]);break;
        case S::Kind::Read:
            for(const auto &e:s.expressions) c.call("scanf",b.getI32Type(),{c.string(e.type==T::Float64?"%lf":"%d"),pointer(e)},true);
            break;
        case S::Kind::Print:
            for(std::size_t i=0;i<s.expressions.size();++i) {
                const auto &e=s.expressions[i]; auto value=expr(e);
                if(e.type==T::String) value=safeString(value);
                c.call("printf",b.getI32Type(),{c.string(e.type==T::Float64?"%.17g":e.type==T::String?"%s":"%d"),value},true);
                c.call("printf",b.getI32Type(),{c.string("%s"),c.string(i+1==s.expressions.size()?"\n":" ")},true);
            } break;
        case S::Kind::If: {
            auto *yes=block(),*no=block(),*next=block(); auto condition=boolValue(expr(s.expressions[0]));
            b.create<LLVM::CondBrOp>(c.loc,condition,yes,no);
            b.setInsertionPointToStart(yes); statements(s.body); branch(next);
            b.setInsertionPointToStart(no); statements(s.alternative); branch(next);
            b.setInsertionPointToStart(next); break;
        }
        case S::Kind::While: {
            auto *cond=block(),*body=block(),*next=block(); branch(cond);
            b.setInsertionPointToStart(cond); auto test=boolValue(expr(s.expressions[0])); b.create<LLVM::CondBrOp>(c.loc,test,body,next);
            b.setInsertionPointToStart(body); breaks.push_back(next); continues.push_back(cond); statements(s.body); continues.pop_back(); breaks.pop_back(); branch(cond);
            b.setInsertionPointToStart(next); break;
        }
        case S::Kind::For: {
            auto start=expr(s.expressions[1]); auto ptr=pointer(s.expressions[0]); b.create<LLVM::StoreOp>(c.loc,start,ptr); auto saved=c.slot(b.getI32Type());
            auto *cond=block(),*body=block(),*stepBlock=block(),*next=block(); branch(cond);
            b.setInsertionPointToStart(cond); auto current=b.create<LLVM::LoadOp>(c.loc,b.getI32Type(),ptr); auto step=expr(s.expressions[2]);
            failureIf(b.create<LLVM::ICmpOp>(c.loc,LLVM::ICmpPredicate::eq,step,c.integer(0)),"SHD7003","loop step must be non-zero");
            b.create<LLVM::StoreOp>(c.loc,step,saved); auto end=expr(s.expressions[3]);
            auto positive=b.create<LLVM::ICmpOp>(c.loc,LLVM::ICmpPredicate::sgt,step,c.integer(0));
            auto forward=b.create<LLVM::ICmpOp>(c.loc,LLVM::ICmpPredicate::slt,current,end);
            auto reverse=b.create<LLVM::ICmpOp>(c.loc,LLVM::ICmpPredicate::sgt,current,end);
            b.create<LLVM::CondBrOp>(c.loc,b.create<LLVM::SelectOp>(c.loc,positive,forward,reverse),body,next);
            b.setInsertionPointToStart(body); breaks.push_back(next); continues.push_back(stepBlock); statements(s.body); continues.pop_back(); breaks.pop_back(); branch(stepBlock);
            b.setInsertionPointToStart(stepBlock); auto a=b.create<LLVM::LoadOp>(c.loc,b.getI32Type(),ptr); auto add=b.create<LLVM::LoadOp>(c.loc,b.getI32Type(),saved);
            b.create<LLVM::StoreOp>(c.loc,b.create<LLVM::AddOp>(c.loc,a,add),ptr); branch(cond); b.setInsertionPointToStart(next); break;
        }
        case S::Kind::Break:branch(breaks.back());break;
        case S::Kind::Continue:branch(continues.back());break;
        case S::Kind::Return:
            if(s.expressions.empty()) b.create<LLVM::ReturnOp>(c.loc,ValueRange{});
            else b.create<LLVM::ReturnOp>(c.loc,expr(s.expressions[0])); break;
        case S::Kind::Goto:
            if(s.expressions.empty()) branch(labels.back().at(s.name));
            else { auto *next=block(); b.create<LLVM::CondBrOp>(c.loc,boolValue(expr(s.expressions[0])),labels.back().at(s.name),next); b.setInsertionPointToStart(next); }
            break;
        case S::Kind::Label:branch(labels.back().at(s.name));b.setInsertionPointToStart(labels.back().at(s.name));break;
        case S::Kind::Model:case S::Kind::Contract:break; // resolved static declarations
        case S::Kind::Tensor: {
            const auto &t=p.tensors[s.declaration]; auto tensor=tensors.at(t.name); auto elem=tensor.getElementType(); SmallVector<Attribute> values;
            for(double v:t.values) values.push_back(isa<FloatType>(elem)?Attribute(b.getFloatAttr(elem,v)):Attribute(b.getIntegerAttr(elem,static_cast<int64_t>(v))));
            if(values.empty()) values.push_back(isa<FloatType>(elem)?Attribute(b.getFloatAttr(elem,0.0)):Attribute(b.getIntegerAttr(elem,0)));
            auto dense=DenseElementsAttr::get(tensor,values);
            auto value=op<ir::TensorOp>(TypeRange{tensor},{},{attr("name",b.getStringAttr(t.name)),attr("value",dense)}).getResult();
            auto converted=b.create<UnrealizedConversionCastOp>(c.loc,TypeRange{tensorStorage(tensor)},ValueRange{value}).getResult(0);
            b.create<LLVM::StoreOp>(c.loc,converted,b.create<LLVM::AddressOfOp>(c.loc,module->lookupSymbol<LLVM::GlobalOp>("__sh_tensor_"+t.name)));
            break;
        }
        case S::Kind::Infer: {
            const auto &i=p.inferences[s.declaration]; auto input=tensors.at(i.input_tensor_name),output=tensors.at(i.output_tensor_name);
            auto ptr=b.create<LLVM::AddressOfOp>(c.loc,module->lookupSymbol<LLVM::GlobalOp>("__sh_tensor_"+i.input_tensor_name));
            auto loaded=b.create<LLVM::LoadOp>(c.loc,tensorStorage(input),ptr);
            auto tensor=b.create<UnrealizedConversionCastOp>(c.loc,TypeRange{input},ValueRange{loaded}).getResult(0);
            auto inferred=op<ir::InferOp>(TypeRange{output},ValueRange{tensor},{attr("model",FlatSymbolRefAttr::get(b.getContext(),"__sh_model_"+i.model_name))}).getResult();
            auto array=b.create<UnrealizedConversionCastOp>(c.loc,TypeRange{tensorStorage(output)},ValueRange{inferred}).getResult(0);
            b.create<LLVM::StoreOp>(c.loc,array,b.create<LLVM::AddressOfOp>(c.loc,module->lookupSymbol<LLVM::GlobalOp>("__sh_tensor_"+i.output_tensor_name))); break;
        }
        case S::Kind::Measurement: {
            const auto &m=p.measurements[s.declaration]; StringRef policy=m.backend;
            auto found=models.find(m.backend); if(found!=models.end()) policy=found->second.getBackend().getName();
            auto bk=ir::BackendAttr::getChecked([&]{return emitError(c.loc);},b.getContext(),policy);
            if(!bk) { invalid=true; break; }
            op<ir::GreenAIMeasureOp>({}, {},{attr("workload",FlatSymbolRefAttr::get(b.getContext(),"__sh_contract_"+m.workload)),
                attr("backend",bk),attr("inferences",b.getI64IntegerAttr(m.inferences)),attr("watts",b.getF64FloatAttr(m.watts)),attr("seconds",b.getF64FloatAttr(m.seconds))}); break;
        }
        case S::Kind::LegacyInfer: {
            SmallVector<Value> args; for(const auto &str:s.strings) args.push_back(c.string(str));
            c.call("printf",b.getI32Type(),{c.string("AI inference request: model=%s shape=%s input=%s\n"),args[0],args[1],args[2]},true);
            auto status=c.call("short_ai_infer_legacy",b.getI32Type(),args).getResult();
            failureIf(boolValue(status),"SHD7030","AI runtime did not execute inference");break;
        }
        case S::Kind::GreenReport: {
            auto inferences=expr(s.expressions[0]),watts=expr(s.expressions[1]),seconds=expr(s.expressions[2]);
            auto energy=b.create<LLVM::MulOp>(c.loc,watts,seconds); auto z=b.create<LLVM::ICmpOp>(c.loc,LLVM::ICmpPredicate::eq,energy,c.integer(0));
            auto safe=b.create<LLVM::SelectOp>(c.loc,z,c.integer(1),energy); auto overflow=b.create<LLVM::AndOp>(c.loc,b.create<LLVM::ICmpOp>(c.loc,LLVM::ICmpPredicate::eq,inferences,c.integer(INT32_MIN)),b.create<LLVM::ICmpOp>(c.loc,LLVM::ICmpPredicate::eq,safe,c.integer(-1)));
            failureIf(overflow,"SHD7001","GreenAI integer division overflow");
            auto efficiency=b.create<LLVM::SDivOp>(c.loc,inferences,safe);
            auto result=b.create<LLVM::SelectOp>(c.loc,z,c.integer(0),efficiency);
            c.call("printf",b.getI32Type(),{c.string("GreenAI workload %s: inferences=%d energy_j=%d inf_per_j=%d\n"),c.string(s.name),inferences,energy,result},true);break;
        }
        case S::Kind::Evidence: {
            std::string text="shorthand.c3eco|"+s.name; for(const auto &str:s.strings) text+="|"+str;
            auto g=c.stringGlobal(text,"__sh_evidence_"); g.setSectionAttr(b.getStringAttr("shorthand_ai_metadata"));break;
        }
        }
    }
    LogicalResult metadata() {
        b.setInsertionPointToEnd(module->getBody());
        for(const auto &m:p.models) {
            c.loc=location(m.source_range); auto in=tensorType(m.input_shape,m.precision),out=tensorType(m.output_shape,m.precision);
            if(!in||!out) return failure();
            auto signature=ir::ModelType::getChecked([&]{return emitError(c.loc);},b.getContext(),in,out);
            auto policy=ir::BackendAttr::getChecked([&]{return emitError(c.loc);},b.getContext(),backend(m.backend_preference.front()));
            if(!signature||!policy) return failure();
            SmallVector<StringRef> preferences; for(auto kind:m.backend_preference) preferences.push_back(backend(kind));
            models[m.name]=op<ir::ModelOp>({}, {},{attr("sym_name",b.getStringAttr("__sh_model_"+m.name)),attr("signature",TypeAttr::get(signature)),
                attr("format",b.getStringAttr(format(m.format))),attr("path",b.getStringAttr(m.path)),attr("task",b.getStringAttr(m.task)),
                attr("quality_guardrail",b.getStringAttr(m.quality_guardrail)),attr("backend",policy),attr("shorthand.backend_preference",b.getStrArrayAttr(preferences))});
        }
        for(const auto &t:p.tensors) { auto ty=tensorType(t.shape,t.element_type); if(!ty) return failure(); tensors[t.name]=ty; global("__sh_tensor_"+t.name,tensorStorage(ty)); }
        for(const auto &d:p.contracts) {
            c.loc=location(d.source_range);
            auto evidence=ir::EvidenceAttr::getChecked([&]{return emitError(c.loc);},b.getContext(),StringRef(d.measurement_quality),StringRef(d.data_quality),StringRef(d.claims_mode));
            if(!evidence) return failure();
            SmallVector<StringRef> boundary; for(const auto &part:d.boundary) boundary.push_back(part);
            contracts[d.name]=op<ir::GreenAIContractOp>({}, {},{attr("sym_name",b.getStringAttr("__sh_contract_"+d.name)),
                attr("functional_unit",b.getStringAttr(d.functional_unit)),attr("success_criteria",b.getStringAttr(d.success_criteria)),
                attr("quality_guardrail",b.getStringAttr(d.quality_guardrail)),attr("boundary",b.getStrArrayAttr(boundary)),attr("evidence",evidence),
                attr("carbon_factor",b.getF64FloatAttr(d.carbon_factor)),attr("energy_budget_j",b.getF64FloatAttr(d.energy_budget_j)),attr("carbon_budget_gco2e",b.getF64FloatAttr(d.carbon_budget_gco2e))});
        }
        return verify(*module);
    }
    OwningOpRef<ModuleOp> run() {
        for(const auto &d:p.composites) {
            StringRef kind;
            switch(d.kind) {
            case sir::Composite::Kind::Record:kind="record";break;
            case sir::Composite::Kind::Enum:kind="enum";break;
            case sir::Composite::Kind::Option:kind="option";break;
            case sir::Composite::Kind::Result:kind="result";break;
            case sir::Composite::Kind::Slice:kind="slice";break;
            }
            SmallVector<Attribute> fields,names;
            for(T field:d.fields) fields.push_back(TypeAttr::get(type(field)));
            for(const auto &name:d.names) names.push_back(b.getStringAttr(name));
            composites[d.name]=ir::CompositeType::get(b.getContext(),kind,d.name,b.getArrayAttr(fields),b.getArrayAttr(names));
        }
        if(failed(metadata())) return {};
        for(const auto &v:p.globals) { globals[v.name]=v; global(v.name,storage(v)); }
        for(const auto &f:p.functions) { SmallVector<Type> args; for(const auto &v:f.parameters) args.push_back(type(v.type,v.composite)); c.function(f.name,type(f.result,f.result_composite),args); }
        auto emitFunction=[&](LLVM::LLVMFuncOp fn,const std::vector<sir::Variable> &params,const S &body,bool main) {
            function=fn; auto *entry=fn.addEntryBlock(); b.setInsertionPointToStart(entry); scopes.push_back({});
            for(std::size_t i=0;i<params.size();++i) { declare(params[i]); b.create<LLVM::StoreOp>(c.loc,entry->getArgument(i),scopes.back().at(params[i].name).pointer); }
            stmt(body);
            if(!terminated()) {
                if(main) b.create<LLVM::ReturnOp>(c.loc,c.integer(0));
                else if(isa<LLVM::LLVMVoidType>(fn.getFunctionType().getReturnType())) b.create<LLVM::ReturnOp>(c.loc,ValueRange{});
                else c.failureBody("SHD7004","non-void function completed without returning a value");
            }
            scopes.pop_back();
        };
        for(const auto &f:p.functions) emitFunction(module->lookupSymbol<LLVM::LLVMFuncOp>(f.name),f.parameters,f.body,false);
        emitFunction(c.function("main",b.getI32Type(),{}),{},p.body,true);
        if(invalid||failed(verify(*module))) return {};
        return std::move(module);
    }
};
}
OwningOpRef<ModuleOp> lowerSemanticIR(MLIRContext &context,const semantic_ir::ProgramIR &program) {
    std::string error;
    if(!verifySemanticIR(program,error)) { emitError(UnknownLoc::get(&context))<<error; return {}; }
    context.getOrLoadDialect<ir::ShortHandDialect>(); context.getOrLoadDialect<LLVM::LLVMDialect>();
    Generator generator(context,program); return generator.run();
}
bool emitProgram(const semantic_ir::ProgramIR &program,std::ostream &output,bool llvmIR,std::string &error,bool optimize) {
    MLIRContext context;
    context.disableMultithreading();
    ScopedDiagnosticHandler handler(&context,[&](Diagnostic &diagnostic) {
        llvm::raw_string_ostream stream(error); diagnostic.print(stream); stream<<'\n'; return success();
    });
    auto module=lowerSemanticIR(context,program); if(!module) return false;
    std::string text; llvm::raw_string_ostream stream(text);
    if(llvmIR) {
        if(failed(lowerToLLVM(*module,optimize))) return false;
        registerBuiltinDialectTranslation(context); registerLLVMDialectTranslation(context); llvm::LLVMContext llvmContext;
        auto translated=translateModuleToLLVMIR(*module,llvmContext,"shorthand.mlir.lowering.v1");
        if(!translated) { error+="LLVM IR translation failed"; return false; }
        translated->print(stream,nullptr);
    } else { module->print(stream,OpPrintingFlags().enableDebugInfo()); stream<<'\n'; }
    output<<text; return output.good();
}
}
