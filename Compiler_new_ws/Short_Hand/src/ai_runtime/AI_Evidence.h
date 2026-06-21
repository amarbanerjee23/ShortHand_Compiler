#pragma once
#include <string>
namespace shorthand::ai { struct EvidenceEvent { std::string runtime_backend="fallback"; std::string inference_status="not_executed"; std::string reason="backend_not_available"; }; }
