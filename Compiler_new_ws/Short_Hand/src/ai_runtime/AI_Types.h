#pragma once
#include <string>
#include <vector>
namespace shorthand::ai { struct ModelSpec { std::string name, path, format, inputShape, precision; std::vector<std::string> backendPreference; bool allowFallback=true; }; struct InferenceResult { std::string runtimeBackend="fallback", inferenceStatus="not_executed", reason="backend_not_available"; }; }
