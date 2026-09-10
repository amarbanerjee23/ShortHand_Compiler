#include "ShortHand/IR/ShortHandOps.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/DialectImplementation.h"
#include "mlir/IR/OpImplementation.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/ADT/TypeSwitch.h"
#include <cmath>
#include <limits>

#include "ShortHand/IR/ShortHandDialect.cpp.inc"
#define GET_TYPEDEF_CLASSES
#include "ShortHand/IR/ShortHandTypes.cpp.inc"
#define GET_ATTRDEF_CLASSES
#include "ShortHand/IR/ShortHandAttributes.cpp.inc"
#define GET_OP_CLASSES
#include "ShortHand/IR/ShortHandOps.cpp.inc"

using namespace ::mlir;
namespace shorthand::ir {
void ShortHandDialect::initialize() {
    addTypes<
#define GET_TYPEDEF_LIST
#include "ShortHand/IR/ShortHandTypes.cpp.inc"
        >();
    addAttributes<
#define GET_ATTRDEF_LIST
#include "ShortHand/IR/ShortHandAttributes.cpp.inc"
        >();
    addOperations<
#define GET_OP_LIST
#include "ShortHand/IR/ShortHandOps.cpp.inc"
        >();
}

namespace {
LogicalResult verifyTensor(llvm::function_ref<InFlightDiagnostic()> error, RankedTensorType type) {
    if (!type || !type.hasStaticShape() || type.getRank() == 0 || type.getEncoding())
        return error() << "expected an unencoded, non-scalar static ranked tensor";
    Type element = type.getElementType();
    bool supported = element.isF32() || element.isF16() || element.isBF16() ||
                     element.isSignlessInteger(4) || element.isSignlessInteger(8) ||
                     element.isSignlessInteger(32);
    if (!supported)
        return error() << "unsupported tensor element type: " << element;
    int64_t count = 1;
    for (int64_t dim : type.getShape()) {
        if (dim <= 0)
            return error() << "tensor dimensions must be positive";
        if (count > std::numeric_limits<int64_t>::max() / dim)
            return error() << "tensor element count overflows int64";
        count *= dim;
    }
    // Bound byte storage without multiplying a potentially overflowing count.
    const int64_t bytes = (element.getIntOrFloatBitWidth() + 7) / 8;
    if (count > std::numeric_limits<int64_t>::max() / bytes)
        return error() << "tensor byte size overflows int64";
    return success();
}

bool knownBackend(StringRef name) {
    return llvm::StringSwitch<bool>(name)
        .Cases("fallback", "onnxruntime_cpu", "onnxruntime_cuda", true)
        .Cases("onnxruntime_tensorrt", "tensorrt", "openvino", true)
        .Cases("libtorch", "llamacpp", true)
        .Default(false);
}

bool finiteNonnegative(double value) { return std::isfinite(value) && value >= 0.0; }
} // namespace

LogicalResult ModelType::verify(llvm::function_ref<InFlightDiagnostic()> error,
                                RankedTensorType input, RankedTensorType output) {
    if (failed(verifyTensor(error, input)))
        return failure();
    return verifyTensor(error, output);
}

LogicalResult BackendAttr::verify(llvm::function_ref<InFlightDiagnostic()> error, StringRef name) {
    if (!knownBackend(name))
        return error() << "unknown backend policy: " << name;
    return success();
}

LogicalResult EvidenceAttr::verify(llvm::function_ref<InFlightDiagnostic()> error, StringRef mq,
                                   StringRef dq, StringRef claims) {
    if (mq.size() != 3 || !mq.starts_with("MQ") || mq[2] < '0' || mq[2] > '4')
        return error() << "measurement quality must be MQ0 through MQ4";
    if (dq.size() != 3 || !dq.starts_with("DQ") || dq[2] < '0' || dq[2] > '4')
        return error() << "data quality must be DQ0 through DQ4";
    if (claims != "evidence_only")
        return error() << "claims mode must be evidence_only";
    return success();
}

LogicalResult ModelOp::verify() {
    auto error = [&]() { return emitOpError(); };
    auto signature = cast<ModelType>(getSignature());
    // Unchecked C++ builders must not bypass nested type/attribute validation.
    if (failed(ModelType::verify(error, signature.getInput(), signature.getOutput())) ||
        failed(BackendAttr::verify(error, getBackend().getName())))
        return failure();
    if (getSymName().trim().empty() || getPath().trim().empty() || getTask().trim().empty() ||
        getQualityGuardrail().trim().empty())
        return emitOpError("model name, path, task and quality_guardrail must be nonempty");
    StringRef format = getFormat();
    StringRef backend = getBackend().getName();
    bool compatible =
        llvm::StringSwitch<bool>(format)
            .Case("onnx", backend.starts_with("onnxruntime_") || backend == "openvino")
            .Case("tensorrt_engine", backend == "tensorrt")
            .Case("torchscript", backend == "libtorch")
            .Case("openvino_ir", backend == "openvino")
            .Case("gguf", backend == "llamacpp")
            .Default(false);
    bool knownFormat =
        llvm::StringSwitch<bool>(format)
            .Cases("onnx", "tensorrt_engine", "torchscript", "openvino_ir", "gguf", true)
            .Default(false);
    if (!knownFormat)
        return emitOpError("unknown model format");
    if (!compatible && backend != "fallback")
        return emitOpError("backend is incompatible with model format");
    return success();
}

LogicalResult TensorOp::verify() {
    if (failed(verifyTensor([&]() { return emitOpError(); }, getResult().getType())))
        return failure();
    if (getName().trim().empty())
        return emitOpError("tensor name must be nonempty");
    if (!isa<DenseElementsAttr>(getValue()) || getValue().getType() != getResult().getType())
        return emitOpError("dense tensor data must match the result type");
    return success();
}

LogicalResult InferOp::verify() {
    auto error = [&]() { return emitOpError(); };
    if (failed(verifyTensor(error, getInput().getType())))
        return failure();
    return verifyTensor(error, getResult().getType());
}

LogicalResult InferOp::verifySymbolUses(SymbolTableCollection &symbols) {
    auto model = symbols.lookupNearestSymbolFrom<ModelOp>(*this, getModelAttr());
    if (!model)
        return emitOpError("model must resolve to a shorthand.model symbol");
    auto attr = model->getAttrOfType<TypeAttr>("signature");
    auto signature = attr ? dyn_cast<ModelType>(attr.getValue()) : ModelType();
    if (!signature)
        return emitOpError("referenced model has no valid signature");
    if (getInput().getType() != signature.getInput() ||
        getResult().getType() != signature.getOutput())
        return emitOpError("input and output must exactly match the model signature");
    return success();
}

LogicalResult GreenAIContractOp::verify() {
    if (getSymName().trim().empty() || getFunctionalUnit().trim().empty() ||
        getSuccessCriteria().trim().empty() || getQualityGuardrail().trim().empty())
        return emitOpError("contract name, functional unit, success criteria and quality guardrail "
                           "must be nonempty");
    if (getBoundary().empty())
        return emitOpError("boundary must contain at least one component");
    llvm::SmallSet<StringRef, 8> seen;
    for (Attribute component : getBoundary()) {
        StringRef name = cast<StringAttr>(component).getValue();
        if (name.trim().empty() || name != name.trim() || !seen.insert(name).second)
            return emitOpError("boundary components must be nonempty, trimmed and unique");
    }
    auto evidence = getEvidence();
    if (failed(EvidenceAttr::verify([&]() { return emitOpError(); },
                                    evidence.getMeasurementQuality(), evidence.getDataQuality(),
                                    evidence.getClaimsMode())))
        return failure();
    if (!std::isfinite(getCarbonFactor().convertToDouble()) ||
        getCarbonFactor().convertToDouble() <= 0.0)
        return emitOpError("carbon factor must be finite and positive in gCO2e/kWh");
    if (!finiteNonnegative(getEnergyBudgetJ().convertToDouble()) ||
        !finiteNonnegative(getCarbonBudgetGco2e().convertToDouble()))
        return emitOpError("energy and carbon budgets must be finite and nonnegative");
    return success();
}

LogicalResult GreenAIMeasureOp::verify() {
    if (failed(BackendAttr::verify([&]() { return emitOpError(); }, getBackend().getName())))
        return failure();
    double watts = getWatts().convertToDouble();
    double seconds = getSeconds().convertToDouble();
    if (getInferencesAttr().getInt() <= 0 || !std::isfinite(watts) || watts <= 0.0 ||
        !std::isfinite(seconds) || seconds <= 0.0 || !std::isfinite(watts * seconds) ||
        watts * seconds == 0.0)
        return emitOpError(
            "inferences, watts, seconds and computed joules must be finite and positive");
    return success();
}

LogicalResult GreenAIMeasureOp::verifySymbolUses(SymbolTableCollection &symbols) {
    if (!symbols.lookupNearestSymbolFrom<GreenAIContractOp>(*this, getWorkloadAttr()))
        return emitOpError("workload must resolve to a shorthand.greenai_contract symbol");
    return success();
}
} // namespace shorthand::ir
