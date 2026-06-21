#pragma once
#include "../AI_Backend.h"
namespace shorthand::ai { class FallbackBackend final : public AIBackend { public: std::string name() const override { return "fallback"; } bool available() const override { return true; } InferenceResult infer(const ModelSpec&) override; }; }
