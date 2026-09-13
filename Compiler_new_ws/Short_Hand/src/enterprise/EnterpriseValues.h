#pragma once
#include "../semantic_ir/SemanticIR.h"
#include <string>
namespace shorthand::enterprise {
bool isValueSource(const std::string &path);
bool lowerValueSource(const std::string &path, semantic_ir::ProgramIR &program, std::string &error, unsigned &line);
}
