//===- ShortHandOps.td - ShortHand MLIR operation scaffold -*- tablegen -*-===//
//
// Initial operation contract for the ShortHand MLIR dialect.
// These ops mirror the current beta AI/GreenAI language constructs and the
// existing SemanticIR scaffold.
//
//===----------------------------------------------------------------------===//

#ifndef SHORTHAND_OPS_TD
#define SHORTHAND_OPS_TD

include "ShortHand/IR/ShortHandDialect.td"
include "mlir/IR/AttrTypeBase.td"
include "mlir/IR/BuiltinAttributes.td"
include "mlir/Interfaces/InferTypeOpInterface.td"
include "mlir/Interfaces/SideEffectInterfaces.td"

def ShortHand_ModelOp : ShortHand_Op<"model", [NoMemoryEffect]> {
  let summary = "Declare an AI model available to ShortHand lowering";
  let description = [{
    Represents a model declaration after semantic validation. Attributes carry
    the model identity, file format, path, task, precision, expected input/output
    shapes, and backend preference.
  }];
  let arguments = (ins
    StrAttr:$name,
    StrAttr:$format,
    StrAttr:$path,
    StrAttr:$task,
    StrAttr:$precision,
    StrAttr:$input_shape,
    StrAttr:$output_shape,
    OptionalAttr<StrAttr>:$backend_preference
  );
  let assemblyFormat = "attr-dict";
}

def ShortHand_TensorOp : ShortHand_Op<"tensor", [NoMemoryEffect]> {
  let summary = "Declare a named tensor and its static shape metadata";
  let description = [{
    Represents a tensor declaration after semantic validation. The shape remains
    string-backed in the scaffold so it can map directly to the current AST and
    SemanticIR fields before shaped MLIR types are introduced.
  }];
  let arguments = (ins
    StrAttr:$name,
    StrAttr:$element_type,
    StrAttr:$shape,
    I64Attr:$rank,
    I64Attr:$total_elements
  );
  let assemblyFormat = "attr-dict";
}

def ShortHand_InferOp : ShortHand_Op<"infer"> {
  let summary = "Represent a model inference request";
  let description = [{
    Connects a model declaration to input and output tensors. Later lowering
    will map this op to runtime hooks or backend-specific execution calls.
  }];
  let arguments = (ins
    StrAttr:$model,
    StrAttr:$input,
    StrAttr:$output
  );
  let assemblyFormat = "attr-dict";
}

def ShortHand_GreenAIContractOp : ShortHand_Op<"greenai_contract", [NoMemoryEffect]> {
  let summary = "Declare a GreenAI/C3-ECO evidence contract";
  let description = [{
    Represents the evidence boundary, functional unit, success criteria,
    measurement/data quality, carbon factor, and claim mode for a workload.
  }];
  let arguments = (ins
    StrAttr:$name,
    StrAttr:$functional_unit,
    StrAttr:$success_criteria,
    StrAttr:$boundary,
    StrAttr:$measurement_quality,
    StrAttr:$data_quality,
    F64Attr:$carbon_factor,
    StrAttr:$claims_mode
  );
  let assemblyFormat = "attr-dict";
}

def ShortHand_GreenAIMeasureOp : ShortHand_Op<"greenai_measure", [NoMemoryEffect]> {
  let summary = "Record GreenAI measurement metadata";
  let description = [{
    Represents declared or measured workload activity used by C3-ECO candidate
    evidence generation. Later lowering can attach measured telemetry and energy
    sources to this op.
  }];
  let arguments = (ins
    StrAttr:$workload,
    StrAttr:$backend,
    I64Attr:$inferences,
    F64Attr:$watts,
    F64Attr:$seconds
  );
  let assemblyFormat = "attr-dict";
}

#endif // SHORTHAND_OPS_TD
