#pragma once
#include <cstdint>
#include <set>
#include <string>
namespace shorthand::ai {
// Bounded streaming inspection of the ONNX protobuf envelope. Tensor payloads
// are skipped in-place; ONNX Runtime remains responsible for graph semantics.
std::set<std::string> inspectOnnxExternalFiles(const std::string &,std::uint64_t maximum_bytes);
}
