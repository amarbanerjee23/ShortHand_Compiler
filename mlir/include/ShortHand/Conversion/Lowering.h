#pragma once
#include "ShortHand/SemanticIR.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/Pass/Pass.h"
#include <iosfwd>
#include <memory>
#include <string>

namespace shorthand::lowering {
// Returns no partially lowered module on invalid semantic input.
bool verifySemanticIR(const semantic_ir::ProgramIR &program, std::string &error);
mlir::OwningOpRef<mlir::ModuleOp> lowerSemanticIR(mlir::MLIRContext &context,
                                               const semantic_ir::ProgramIR &program);
std::unique_ptr<mlir::Pass> createLowerToLLVMPass();
void registerLoweringPasses();
mlir::LogicalResult lowerToLLVM(mlir::ModuleOp module, bool optimize = true);
bool emitProgram(const semantic_ir::ProgramIR &program, std::ostream &output,
                 bool llvmIR, std::string &error, bool optimize = true);
}
