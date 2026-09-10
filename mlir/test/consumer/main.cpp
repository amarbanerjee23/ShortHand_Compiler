#include "ShortHand/IR/ShortHandOps.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Parser/Parser.h"
#include "llvm/Support/raw_ostream.h"

using namespace mlir;
using namespace shorthand::ir;

int main(int argc, char **argv) {
    if (argc != 2)
        return 1;
    MLIRContext context;
    context.getOrLoadDialect<ShortHandDialect>();
    auto module = parseSourceFile<ModuleOp>(argv[1], &context);
    if (!module || failed(verify(*module)))
        return 2;
    OpBuilder builder(&context);
    auto loc = builder.getUnknownLoc();
    unsigned errors = 0;
    ScopedDiagnosticHandler diagnostics(&context, [&](Diagnostic &diagnostic) {
        if (diagnostic.getSeverity() == DiagnosticSeverity::Error)
            ++errors;
        return success();
    });
    auto error = [&]() { return emitError(loc); };
    auto tensor = RankedTensorType::get({1, 4}, builder.getF32Type());
    auto output = RankedTensorType::get({1, 2}, builder.getF32Type());
    auto dynamic = RankedTensorType::get({ShapedType::kDynamic}, builder.getF32Type());
    if (ModelType::getChecked(error, &context, dynamic, output) ||
        BackendAttr::getChecked(error, &context, StringRef("invalid")) ||
        EvidenceAttr::getChecked(error, &context, StringRef("MQ4"), StringRef("DQ4"),
                                 StringRef("certified")) ||
        errors != 3)
        return 3;
    auto signature = ModelType::getChecked(error, &context, tensor, output);
    auto backend = BackendAttr::getChecked(error, &context, StringRef("onnxruntime_cpu"));
    if (!signature || !backend || signature.getInput() != tensor)
        return 4;

    // Exercise generated builders, SSA use chains, and symbol verification.
    builder.setInsertionPointToEnd(module->getBody());
    auto model = builder.create<ModelOp>(loc, "built", signature, "onnx", "built.onnx",
                                         "classification", "accuracy >= 0.95", backend);
    auto data = DenseElementsAttr::get(tensor, llvm::ArrayRef<float>{1.0f});
    auto input = builder.create<TensorOp>(loc, tensor, "built_input", data);
    auto infer = builder.create<InferOp>(loc, output, "built", input.getResult());
    if (failed(verify(*module)) || infer.getInput() != input.getResult())
        return 5;
    infer.getResult().setType(tensor);
    if (succeeded(verify(*module)))
        return 6;
    infer.getResult().setType(output);
    model.setBackendAttr(BackendAttr::get(&context, "invalid"));
    if (succeeded(verify(*module)))
        return 7;
    model.setBackendAttr(backend);
    model.setSignatureAttr(TypeAttr::get(ModelType::get(&context, dynamic, output)));
    if (succeeded(verify(*module)))
        return 8;
    model.setSignatureAttr(TypeAttr::get(signature));
    auto contract = *module->getOps<GreenAIContractOp>().begin();
    auto evidence = contract.getEvidence();
    contract.setEvidenceAttr(EvidenceAttr::get(&context, "MQ4", "DQ4", "certified"));
    if (succeeded(verify(*module)))
        return 9;
    contract.setEvidenceAttr(evidence);
    if (failed(verify(*module)) || errors != 7)
        return 10;
    llvm::outs()
        << "PASS installed MLIR API builders, checked construction and mutation verification\n";
    return 0;
}
