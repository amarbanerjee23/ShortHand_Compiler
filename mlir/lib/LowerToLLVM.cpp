#include "ShortHand/Conversion/Lowering.h"
#include "ShortHand/IR/ShortHandOps.h"
#include "LoweringSupport.h"
#include "mlir/Conversion/LLVMCommon/TypeConverter.h"
#include "mlir/Conversion/FuncToLLVM/ConvertFuncToLLVM.h"
#include "mlir/Conversion/ArithToLLVM/ArithToLLVM.h"
#include "mlir/Conversion/ControlFlowToLLVM/ControlFlowToLLVM.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Transforms/DialectConversion.h"
#include "mlir/Transforms/Passes.h"
#include "mlir/IR/Verifier.h"
#include <iomanip>
#include <sstream>

using namespace mlir;
namespace shorthand::lowering {
namespace {
std::string number(double value) { std::ostringstream s; s<<std::setprecision(17)<<value; return s.str(); }
std::string shape(RankedTensorType t) {
    std::string result;
    for(int64_t dim:t.getShape()) { if(!result.empty()) result+=','; result+=std::to_string(dim); }
    return result;
}
void failIf(ConversionPatternRewriter &r,CodeBuilder &c,Value condition,StringRef reason,StringRef code="SHD7030") {
    Block *current=r.getInsertionBlock();
    Block *next=r.splitBlock(current,r.getInsertionPoint());
    Block *bad=r.createBlock(next);
    r.setInsertionPointToEnd(current); r.create<LLVM::CondBrOp>(c.loc,condition,bad,next);
    r.setInsertionPointToStart(bad); c.failureBody(code,reason);
    r.setInsertionPointToStart(next);
}
void checkStatus(ConversionPatternRewriter &r,CodeBuilder &c,Value status,StringRef reason) {
    failIf(r,c,r.create<LLVM::ICmpOp>(c.loc,LLVM::ICmpPredicate::ne,status,c.integer(0)),reason);
}
Value callStrings(CodeBuilder &c,StringRef name,ArrayRef<std::string> strings) {
    SmallVector<Value> args; for(const auto &s:strings) args.push_back(c.string(s));
    return c.call(name,c.b.getI32Type(),args).getResult();
}
struct TensorPattern:OpConversionPattern<ir::TensorOp> {
    using OpConversionPattern::OpConversionPattern;
    LogicalResult matchAndRewrite(ir::TensorOp op,OpAdaptor,ConversionPatternRewriter &r) const override {
        auto type=cast<RankedTensorType>(op.getResult().getType());
        auto dense=dyn_cast<DenseElementsAttr>(op.getValue());
        if(!dense) return op.emitOpError("LLVM lowering requires dense initialized tensor data");
        if(type.getNumElements()>65536) return op.emitOpError("LLVM lowering supports at most 65536 static tensor elements");
        auto flat=dense.reshape(RankedTensorType::get({type.getNumElements()},type.getElementType()));
        r.replaceOpWithNewOp<LLVM::ConstantOp>(op,getTypeConverter()->convertType(type),flat);
        return success();
    }
};
struct CastPattern:OpConversionPattern<UnrealizedConversionCastOp> {
    using OpConversionPattern::OpConversionPattern;
    LogicalResult matchAndRewrite(UnrealizedConversionCastOp op,OpAdaptor adapt,ConversionPatternRewriter &r) const override {
        if(op.getNumResults()!=1||adapt.getInputs().size()!=1||getTypeConverter()->convertType(op.getResult(0).getType())!=adapt.getInputs()[0].getType()) return failure();
        r.replaceOp(op,adapt.getInputs()); return success();
    }
};
struct InferPattern:OpConversionPattern<ir::InferOp> {
    using OpConversionPattern::OpConversionPattern;
    LogicalResult matchAndRewrite(ir::InferOp op,OpAdaptor adapt,ConversionPatternRewriter &r) const override {
        auto model=SymbolTable::lookupNearestSymbolFrom<ir::ModelOp>(op,op.getModelAttr());
        if(!model) return op.emitOpError("model symbol is unresolved");
        auto signature=cast<ir::ModelType>(model.getSignature());
        auto input=signature.getInput(),output=signature.getOutput();
        if(!input.getElementType().isF32()||!output.getElementType().isF32()) return op.emitOpError("runtime inference lowering requires float32 tensors");
        if(input.getNumElements()>65536||output.getNumElements()>65536) return op.emitOpError("runtime lowering supports at most 65536 elements per tensor");
        auto module=op->getParentOfType<ModuleOp>(); CodeBuilder c{r,module,op.getLoc()};
        std::string preference=model.getBackend().getName().str();
        if(auto prefs=model->getAttrOfType<ArrayAttr>("shorthand.backend_preference")) {
            preference.clear(); for(auto pref:prefs) { if(!preference.empty()) preference+=','; preference+=cast<StringAttr>(pref).str(); }
        }
        auto status=callStrings(c,"short_ai_register_model",{model.getSymName().str(),model.getFormat().str(),model.getPath().str(),model.getTask().str(),"float32",shape(input),shape(output),preference});
        checkStatus(r,c,status,"model registration failed");
        std::string inputName=model.getSymName().str()+"_input_"+shape(input),outputName=model.getSymName().str()+"_output_"+shape(output);
        status=callStrings(c,"short_ai_register_tensor",{inputName,"float32",shape(input),std::to_string(input.getRank()),std::to_string(input.getNumElements())});
        checkStatus(r,c,status,"input registration failed");
        status=callStrings(c,"short_ai_register_tensor",{outputName,"float32",shape(output),std::to_string(output.getRank()),std::to_string(output.getNumElements())});
        checkStatus(r,c,status,"output registration failed");
        auto inputType=getTypeConverter()->convertType(input),outputType=getTypeConverter()->convertType(output);
        Value inputBuffer=c.slot(inputType),outputBuffer=c.slot(outputType),count=c.slot(r.getI32Type());
        r.create<LLVM::StoreOp>(c.loc,adapt.getInput(),inputBuffer);
        r.create<LLVM::StoreOp>(c.loc,r.create<LLVM::ZeroOp>(c.loc,outputType),outputBuffer);
        r.create<LLVM::StoreOp>(c.loc,c.integer(0),count);
        status=c.call("short_ai_infer_f32",r.getI32Type(),{c.string(model.getSymName()),c.string(inputName),inputBuffer,c.integer(input.getNumElements()),
            c.string(outputName),outputBuffer,c.integer(output.getNumElements()),count}).getResult();
        checkStatus(r,c,status,"AI runtime did not execute inference");
        auto actual=r.create<LLVM::LoadOp>(c.loc,r.getI32Type(),count);
        failIf(r,c,r.create<LLVM::ICmpOp>(c.loc,LLVM::ICmpPredicate::ne,actual,c.integer(output.getNumElements())),"AI runtime returned an unexpected tensor size");
        r.replaceOpWithNewOp<LLVM::LoadOp>(op,outputType,outputBuffer);
        return success();
    }
};
struct MeasurePattern:OpConversionPattern<ir::GreenAIMeasureOp> {
    using OpConversionPattern::OpConversionPattern;
    LogicalResult matchAndRewrite(ir::GreenAIMeasureOp op,OpAdaptor,ConversionPatternRewriter &r) const override {
        auto contract=SymbolTable::lookupNearestSymbolFrom<ir::GreenAIContractOp>(op,op.getWorkloadAttr());
        if(!contract) return op.emitOpError("contract symbol is unresolved");
        CodeBuilder c{r,op->getParentOfType<ModuleOp>(),op.getLoc()};
        std::string boundary; for(auto part:contract.getBoundary()) { if(!boundary.empty()) boundary+=','; boundary+=cast<StringAttr>(part).str(); }
        auto evidence=contract.getEvidence();
        auto status=callStrings(c,"short_greenai_register_contract",{contract.getSymName().str(),contract.getFunctionalUnit().str(),contract.getSuccessCriteria().str(),boundary,
            evidence.getMeasurementQuality().str(),evidence.getDataQuality().str(),number(contract.getCarbonFactor().convertToDouble()),evidence.getClaimsMode().str()});
        checkStatus(r,c,status,"contract registration failed");
        status=callStrings(c,"short_greenai_record_measurement",{contract.getSymName().str(),op.getBackend().getName().str(),std::to_string(op.getInferencesAttr().getInt()),
            number(op.getWatts().convertToDouble()),number(op.getSeconds().convertToDouble())});
        checkStatus(r,c,status,"declared measurement recording failed");
        r.eraseOp(op); return success();
    }
};
SmallVector<Type> compositeFields(ir::CompositeType type) {
    auto *ctx=type.getContext(); SmallVector<Type> fields;
    StringRef kind=type.getKind();
    if(kind=="enum"||kind=="option"||kind=="result") fields.push_back(IntegerType::get(ctx,32));
    if(kind=="slice") return {LLVM::LLVMPointerType::get(ctx),IntegerType::get(ctx,64)};
    for(auto field:type.getFields()) fields.push_back(cast<TypeAttr>(field).getValue());
    return fields;
}
void checkComposite(ConversionPatternRewriter &r,CodeBuilder &c,ir::CompositeType type,Value tag,Value pointer={}) {
    StringRef kind=type.getKind();
    if(kind=="enum"||kind=="option"||kind=="result") {
        auto low=r.create<LLVM::ICmpOp>(c.loc,LLVM::ICmpPredicate::slt,tag,c.integer(0));
        auto high=r.create<LLVM::ICmpOp>(c.loc,LLVM::ICmpPredicate::sge,tag,c.integer(kind=="enum"?type.getNames().size():2));
        failIf(r,c,r.create<LLVM::OrOp>(c.loc,low,high),"invalid composite tag","SHD7031");
    } else if(kind=="slice") {
        auto negative=r.create<LLVM::ICmpOp>(c.loc,LLVM::ICmpPredicate::slt,tag,c.integer(0,64));
        auto nonempty=r.create<LLVM::ICmpOp>(c.loc,LLVM::ICmpPredicate::sgt,tag,c.integer(0,64));
        auto null=r.create<LLVM::ICmpOp>(c.loc,LLVM::ICmpPredicate::eq,pointer,r.create<LLVM::ZeroOp>(c.loc,c.ptr()));
        failIf(r,c,r.create<LLVM::OrOp>(c.loc,negative,r.create<LLVM::AndOp>(c.loc,nonempty,null)),"invalid borrowed slice descriptor","SHD7002");
    }
}
struct ValuePattern:OpConversionPattern<ir::ValueOp> {
    using OpConversionPattern::OpConversionPattern;
    LogicalResult matchAndRewrite(ir::ValueOp op,OpAdaptor adapt,ConversionPatternRewriter &r) const override {
        auto type=op.getResult().getType(); CodeBuilder c{r,op->getParentOfType<ModuleOp>(),op.getLoc()};
        StringRef kind=type.getKind();
        if(kind=="enum"||kind=="option"||kind=="result") checkComposite(r,c,type,adapt.getFields()[0]);
        else if(kind=="slice") checkComposite(r,c,type,adapt.getFields()[1],adapt.getFields()[0]);
        Value value=r.create<LLVM::ZeroOp>(c.loc,getTypeConverter()->convertType(type));
        for(auto field:llvm::enumerate(adapt.getFields())) value=r.create<LLVM::InsertValueOp>(c.loc,value,field.value(),ArrayRef<int64_t>{static_cast<int64_t>(field.index())});
        r.replaceOp(op,value); return success();
    }
};
struct ProjectPattern:OpConversionPattern<ir::ProjectOp> {
    using OpConversionPattern::OpConversionPattern;
    LogicalResult matchAndRewrite(ir::ProjectOp op,OpAdaptor adapt,ConversionPatternRewriter &r) const override {
        auto type=op.getValue().getType(); CodeBuilder c{r,op->getParentOfType<ModuleOp>(),op.getLoc()};
        StringRef kind=type.getKind(); int64_t index=op.getIndexAttr().getInt();
        if(kind=="enum"||kind=="option"||kind=="result") {
            auto tag=r.create<LLVM::ExtractValueOp>(c.loc,adapt.getValue(),ArrayRef<int64_t>{0});
            checkComposite(r,c,type,tag);
            if(index>0) {
                int64_t expected=kind=="option"?1:index-1;
                failIf(r,c,r.create<LLVM::ICmpOp>(c.loc,LLVM::ICmpPredicate::ne,tag,c.integer(expected)),"inactive option/result payload","SHD7031");
            }
        } else if(kind=="slice") {
            auto ptr=r.create<LLVM::ExtractValueOp>(c.loc,adapt.getValue(),ArrayRef<int64_t>{0});
            auto length=r.create<LLVM::ExtractValueOp>(c.loc,adapt.getValue(),ArrayRef<int64_t>{1});
            checkComposite(r,c,type,length,ptr);
        }
        r.replaceOpWithNewOp<LLVM::ExtractValueOp>(op,adapt.getValue(),ArrayRef<int64_t>{index}); return success();
    }
};
struct UpdatePattern:OpConversionPattern<ir::UpdateOp> {
    using OpConversionPattern::OpConversionPattern;
    LogicalResult matchAndRewrite(ir::UpdateOp op,OpAdaptor adapt,ConversionPatternRewriter &r) const override {
        r.replaceOpWithNewOp<LLVM::InsertValueOp>(op,adapt.getRecord(),adapt.getValue(),ArrayRef<int64_t>{op.getIndexAttr().getInt()}); return success();
    }
};
struct LowerPass:PassWrapper<LowerPass,OperationPass<ModuleOp>> {
    MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(LowerPass)
    StringRef getArgument() const final { return "lower-shorthand-to-llvm"; }
    StringRef getDescription() const final { return "Lower verified ShortHand operations and checked runtime calls to LLVM"; }
    void getDependentDialects(DialectRegistry &registry) const override { registry.insert<LLVM::LLVMDialect>(); }
    void runOnOperation() override {
        auto module=getOperation(); auto *ctx=&getContext();
        if(failed(verify(module))) { signalPassFailure(); return; }
        bool invalidPlacement=false;
        module.walk([&](Operation *op) {
            if(isa<ir::TensorOp,ir::InferOp,ir::GreenAIMeasureOp,ir::ValueOp,ir::ProjectOp,ir::UpdateOp>(op) &&
               !op->getParentOfType<LLVM::LLVMFuncOp>() && !op->getParentOfType<func::FuncOp>()) {
                op->emitError("executable ShortHand operations require a function body for LLVM lowering");
                invalidPlacement=true;
            }
        });
        if(invalidPlacement) { signalPassFailure(); return; }
        // A module must not impersonate the runtime or change a C entry signature.
        const auto ptr=LLVM::LLVMPointerType::get(ctx); OpBuilder b(ctx);
        auto check=[&](StringRef name,Type ret,ArrayRef<Type> args,bool variadic=false)->LogicalResult {
            auto *symbol=module.lookupSymbol(name); if(!symbol) return success();
            auto fn=dyn_cast<LLVM::LLVMFuncOp>(symbol);
            if(!fn||!fn.isExternal()||fn.getFunctionType()!=LLVM::LLVMFunctionType::get(ret,args,variadic)) return module.emitError()<<"runtime ABI symbol conflict: "<<name;
            return success();
        };
        SmallVector<Type> modelArgs(8,ptr),tensorArgs(5,ptr),contractArgs(8,ptr),measureArgs(5,ptr);
        if(failed(check("short_ai_register_model",b.getI32Type(),modelArgs))||failed(check("short_ai_register_tensor",b.getI32Type(),tensorArgs))||
           failed(check("short_greenai_register_contract",b.getI32Type(),contractArgs))||failed(check("short_greenai_record_measurement",b.getI32Type(),measureArgs))||
           failed(check("short_ai_infer_f32",b.getI32Type(),{ptr,ptr,ptr,b.getI32Type(),ptr,ptr,b.getI32Type(),ptr}))||
           failed(check("write",b.getI64Type(),{b.getI32Type(),ptr,b.getI64Type()}))||failed(check("exit",LLVM::LLVMVoidType::get(ctx),{b.getI32Type()}))) { signalPassFailure(); return; }
        LLVMTypeConverter converter(ctx);
        converter.addConversion([&](RankedTensorType type)->Type {
            if(!type.hasStaticShape()||type.getRank()==0||type.getEncoding()||type.getNumElements()<=0||type.getNumElements()>65536) return {};
            return LLVM::LLVMArrayType::get(type.getElementType(),type.getNumElements());
        });
        converter.addConversion([&](ir::CompositeType type)->Type { return LLVM::LLVMStructType::getLiteral(ctx,compositeFields(type)); });
        RewritePatternSet patterns(ctx);
        patterns.add<TensorPattern,InferPattern,MeasurePattern,CastPattern,ValuePattern,ProjectPattern,UpdatePattern>(converter,ctx);
        populateFuncToLLVMConversionPatterns(converter,patterns);
        arith::populateArithToLLVMConversionPatterns(converter,patterns);
        cf::populateControlFlowToLLVMConversionPatterns(converter,patterns);
        ConversionTarget target(*ctx);
        target.addLegalDialect<LLVM::LLVMDialect>(); target.addLegalOp<ModuleOp,ir::ModelOp,ir::GreenAIContractOp>();
        target.addIllegalDialect<ir::ShortHandDialect>(); target.addIllegalOp<UnrealizedConversionCastOp>();
        if(failed(applyFullConversion(module,target,std::move(patterns)))) { signalPassFailure(); return; }
        SmallVector<Operation *> declarations;
        module.walk([&](Operation *op) { if(isa<ir::ModelOp,ir::GreenAIContractOp>(op)) declarations.push_back(op); });
        for(auto *op:declarations) {
            b.setInsertionPoint(op); CodeBuilder c{b,module,op->getLoc()}; std::string text; llvm::raw_string_ostream stream(text); op->print(stream);
            auto global=c.stringGlobal(text,"__sh_declaration_"); global.setSectionAttr(b.getStringAttr("shorthand_ai_metadata")); op->erase();
        }
        SmallVector<LLVM::GlobalOp> metadata;
        for(auto global:module.getOps<LLVM::GlobalOp>()) if(global.getSection().value_or("")=="shorthand_ai_metadata") metadata.push_back(global);
        if(!metadata.empty()) {
            if(module.lookupSymbol("llvm.used")) { module.emitError("reserved llvm.used conflicts with retained ShortHand metadata"); signalPassFailure(); return; }
            b.setInsertionPointToEnd(module.getBody()); auto loc=module.getLoc(); auto array=LLVM::LLVMArrayType::get(ptr,metadata.size());
            auto used=b.create<LLVM::GlobalOp>(loc,array,false,LLVM::Linkage::Appending,"llvm.used",Attribute()); used.setSectionAttr(b.getStringAttr("llvm.metadata"));
            auto *entry=new Block; used.getInitializerRegion().push_back(entry); b.setInsertionPointToStart(entry); Value value=b.create<LLVM::ZeroOp>(loc,array);
            for(auto item:llvm::enumerate(metadata)) { Value address=b.create<LLVM::AddressOfOp>(loc,item.value()); value=b.create<LLVM::InsertValueOp>(loc,value,address,ArrayRef<int64_t>{static_cast<int64_t>(item.index())}); }
            b.create<LLVM::ReturnOp>(loc,value);
        }
        if(failed(verify(module))) signalPassFailure();
    }
};
}
std::unique_ptr<Pass> createLowerToLLVMPass() { return std::make_unique<LowerPass>(); }
void registerLoweringPasses() { PassRegistration<LowerPass>(); }
LogicalResult lowerToLLVM(ModuleOp module,bool optimize) {
    PassManager manager(module.getContext()); manager.enableVerifier(true);
    manager.addPass(createLowerToLLVMPass());
    if(optimize) { manager.addPass(createCanonicalizerPass()); manager.addPass(createCSEPass()); }
    return manager.run(module);
}
}
