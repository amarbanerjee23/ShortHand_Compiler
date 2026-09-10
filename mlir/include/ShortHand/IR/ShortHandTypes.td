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

#endif
