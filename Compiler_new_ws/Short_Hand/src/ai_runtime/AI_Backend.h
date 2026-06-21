#pragma once
#include "AI_Types.h"
#include <string>
namespace shorthand::ai { class AIBackend { public: virtual ~AIBackend()=default; virtual std::string name() const=0; virtual bool available() const=0; virtual InferenceResult infer(const ModelSpec&)=0; }; }
