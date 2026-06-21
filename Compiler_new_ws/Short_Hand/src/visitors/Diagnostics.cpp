#include "Diagnostics.h"
#include <iostream>
void Diagnostics::error(const std::string &m){ errors_.push_back(m); }
void Diagnostics::warning(const std::string &m){ warnings_.push_back(m); }
bool Diagnostics::hasErrors() const { return !errors_.empty(); }
void Diagnostics::print() const { for(const auto &w:warnings_) std::cerr << "warning: " << w << "\n"; for(const auto &e:errors_) std::cerr << "error: " << e << "\n"; }
