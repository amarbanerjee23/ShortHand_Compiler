//===- ShortHandDialect.td - ShortHand MLIR dialect scaffold -*- tablegen -*-===//
//
// This file defines the initial ShortHand MLIR dialect contract.
// It is intentionally scaffold-only until the compiler wires AST/SemanticIR lowering
// into an MLIR pass pipeline.
//
//===----------------------------------------------------------------------===//

#ifndef SHORTHAND_DIALECT_TD
#define SHORTHAND_DIALECT_TD

include "mlir/IR/OpBase.td"

def ShortHand_Dialect : Dialect {
  let name = "shorthand";
  let cppNamespace = "::shorthand::mlir";
  let summary = "ShortHand AI, tensor, infer, and GreenAI dialect";
  let description = [{
    The ShortHand dialect represents typed compiler operations for model
    declarations, tensor declarations, inference calls, GreenAI contracts, and
    GreenAI measurements before lowering to LLVM dialect and native code.
  }];
}

class ShortHand_Op<string mnemonic, list<Trait> traits = []>
    : Op<ShortHand_Dialect, mnemonic, traits>;

#endif // SHORTHAND_DIALECT_TD
