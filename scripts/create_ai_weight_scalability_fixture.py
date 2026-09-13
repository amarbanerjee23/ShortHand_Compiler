#!/usr/bin/env python3
"""120M FP32 external weights, deterministic and synthetic; never a trained LLM."""
import argparse
import hashlib
import json
import pathlib
import sys
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / "tests/ai_energy"))
from onnx_fixture import external_model, write_weights
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("output", type=pathlib.Path)
args = parser.parse_args()
args.output.mkdir(parents=True, exist_ok=True)
model, weights = args.output / "synthetic_120m.onnx", args.output / "weights.bin"
if model.exists() or weights.exists():
    parser.error("output artifacts already exist")
external_model(model, 10000, 12000)
digest = write_weights(weights, 120000000)
config = {"schema": "shorthand.ai.cpu_qualification.config.v1", "mode": "inference", "workload": "synthetic_120m_fp32_scalability",
          "model_path": str(model.resolve()), "model_sha256": hashlib.sha256(model.read_bytes()).hexdigest(),
          "external_files": [{"name": "weights.bin", "sha256": digest, "size_bytes": 480000000}],
          "input_shape": [1, 10000], "output_shape": [1, 12000], "threads": [1], "warmups": 1,
          "repetitions": 2, "trials": 2, "maximum_memory_bytes": 2147483648, "energy_source": "unavailable"}
(args.output / "qualification.json").write_text(json.dumps(config, indent=2) + "\n")
print(f"Created synthetic 120M FP32 fixture: graph={model.stat().st_size} bytes, external_weights=480000000 bytes; no trained-model claim")
