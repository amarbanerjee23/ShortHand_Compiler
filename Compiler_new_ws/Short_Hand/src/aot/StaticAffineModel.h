#pragma once
#include <cstddef>
#include <string>
#include <vector>

namespace shorthand::aot {
enum class ImportDisposition { Supported, RequiresBackendValidation, Rejected };
struct ImportResult;
class StaticAffineModel {
    friend ImportResult inspectStaticAffine(std::vector<unsigned char>);
    // Owned immutable bytes and digest identify exactly what was inspected.
    std::vector<unsigned char> snapshot_;
    std::string sha256_;
    std::size_t batch_=0,inputs_=0,outputs_=0;
    std::vector<float> weights_,bias_;
public:
    const std::vector<unsigned char> &snapshot() const { return snapshot_; }
    const std::string &sha256() const { return sha256_; }
    std::size_t batch() const { return batch_; }
    std::size_t inputs() const { return inputs_; }
    std::size_t outputs() const { return outputs_; }
    const std::vector<float> &weights() const { return weights_; }
    const std::vector<float> &bias() const { return bias_; }
};
struct ImportResult {
    ImportDisposition disposition=ImportDisposition::Rejected;
    std::string reason;
    StaticAffineModel model;
};
// No file access, protobuf SDK, ONNX SDK, Python, runtime routing or execution.
// RequiresBackendValidation is NOT proof of ONNX semantic validity: hybrid
// callers must independently validate/prepare with a qualified real backend.
// Rejected input must never be retried as a successful fallback.
ImportResult inspectStaticAffine(std::vector<unsigned char> snapshot);
}
