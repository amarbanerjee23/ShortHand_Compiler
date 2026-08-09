#include "IR_Generator.h"

bool IR_Generator::addLibraryProgram(AST_PROGRAM *program) {
    if (program == nullptr) return false;
    if (program->decl_block != nullptr) program->decl_block->accept(*this);
    if (program->functions != nullptr) program->functions->accept(*this);
    return true;
}
