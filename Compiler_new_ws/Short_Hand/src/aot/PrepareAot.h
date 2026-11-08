#pragma once
#include "ShortHand/SemanticIR.h"
#include <string>
namespace shorthand::aot {
bool prepare(semantic_ir::ProgramIR &program,bool hybrid,std::string &error);
}
