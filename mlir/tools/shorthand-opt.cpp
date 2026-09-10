#include "ShortHand/IR/ShortHandDialect.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Tools/mlir-opt/MlirOptMain.h"
#include "mlir/Transforms/Passes.h"

int main(int argc, char **argv) {
    mlir::DialectRegistry registry;
    registry.insert<shorthand::ir::ShortHandDialect, mlir::func::FuncDialect>();
    mlir::registerTransformsPasses();
    return mlir::asMainReturnCode(
        mlir::MlirOptMain(argc, argv, "ShortHand MLIR v1 dialect driver\n", registry));
}
