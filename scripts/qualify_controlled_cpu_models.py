#!/usr/bin/env python3
"""Required controlled model tier. Missing models, meters or quality results fail."""
import argparse
import hashlib
import json
import os
import pathlib
import subprocess
import urllib.request

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--tool", required=True)
parser.add_argument("--cache", type=pathlib.Path, required=True)
parser.add_argument("--output", type=pathlib.Path, required=True)
parser.add_argument("--download", action="store_true", help="fetch only the immutable, size/hash-pinned model artifacts")
parser.add_argument("--instrument", type=pathlib.Path, help="operator calibration, isolation and uncertainty JSON")
parser.add_argument("--guardrail-ref", default="")
parser.add_argument("--execution-only", action="store_true", help="explicit numerical execution run; cannot qualify measured energy")
args = parser.parse_args()
root = pathlib.Path(__file__).resolve().parents[1]
registry = json.loads((root / "tests/ai_energy/controlled/models.json").read_text())
if not args.execution_only and (not args.instrument or not args.guardrail_ref):
    parser.error("controlled energy qualification requires --instrument and --guardrail-ref")
args.cache.mkdir(parents=True, exist_ok=True)
args.output.mkdir(parents=True, exist_ok=True)
instrument = json.loads(args.instrument.read_text()) if args.instrument else {}


def digest(path):
    h = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest()


for model in registry["models"]:
    target = args.cache / (model["id"] + ".onnx")
    if not target.exists() and args.download:
        url = f'https://media.githubusercontent.com/media/onnx/models/{registry["source_revision"]}/{model["path"]}'
        temporary = target.with_suffix(".download")
        try:
            with urllib.request.urlopen(url, timeout=60) as source, temporary.open("xb") as out:
                total = 0
                while True:
                    chunk = source.read(1024 * 1024)
                    if not chunk:
                        break
                    total += len(chunk)
                    if total > model["size_bytes"]:
                        raise RuntimeError("model download exceeds pinned size")
                    out.write(chunk)
            if temporary.stat().st_size != model["size_bytes"] or digest(temporary) != model["sha256"]:
                raise RuntimeError("model download hash/size mismatch")
            temporary.rename(target)
        finally:
            temporary.unlink(missing_ok=True)
    if target.is_symlink() or not target.is_file() or target.stat().st_size != model["size_bytes"] or digest(target) != model["sha256"]:
        raise RuntimeError("required controlled model unavailable or corrupt: " + model["id"])
    config = {"schema": "shorthand.ai.cpu_qualification.config.v1", "mode": "inference", "workload": model["id"],
              "model_path": str(target.resolve()), "model_sha256": model["sha256"], "input_shape": model["input_shape"],
              "output_shape": model["output_shape"], "quality_metric": model["quality_metric"], "warmups": 3,
              "repetitions": 10, "trials": 5, "energy_source": "unavailable" if args.execution_only else "rapl",
              "require_measured_energy": not args.execution_only, "instrument": instrument, "guardrail_evidence_ref": args.guardrail_ref}
    path = args.output / (model["id"] + "-config.json")
    path.write_text(json.dumps(config, sort_keys=True, indent=2) + "\n")
    subprocess.run([args.tool, "qualify", str(path), str(args.output / (model["id"] + "-report.json"))],
                   env=dict(os.environ, ORT_DISABLE_TELEMETRY="1"), check=True, timeout=1800)
    (args.output / (model["id"] + "-provenance.json")).write_text(json.dumps(dict(model, source_revision=registry["source_revision"],
        execution_only=args.execution_only, dataset_class="deterministic_synthetic_inputs", official_certification=False), indent=2) + "\n")
print("PASS controlled standard CNN, small YOLO and vision transformer " + ("numerical execution only" if args.execution_only else "measured CPU qualification"))
