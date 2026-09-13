#pragma once

#include <cstdint>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace shorthand::semantic_ir {

struct SourceLocation {
    std::string file;
    int line = 0;
    int column = 0;

    bool isKnown() const {
        return !file.empty() && line > 0 && column > 0;
    }
};

struct SourceRange {
    SourceLocation begin;
    SourceLocation end;

    bool isKnown() const {
        return begin.isKnown();
    }
};

enum class ElementType {
    Unknown,
    Float32,
    Float16,
    BFloat16,
    Int8,
    Int4,
    Int32
};

enum class ModelFormat {
    Unknown,
    Onnx,
    TensorRTEngine,
    TorchScript,
    OpenVINOIR,
    GGUF
};

enum class BackendKind {
    Fallback,
    OnnxRuntimeCPU,
    OnnxRuntimeCUDA,
    OnnxRuntimeTensorRT,
    TensorRT,
    OpenVINO,
    LibTorch,
    LlamaCpp
};

enum class OperationKind {
    Model,
    Tensor,
    Infer,
    GreenAIContract,
    GreenAIMeasurement
};

struct TensorShape {
    std::vector<std::int64_t> dims;

    bool isStatic() const {
        if (dims.empty()) return false;
        for (auto dim : dims) {
            if (dim <= 0) return false;
        }
        return true;
    }

    std::int64_t elementCount() const {
        if (!isStatic()) return 0;
        std::int64_t total = 1;
        for (auto dim : dims) {
            if (total > std::numeric_limits<std::int64_t>::max() / dim) return 0;
            total *= dim;
        }
        return total;
    }
};

struct Operation {
    OperationKind kind;
    std::string name;
    SourceRange source_range;

    Operation(OperationKind k, std::string n, SourceRange r = {})
        : kind(k), name(std::move(n)), source_range(std::move(r)) {}
};

struct TensorOp : Operation {
    ElementType element_type = ElementType::Unknown;
    TensorShape shape;
    // Empty source declarations have defined zero initialization. An explicit
    // initializer must contain exactly shape.elementCount() values.
    std::vector<double> values;

    TensorOp(std::string n, ElementType type, TensorShape tensor_shape, SourceRange r = {})
        : Operation(OperationKind::Tensor, std::move(n), std::move(r)),
          element_type(type),
          shape(std::move(tensor_shape)) {}
};

struct ModelOp : Operation {
    ModelFormat format = ModelFormat::Unknown;
    std::string path;
    std::string task;
    std::string quality_guardrail;
    ElementType precision = ElementType::Unknown;
    TensorShape input_shape;
    TensorShape output_shape;
    std::vector<BackendKind> backend_preference;

    explicit ModelOp(std::string n, SourceRange r = {})
        : Operation(OperationKind::Model, std::move(n), std::move(r)) {}
};

struct InferOp : Operation {
    std::string model_name;
    std::string input_tensor_name;
    std::string output_tensor_name;

    InferOp(std::string model, std::string input, std::string output, SourceRange r = {})
        : Operation(OperationKind::Infer, model, std::move(r)),
          model_name(std::move(model)),
          input_tensor_name(std::move(input)),
          output_tensor_name(std::move(output)) {}
};

struct GreenAIContractOp : Operation {
    std::string functional_unit;
    std::string success_criteria;
    std::vector<std::string> boundary;
    std::string measurement_quality;
    std::string data_quality;
    double carbon_factor = 0.0;
    std::string claims_mode;
    std::string quality_guardrail;
    double energy_budget_j = 0.0;
    double carbon_budget_gco2e = 0.0;

    explicit GreenAIContractOp(std::string n, SourceRange r = {})
        : Operation(OperationKind::GreenAIContract, std::move(n), std::move(r)) {}
};

struct GreenAIMeasurementOp : Operation {
    std::string workload;
    std::string backend;
    std::int64_t inferences = 0;
    double watts = 0.0;
    double seconds = 0.0;

    explicit GreenAIMeasurementOp(std::string n, SourceRange r = {})
        : Operation(OperationKind::GreenAIMeasurement, std::move(n), std::move(r)),
          workload(name) {}

    double energyJoules() const {
        return watts * seconds;
    }
};

// Owned executable semantics, independent of parser nodes, LLVM and MLIR.
// Bool storage is int32, while expressions retain their exact source type.
enum class ScalarType { Void, Int32, Bool, Float64, String, Composite };

struct Composite {
    enum class Kind { Record, Enum, Option, Result, Slice };
    Kind kind = Kind::Record;
    std::string name;
    std::vector<ScalarType> fields;
    std::vector<std::string> names;
};

struct Variable {
    std::string name;
    ScalarType type = ScalarType::Int32;
    std::int64_t extent = 0; // zero denotes a scalar, positive denotes an array
    SourceRange source_range;
    std::string composite;
};

struct Expression {
    enum class Kind { Integer, Boolean, Float, String, Variable, Index, Binary, Unary, Call, Construct, Project, Slice, SliceIndex, TensorIndex };
    Kind kind = Kind::Integer;
    ScalarType type = ScalarType::Int32;
    std::string name;
    std::int32_t integer = 0;
    double decimal = 0.0;
    int opcode = 0;
    std::vector<Expression> operands;
    SourceRange source_range;
    std::string composite;
};

struct Statement {
    enum class Kind { Block, Declare, Assign, Expression, If, While, For,
        Break, Continue, Return, Goto, Label, Read, Print, Model, Tensor,
        Infer, Contract, Measurement, LegacyInfer, GreenReport, Evidence, Update };
    Kind kind = Kind::Block;
    std::string name;
    std::vector<std::string> strings;
    std::vector<Expression> expressions;
    std::vector<Variable> variables;
    std::vector<Statement> body;
    std::vector<Statement> alternative;
    std::size_t declaration = 0;
    bool lexical_scope = false;
    SourceRange source_range;
};

struct Function {
    std::string name;
    ScalarType result = ScalarType::Void;
    std::vector<Variable> parameters;
    Statement body;
    SourceRange source_range;
    std::string result_composite;
};

struct ProgramIR {
    std::vector<Composite> composites;
    std::vector<ModelOp> models;
    std::vector<TensorOp> tensors;
    std::vector<InferOp> inferences;
    std::vector<GreenAIContractOp> contracts;
    std::vector<GreenAIMeasurementOp> measurements;
    std::vector<Variable> globals;
    std::vector<Function> functions;
    Statement body;

    bool empty() const {
        return composites.empty() && models.empty() && tensors.empty() && inferences.empty() &&
               contracts.empty() && measurements.empty() && globals.empty() &&
               functions.empty() && body.body.empty();
    }
};

} // namespace shorthand::semantic_ir
