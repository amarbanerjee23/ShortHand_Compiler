#include "ShortHand/IR/ShortHandDialect.h"
#include "ShortHand/Conversion/Lowering.h"
#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlowOps.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Tools/mlir-opt/MlirOptMain.h"
#include "mlir/Transforms/Passes.h"

int main(int argc, char **argv) {
    mlir::DialectRegistry registry;
    registry.insert<shorthand::ir::ShortHandDialect, mlir::func::FuncDialect,
                    mlir::LLVM::LLVMDialect, mlir::arith::ArithDialect, mlir::cf::ControlFlowDialect>();
    mlir::registerTransformsPasses();
    shorthand::lowering::registerLoweringPasses();
    return mlir::asMainReturnCode(
        mlir::MlirOptMain(argc, argv, "ShortHand MLIR v1 dialect driver\n", registry));
}
