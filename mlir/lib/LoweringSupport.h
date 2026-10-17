#pragma once
#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/SymbolTable.h"
#include "llvm/Config/llvm-config.h"
#include "llvm/TargetParser/Host.h"
#include "llvm/TargetParser/Triple.h"

namespace shorthand::lowering {
// Use the module target for both the C runtime ABI and object-file metadata.
inline llvm::Triple targetTriple(mlir::ModuleOp module) {
    auto attr=module->getAttrOfType<mlir::StringAttr>("llvm.target_triple");
    return llvm::Triple(attr?attr.str():llvm::sys::getDefaultTargetTriple());
}
inline llvm::StringRef metadataSection(mlir::ModuleOp module) {
    const auto triple=targetTriple(module);
    return triple.isOSDarwin()?"__DATA,__sh_ai_meta":triple.isOSWindows()?".sh_ai":"shorthand_ai_metadata";
}
struct CodeBuilder {
    mlir::OpBuilder &b;
    mlir::ModuleOp module;
    mlir::Location loc;
    mlir::Type ptr() const { return mlir::LLVM::LLVMPointerType::get(b.getContext()); }
    mlir::Type voidTy() const { return mlir::LLVM::LLVMVoidType::get(b.getContext()); }
    mlir::Value integer(int64_t n, unsigned bits=32) {
        return b.create<mlir::LLVM::ConstantOp>(loc,b.getIntegerType(bits),n);
    }
    mlir::Value floating(double n) { return b.create<mlir::LLVM::ConstantOp>(loc,b.getF64Type(),b.getF64FloatAttr(n)); }
    mlir::LLVM::LLVMFuncOp function(llvm::StringRef name, mlir::Type result,
                                  mlir::TypeRange args, bool varargs=false) {
        llvm::SmallVector<mlir::Type> argumentTypes(args.begin(), args.end());
        auto type=mlir::LLVM::LLVMFunctionType::get(result, argumentTypes, varargs);
        auto fn=module.lookupSymbol<mlir::LLVM::LLVMFuncOp>(name);
        if(fn) return fn;
        mlir::OpBuilder::InsertionGuard guard(b);
        b.setInsertionPointToStart(module.getBody());
        return b.create<mlir::LLVM::LLVMFuncOp>(loc,name,type);
    }
    mlir::LLVM::CallOp call(llvm::StringRef name, mlir::Type result,
                           mlir::ValueRange args, bool varargs=false) {
        llvm::SmallVector<mlir::Type> types;
        for(auto value:args) types.push_back(value.getType());
        if(varargs) types.resize(1);
        return b.create<mlir::LLVM::CallOp>(loc,function(name,result,types,varargs),args);
    }
    mlir::LLVM::GlobalOp stringGlobal(llvm::StringRef value, llvm::StringRef prefix="__sh_string_") {
        mlir::OpBuilder::InsertionGuard guard(b);
        unsigned index=0; std::string name;
        do { name=(prefix+llvm::Twine(index++)).str(); } while(module.lookupSymbol(name));
        b.setInsertionPointToStart(module.getBody());
        std::string terminated=value.str(); terminated.push_back('\0');
        return b.create<mlir::LLVM::GlobalOp>(loc,
            mlir::LLVM::LLVMArrayType::get(b.getI8Type(),terminated.size()),true,
            mlir::LLVM::Linkage::Private,name,b.getStringAttr(terminated));
    }
    mlir::Value string(llvm::StringRef value) {
        return b.create<mlir::LLVM::AddressOfOp>(loc,stringGlobal(value));
    }
    mlir::Value slot(mlir::Type type) {
        mlir::OpBuilder::InsertionGuard guard(b);
        mlir::Block *block=b.getInsertionBlock();
        b.setInsertionPointToStart(&block->getParent()->front());
        return b.create<mlir::LLVM::AllocaOp>(loc,ptr(),type,integer(1,64));
    }
    mlir::Value gep(mlir::Type element, mlir::Value base,
                    llvm::ArrayRef<mlir::LLVM::GEPArg> indices) {
#if LLVM_VERSION_MAJOR >= 22
        return b.create<mlir::LLVM::GEPOp>(loc,ptr(),element,base,indices,
                                         mlir::LLVM::GEPNoWrapFlags::inbounds);
#else
        return b.create<mlir::LLVM::GEPOp>(loc,ptr(),element,base,indices,true);
#endif
    }
    mlir::Block *entryBlock(mlir::LLVM::LLVMFuncOp fn) {
#if LLVM_VERSION_MAJOR >= 22
        return fn.addEntryBlock(b);
#else
        return fn.addEntryBlock();
#endif
    }
    void failureBody(llvm::StringRef code, llvm::StringRef message) {
        std::string text;
        if(auto source=mlir::dyn_cast<mlir::FileLineColLoc>(loc)) text=source.getFilename().str()+":"+std::to_string(source.getLine())+":"+std::to_string(source.getColumn())+": ";
        text+=("error: ["+code+"] "+message+"\n").str();
        const bool windows=targetTriple(module).isOSWindows();
        call(windows?"_write":"write",b.getIntegerType(windows?32:64),
             {integer(2),string(text),integer(text.size(),windows?32:64)});
        call("exit",voidTy(),{integer(1)});
        b.create<mlir::LLVM::UnreachableOp>(loc);
    }
};
}
