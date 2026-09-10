#ifndef SHORTHAND_MLIR_OPS_H
#define SHORTHAND_MLIR_OPS_H
#include "ShortHand/IR/ShortHandAttributes.h"
#include "ShortHand/IR/ShortHandDialect.h"
#include "ShortHand/IR/ShortHandTypes.h"
#include "mlir/Bytecode/BytecodeOpInterface.h"
#include "mlir/IR/BuiltinAttributes.h"
#include "mlir/IR/OpDefinition.h"
#include "mlir/IR/SymbolTable.h"
#include "mlir/Interfaces/SideEffectInterfaces.h"
#define GET_OP_CLASSES
#include "ShortHand/IR/ShortHandOps.h.inc"
#endif
