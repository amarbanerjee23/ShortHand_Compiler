#ifndef SHORTHAND_DIAGNOSTICS_H
#define SHORTHAND_DIAGNOSTICS_H
#include <string>
#include <vector>
class Diagnostics { public: void error(const std::string &m); void warning(const std::string &m); bool hasErrors() const; void print() const; private: std::vector<std::string> errors_; std::vector<std::string> warnings_; };
#endif
