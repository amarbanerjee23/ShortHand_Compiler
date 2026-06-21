#pragma once
#include "../AI_Backend.h"
namespace shorthand::ai { class LibTorchBackend final : public AIBackend { public: std::string name() const override; bool available() const override; InferenceResult infer(const ModelSpec&) override; }; }
