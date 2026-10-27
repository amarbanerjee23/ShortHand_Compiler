#pragma once
// Internal, versioned source interface. Not installed and not exported by the
// shared library. No stable C++ ABI or source-language model embedding promise.
#include "../ai_runtime/AI_Types.h"
#include <cstddef>
#include <cstdint>
#include <string>

#if defined(__GNUC__) || defined(__clang__)
#define SHORTHAND_INTERNAL_MODEL __attribute__((visibility("hidden")))
#else
#define SHORTHAND_INTERNAL_MODEL
#endif

namespace shorthand::runtime::immutable_v1 {
struct Handle { std::uint64_t generation=0; };
struct Binding { Handle handle; std::string error; };

// One process-scoped slot, serialized with the public runtime facade. A
// successful bind replaces the slot; failed binds leave the previous slot
// usable. Caller storage must remain readable and unmodified during bind only.
// SHA-256 verifies exact owned bytes against a caller-trusted lowercase digest;
// it is not a signature. ModelSpec is copied; its path is diagnostic only.
SHORTHAND_INTERNAL_MODEL Binding bind(const ai::ModelSpec &,const void *bytes,
                                     std::size_t size,const std::string &sha256);
// Exact FP32 input/output counts. Caller owns valid aligned storage. Partial
// input/output overlap is supported. Output commits only on success; no caller
// pointer is retained. Calls are serialized, including reset and release.
SHORTHAND_INTERNAL_MODEL ai::InferenceResult infer(Handle,const float *input,
    std::size_t input_count,float *output,std::size_t output_count);
SHORTHAND_INTERNAL_MODEL bool release(Handle);
// short_runtime_reset invalidates every handle. Generations never wrap/reuse.
}
#undef SHORTHAND_INTERNAL_MODEL
