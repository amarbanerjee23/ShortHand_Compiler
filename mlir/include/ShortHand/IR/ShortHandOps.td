#ifndef SHORTHAND_OPS_TD
#define SHORTHAND_OPS_TD

include "ShortHand/IR/ShortHandTypes.td"
include "ShortHand/IR/ShortHandAttributes.td"
include "mlir/IR/SymbolInterfaces.td"
include "mlir/Interfaces/SideEffectInterfaces.td"

def ShortHand_ModelOp : ShortHand_Op<"model", [Symbol]> {
  let summary = "Declare a model with a checked input/output signature";
  let arguments = (ins SymbolNameAttr:$sym_name, TypeAttrOf<ShortHand_ModelType>:$signature,
    StrAttr:$format, StrAttr:$path, StrAttr:$task, StrAttr:$quality_guardrail,
    ShortHand_BackendAttr:$backend);
  let assemblyFormat = "$sym_name attr-dict";
  let hasVerifier = 1;
}

def ShortHand_TensorOp : ShortHand_Op<"tensor", [Pure]> {
  let summary = "Materialize a statically shaped tensor from defined dense data";
  let arguments = (ins StrAttr:$name, ElementsAttr:$value);
  let results = (outs AnyRankedTensor:$result);
  let assemblyFormat = "attr-dict `:` type($result)";
  let hasVerifier = 1;
}

def ShortHand_InferOp : ShortHand_Op<"infer", [DeclareOpInterfaceMethods<SymbolUserOpInterface>]> {
  let summary = "Infer through a model symbol with typed SSA input and output";
  let description = [{
    The input and output must exactly match the model signature. This operation
    conservatively retains side effects: inference can access external model
    data, fail, and emit telemetry. Backend selection is a policy declaration,
    not a statement that a device or SDK has been qualified.
  }];
  let arguments = (ins FlatSymbolRefAttr:$model, AnyRankedTensor:$input);
  let results = (outs AnyRankedTensor:$result);
  let assemblyFormat = "$model `(` $input `)` attr-dict `:` functional-type($input, $result)";
  let hasVerifier = 1;
}

def ShortHand_GreenAIContractOp : ShortHand_Op<"greenai_contract", [Symbol]> {
  let summary = "Declare a candidate evidence contract and explicit budgets";
  let arguments = (ins SymbolNameAttr:$sym_name, StrAttr:$functional_unit,
    StrAttr:$success_criteria, StrAttr:$quality_guardrail, StrArrayAttr:$boundary,
    ShortHand_EvidenceAttr:$evidence, F64Attr:$carbon_factor,
    F64Attr:$energy_budget_j, F64Attr:$carbon_budget_gco2e);
  let assemblyFormat = "$sym_name attr-dict";
  let hasVerifier = 1;
}

def ShortHand_GreenAIMeasureOp : ShortHand_Op<"greenai_measure", [DeclareOpInterfaceMethods<SymbolUserOpInterface>]> {
  let summary = "Record declared activity under a resolved candidate evidence contract";
  let description = [{
    Units are inferences, watts and seconds. Values are declarations, not proof
    of an instrument reading. The measurement remains observable under generic
    dead-code elimination and CSE. Signing, assessment and certification remain
    separate evidence workflows.
  }];
  let arguments = (ins FlatSymbolRefAttr:$workload, ShortHand_BackendAttr:$backend,
    I64Attr:$inferences, F64Attr:$watts, F64Attr:$seconds);
  let assemblyFormat = "$workload attr-dict";
  let hasVerifier = 1;
}

#endif
