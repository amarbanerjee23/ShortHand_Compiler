#ifndef SHORTHAND_ATTRIBUTES_TD
#define SHORTHAND_ATTRIBUTES_TD

include "ShortHand/IR/ShortHandDialect.td"
include "mlir/IR/AttrTypeBase.td"

def ShortHand_BackendAttr : AttrDef<ShortHand_Dialect, "Backend"> {
  let mnemonic = "backend";
  let summary = "Validated backend policy name, without hardware qualification";
  let parameters = (ins StringRefParameter<>:$name);
  let assemblyFormat = "`<` $name `>`";
  let genVerifyDecl = 1;
}

def ShortHand_EvidenceAttr : AttrDef<ShortHand_Dialect, "Evidence"> {
  let mnemonic = "evidence";
  let summary = "Declared MQ/DQ classes with evidence-only claim control";
  let parameters = (ins StringRefParameter<>:$measurementQuality,
    StringRefParameter<>:$dataQuality, StringRefParameter<>:$claimsMode);
  let assemblyFormat = "`<` $measurementQuality `,` $dataQuality `,` $claimsMode `>`";
  let genVerifyDecl = 1;
}

#endif
