#ifndef SHORTHAND_TYPES_TD
#define SHORTHAND_TYPES_TD

include "ShortHand/IR/ShortHandDialect.td"
include "mlir/IR/AttrTypeBase.td"

def ShortHand_ModelType : TypeDef<ShortHand_Dialect, "Model"> {
  let mnemonic = "model";
  let summary = "Static input and output tensor signature for a model";
  let parameters = (ins "::mlir::RankedTensorType":$input,
                        "::mlir::RankedTensorType":$output);
  let assemblyFormat = "`<` $input `,` $output `>`";
  let genVerifyDecl = 1;
}

def ShortHand_CompositeType : TypeDef<ShortHand_Dialect, "Composite"> {
  let mnemonic = "value";
  let summary = "Named scalar-payload record, enum, option, result or borrowed slice";
  let parameters = (ins StringRefParameter<>:$kind, StringRefParameter<>:$name,
                        "::mlir::ArrayAttr":$fields, "::mlir::ArrayAttr":$names);
  let assemblyFormat = "`<` $kind `,` $name `,` $fields `,` $names `>`";
  let genVerifyDecl = 1;
}

#endif
