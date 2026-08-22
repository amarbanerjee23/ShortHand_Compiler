#include "IR_Generator.h"

bool IR_Generator::addLibraryProgram(AST_PROGRAM *program) {
    if (program == nullptr) return false;
    emitting_global_declarations = true;
    if (program->decl_block != nullptr) program->decl_block->accept(*this);
    emitting_global_declarations = false;
    if (program->functions != nullptr) program->functions->accept(*this);
    return true;
}
