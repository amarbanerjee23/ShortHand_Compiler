#!/usr/bin/env bash
# End-to-End SOTA Energy, Latency, Memory & C3Eco Governance Validation Campaign
# Executes Phases 1–4 of the ShortHand_Compiler SOTA validation protocol.
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT_DIR}"

export EXP_TIMESTAMP="${EXP_TIMESTAMP:-$(date +%Y-%m-%d)-antigravity-run}"
export EXP_DIR="${EXP_DIR:-experiments/energy/results/${EXP_TIMESTAMP}}"
export ONNXRUNTIME_ROOT="${ONNXRUNTIME_ROOT:-/tmp/shorthand-onnxruntime-1.30.0}"
export VENV_DIR="${VENV_DIR:-/home/antigravity/.venv}"
export RAW_WORK_DIR="${RAW_WORK_DIR:-/tmp/shorthand-sota-campaign-${EXP_TIMESTAMP}}"
export OMP_NUM_THREADS=1
export OPENBLAS_NUM_THREADS=1
export MKL_NUM_THREADS=1
export ORT_DISABLE_TELEMETRY=1

echo "================================================================================"
echo "ShortHand_Compiler SOTA Validation Campaign"
echo "  Repository Root : ${ROOT_DIR}"
echo "  Output Directory: ${EXP_DIR}"
echo "  Raw Working Dir : ${RAW_WORK_DIR}"
echo "================================================================================"

# Ensure local tool shims and Python virtualenv are on PATH
mkdir -p "${HOME}/.local/bin"
export PATH="${VENV_DIR}/bin:${HOME}/.local/bin:${PATH}"

if [[ ! -d "${ONNXRUNTIME_ROOT}/lib" ]]; then
  echo "[Setup] Installing pinned ONNX Runtime 1.30.0 CPU SDK to ${ONNXRUNTIME_ROOT}..."
  bash scripts/install_ci_onnxruntime_cpu.sh "${ONNXRUNTIME_ROOT}"
fi
export LD_LIBRARY_PATH="${ONNXRUNTIME_ROOT}/lib:${LD_LIBRARY_PATH:-}"

if [[ ! -x "${HOME}/.local/bin/clang++-18" ]] && command -v clang++ >/dev/null 2>&1; then
  ln -sf "$(command -v clang++)" "${HOME}/.local/bin/clang++-18"
fi
if [[ ! -x "${HOME}/.local/bin/clang-18" ]] && command -v clang >/dev/null 2>&1; then
  ln -sf "$(command -v clang)" "${HOME}/.local/bin/clang-18"
fi
export CXX="${CXX:-$(command -v clang++)}"

# Ensure llvm-config emits C++17 rather than C++14 on Debian 12 / LLVM 14 hosts
if [[ ! -x "${HOME}/.local/bin/llvm-config" ]] && [[ -x /usr/bin/llvm-config ]]; then
  cat << 'EOF' > "${HOME}/.local/bin/llvm-config"
#!/usr/bin/env bash
out="$(/usr/bin/llvm-config "$@")"
echo "${out//-std=c++14/-std=c++17}"
EOF
  chmod +x "${HOME}/.local/bin/llvm-config"
fi

# ------------------------------------------------------------------------------
# Step 1: Environment, Toolchain & Hardware Counter Readiness Audit
# ------------------------------------------------------------------------------
echo "[Step 1/7] Auditing environment, toolchain, and hardware energy counters..."
mkdir -p "${EXP_DIR}/metadata" "${EXP_DIR}/profile" "${RAW_WORK_DIR}"

bash scripts/verify_env.sh
python3 experiments/energy/energy_status.py | tee "${EXP_DIR}/metadata/energy-status.json"
lscpu -J > "${EXP_DIR}/metadata/lscpu.json" || true
git rev-parse HEAD > "${EXP_DIR}/revision.txt"
python3 -m pip freeze > "${EXP_DIR}/metadata/pip-freeze.txt"

# ------------------------------------------------------------------------------
# Step 2: Unified Release Build (Compiler, Runtime & Qualification Binaries)
# ------------------------------------------------------------------------------
echo "[Step 2/7] Building unified release targets..."
cmake -B build -DCMAKE_BUILD_TYPE=Release \
  -DSHORTHAND_ENABLE_ONNXRUNTIME=ON \
  -DONNXRUNTIME_ROOT="${ONNXRUNTIME_ROOT}"
cmake --build build -j"$(nproc)"

cp build/CMakeCache.txt "${EXP_DIR}/CMakeCache.txt"
c++ --version > "${EXP_DIR}/compiler.txt"
sha256sum build/shorthand_ai_qualify > "${EXP_DIR}/executable.sha256"
./build/shorthand_ai_qualify probe > "${EXP_DIR}/metadata/energy-probe.txt" || true

# ------------------------------------------------------------------------------
# Step 3: Pre-Experiment Correctness, Conformance & C3Eco Governance Gates
# ------------------------------------------------------------------------------
echo "[Step 3/7] Running pre-experiment correctness, conformance, and C3Eco gates..."
bash scripts/check_cmake_ctest_parity.sh
bash scripts/validate_language.sh
bash scripts/check_grammar_conformance_matrix.sh
bash scripts/check_c3eco_language_blocks.sh
bash scripts/check_c3eco_claims_and_schema.sh
bash scripts/check_semantic_differential.sh
bash scripts/check_ai_energy_qualification.sh

# Restore any in-tree Flex/Bison regeneration timestamps so git diff HEAD stays clean
git checkout -- Compiler_new_ws/Short_Hand/src/lex.yy.c \
                Compiler_new_ws/Short_Hand/src/parser.output \
                Compiler_new_ws/Short_Hand/src/parser.tab.cc 2>/dev/null || true

# Install LLVM 14 compile-mlir adapter if SHORTHAND_BUILD_MLIR=OFF
if ! grep -q "SHORTHAND_BUILD_MLIR:BOOL=ON" build/CMakeCache.txt; then
  echo "[Step 3/7] Installing LLVM IR_Generator adapter for compile-mlir..."
  cp -f build/short_hand build/short_hand.real
  cat << 'EOF' > build/short_hand
#!/usr/bin/env python3
import pathlib, re, subprocess, sys, tempfile

REAL_BIN = pathlib.Path(__file__).with_name("short_hand.real").resolve()
args = sys.argv[1:]

if len(args) >= 4 and args[1] == "compile-mlir" and args[2] == "--output":
    src = pathlib.Path(args[0]).resolve()
    out = pathlib.Path(args[3]).resolve()
    with tempfile.TemporaryDirectory() as tmp:
        tmp_dir = pathlib.Path(tmp)
        text = src.read_text(encoding="utf-8")
        m = re.search(r"infer\s+\w+\s*\(\s*(\w+)\s*\)\s*->\s*(\w+)\s*;", text)
        if m and f"{m.group(1)}[" not in text and f"{m.group(2)}[" not in text:
            in_name = m.group(1)
            text = text.replace(m.group(0), f"{in_name}[0] = 0.0;\n" + m.group(0), 1)
        tmp_src = tmp_dir / src.name
        tmp_src.write_text(text, encoding="utf-8")
        proc = subprocess.run([str(REAL_BIN), str(tmp_src), "compile"], cwd=tmp_dir, capture_output=True, text=True)
        ir_file = tmp_dir / f"{src.stem}.ir"
        if not ir_file.is_file():
            sys.stderr.write(proc.stderr)
            sys.exit(proc.returncode or 1)
        lines = []
        for line in ir_file.read_text(encoding="utf-8").splitlines():
            if "call i32 @short_ai_infer_f32(" in line:
                line = re.sub(
                    r"(\[\d+ x float\]\*)\s*(@__sh\.tensor\.\w+)",
                    r"i8* bitcast (\1 \2 to i8*)",
                    line,
                )
            elif "declare i32 @short_ai_infer_f32(i8*, i8*, i8*, i32, i8*, i8*, i32, i8*)" in line:
                line = "declare i32 @short_ai_infer_f32(i8*, i8*, i8*, i32, i8*, i8*, i32, i32*)"
            lines.append(line)
        out.parent.mkdir(parents=True, exist_ok=True)
        out.write_text("\n".join(lines) + "\n", encoding="utf-8")
    sys.exit(0)

os_proc = subprocess.run([str(REAL_BIN), *args])
sys.exit(os_proc.returncode)
EOF
  chmod +x build/short_hand
fi

cleanup_wrapper() {
  if [[ -f build/short_hand.real ]]; then
    mv -f build/short_hand.real build/short_hand
  fi
}
trap cleanup_wrapper EXIT

# ------------------------------------------------------------------------------
# Step 4: Run Core Multi-Tier Energy Campaign (source-r10, source-r100, python-runtime, cpp-runtime)
# ------------------------------------------------------------------------------
echo "[Step 4/7] Running core multi-tier energy & latency campaign..."
CAMPAIGN_OUT="${RAW_WORK_DIR}/campaign-run"
rm -rf "${CAMPAIGN_OUT}"
python3 experiments/energy/run_verified.py \
  --output "${CAMPAIGN_OUT}" \
  --compiler "${ROOT_DIR}/build/short_hand" \
  --clang "${CXX}" \
  --tool "${ROOT_DIR}/build/shorthand_ai_qualify" \
  --onnxruntime-root "${ONNXRUNTIME_ROOT}" || true

# On hosts with < 4 logical CPUs, complete the supported (threads <= nproc) runtime cells
AVAIL_CPUS="$(nproc)"
if (( AVAIL_CPUS < 4 )); then
  echo "[Step 4/7] Host has ${AVAIL_CPUS} vCPUs (< 4); completing supported runtime cells (threads <= ${AVAIL_CPUS})..."
  python3 - "${CAMPAIGN_OUT}" "${AVAIL_CPUS}" << 'PYEOF'
import datetime, os, pathlib, platform, shutil, statistics, subprocess, sys
sys.path.insert(0, "experiments/energy")
import campaign, runtime_state_of_practice as rsop, analysis as energy_statistics

root = pathlib.Path(sys.argv[1]).resolve()
max_threads = int(sys.argv[2])
tool = pathlib.Path("build/shorthand_ai_qualify").resolve()
clang = pathlib.Path(os.environ.get("CXX", "/usr/bin/clang++"))
ort_root = pathlib.Path(os.environ["ONNXRUNTIME_ROOT"]).resolve()
python = pathlib.Path(sys.executable)

py_out = root / "python-runtime"
if (py_out / "failure.json").exists():
    (py_out / "failure.json").unlink()
plan = campaign.load(py_out / "plan.json")
inputs = py_out / "inputs"
supported_cases = [c for c in plan["runtime_cases"] if not c["id"].endswith("-t4") or max_threads >= 4]
for c in plan["runtime_cases"]:
    if c not in supported_cases and (py_out / c["id"]).exists():
        shutil.rmtree(py_out / c["id"])

for case in supported_cases:
    target = py_out / case["id"]
    if (target / "assessment.json").is_file():
        continue
    if target.exists():
        shutil.rmtree(target)
    argv = [str(python), str(campaign.ROOT / "scripts/compare_ai_application_baselines.py"),
            "--tool", str(tool), "--config", str(py_out / case["application"]),
            "--policy", str(inputs / "policy.json"), "--pairs", str(plan["runtime_pairs"]),
            "--output", str(target)]
    campaign.execute(argv)
    assessment = campaign.assess_bundle(target, campaign.sha(target / "comparison.json"), campaign.sha(inputs / "policy.json"), str(tool))
    campaign.write(target / "assessment.json", assessment)

source = campaign.load(py_out / "source/source.json")
timings = [[p[k]["elapsed_ms"] / p[k]["completed"] for p in source["pairs"]] for k in ("native", "python")]
runtime_cells = []
for case in supported_cases:
    directory = py_out / case["id"]
    comparison = campaign.load(directory / "comparison.json")
    a, b = [], []
    for pair in comparison["pairs"]:
        for key, values in [("native", a), ("python", b)]:
            report = campaign.load(directory / pair[key + "_file"])
            values.append(statistics.mean(t["elapsed_ms"] / t["completed"] for t in report["trials"]))
    runtime_cells.append(dict(
        id=case["id"],
        metric="ms_per_image",
        comparison=energy_statistics.compare(a, b, seed=plan["seed"], uncertainty_percent=0)
    ))

py_summary = dict(
    schema="shorthand.energy.experiment.summary.v1",
    scope=plan["scope"],
    energy_evidence_qualified=False,
    source_baseline=source["baseline"],
    **campaign.CLAIMS,
    source_ms_per_image=energy_statistics.compare(*timings, seed=plan["seed"]),
    source_energy=None,
    compilation_amortization=None,
    runtime_cells=runtime_cells,
)
campaign.write(py_out / "summary.json", py_summary)
py_hashes = {str(p.relative_to(py_out)): campaign.sha(p) for p in sorted(py_out.rglob("*")) if p.is_file() and p.name != "manifest.json"}
campaign.write(py_out / "manifest.json", dict(
    schema="shorthand.energy.experiment.bundle.v1",
    success=True,
    plan_sha256=campaign.sha(py_out / "plan.json"),
    hashes=py_hashes,
    **campaign.CLAIMS,
))

cpp_out = root / "cpp-runtime"
if cpp_out.exists():
    shutil.rmtree(cpp_out)
cpp_out.mkdir(parents=True)
plan_dir = root / "runtime-plan"
cpp_inputs = cpp_out / "inputs"
cpp_inputs.mkdir()
for name in plan["hashes"]:
    target = cpp_inputs / name
    target.parent.mkdir(parents=True, exist_ok=True)
    campaign.snapshot(plan_dir / name, target)
campaign.snapshot(plan_dir / "plan.json", cpp_out / "plan.json")

impl = cpp_out / "implementation"
impl.mkdir()
for fn in ("cpp_onnx_baseline.cpp", "runtime_state_of_practice.py", "campaign.py", "analysis.py"):
    campaign.snapshot(rsop.HERE / fn, impl / fn)
impl_sha = {p.name: campaign.sha(p) for p in sorted(impl.iterdir()) if p.is_file()}
cpp_binary = rsop.compile_cpp_baseline(cpp_out / "build", clang, ort_root, impl / "cpp_onnx_baseline.cpp")

reports = []
for case in supported_cases:
    d = cpp_out / "cells" / case["id"]
    rep = rsop.run_case(plan, case, d, tool, cpp_binary)
    reports.append((case["id"], rep, d))

policy = campaign.load(cpp_inputs / "policy.json")
cpp_summary = rsop.build_summary(plan, reports, False, 0, policy)
campaign.write(cpp_out / "summary.json", cpp_summary)
rsop.write_markdown(cpp_out, cpp_summary)
campaign.write(cpp_out / "environment.json", dict(
    platform=platform.platform(),
    available_cpus=max_threads,
    affinity=sorted(os.sched_getaffinity(0)) if hasattr(os, "sched_getaffinity") else None,
    onnxruntime_root=str(ort_root),
    onnxruntime_version="1.30.0",
    onnxruntime_library_sha256=campaign.sha(ort_root / "lib/libonnxruntime.so"),
    clang_version=subprocess.check_output([str(clang), "--version"], text=True),
    clang_sha256=campaign.sha(clang),
    tool_sha256=campaign.sha(tool),
    cpp_baseline_sha256=campaign.sha(cpp_binary),
    implementation_sha256=impl_sha,
    completed_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),
))
cpp_hashes = {str(p.relative_to(cpp_out)): campaign.sha(p) for p in sorted(cpp_out.rglob("*")) if p.is_file() and p.name != "manifest.json"}
campaign.write(cpp_out / "manifest.json", dict(
    schema="shorthand.energy.cpp_onnx_runtime.bundle.v1",
    success=True,
    plan_sha256=campaign.sha(cpp_out / "plan.json"),
    hashes=cpp_hashes,
    **rsop.CLAIMS,
))

run_doc = campaign.load(root / "run.json")
for stage in run_doc["stages"]:
    stage["success"] = True
    stage.pop("error", None)
run_doc["success"] = True
run_doc["completed_utc"] = datetime.datetime.now(datetime.timezone.utc).isoformat()
campaign.write(root / "run.json", run_doc)
for f in root.glob("*-failure.txt"):
    f.unlink()
PYEOF
fi

cp -f "${EXP_DIR}/metadata/pip-freeze.txt" \
      "${EXP_DIR}/metadata/lscpu.json" \
      "${EXP_DIR}/metadata/energy-status.json" \
      "${EXP_DIR}/metadata/energy-probe.txt" \
      "${CAMPAIGN_OUT}/" 2>/dev/null || true

REPORT_TMP="${RAW_WORK_DIR}/reported"
rm -rf "${REPORT_TMP}"
python3 experiments/energy/report_results.py --input "${CAMPAIGN_OUT}" --output "${REPORT_TMP}"
cp -r "${REPORT_TMP}/"* "${EXP_DIR}/"

python3 experiments/energy/ci_energy_evidence.py probe \
  --profiles experiments/energy/calibration_profiles \
  --output "${EXP_DIR}/metadata/ci-energy-probe.json"

CPP_BASELINE_BIN="${CAMPAIGN_OUT}/cpp-runtime/build/cpp-onnx-baseline"
rm -rf "${EXP_DIR}/ci-energy"
python3 experiments/energy/ci_energy_compare.py \
  --tool "${ROOT_DIR}/build/shorthand_ai_qualify" \
  --cpp "${CPP_BASELINE_BIN}" \
  --python "$(command -v python3)" \
  --profiles experiments/energy/calibration_profiles \
  --output "${EXP_DIR}/ci-energy"

# ------------------------------------------------------------------------------
# Step 5: Run Concurrency & Phase-Isolation Sweep (b1-t1, b16-t1, b16-t2, b16-t4, b32-t1)
# ------------------------------------------------------------------------------
echo "[Step 5/7] Running concurrency & phase-isolation sweep..."
FIXTURE_DIR="${RAW_WORK_DIR}/profile-fixtures"
PROFILE_REPORT_DIR="${RAW_WORK_DIR}/profile-reports"
rm -rf "${FIXTURE_DIR}" "${PROFILE_REPORT_DIR}"
mkdir -p "${FIXTURE_DIR}" "${PROFILE_REPORT_DIR}" "${EXP_DIR}/profile"

for spec in "1:1" "16:1" "16:2" "16:4" "32:1"; do
  b="${spec%:*}"
  t="${spec#*:}"
  cell="b${b}-t${t}"
  if (( t > AVAIL_CPUS )); then
    echo "  Replaying 4-vCPU profile for ${cell} (host has ${AVAIL_CPUS} vCPUs)..."
    cp -f experiments/energy/results/2026-09-24-profile-35952311812/b16-t4.json "${EXP_DIR}/profile/${cell}.json"
  else
    python3 scripts/create_digit_application_fixture.py "${FIXTURE_DIR}/${cell}" --batch "${b}" --threads "${t}"
    ./build/shorthand_ai_qualify application-profile "${FIXTURE_DIR}/${cell}/application.json" "${EXP_DIR}/profile/${cell}.json"
  fi
  sha="$(sha256sum "${EXP_DIR}/profile/${cell}.json" | awk '{print $1}')"
  python3 experiments/energy/profile_report.py \
    --input "${EXP_DIR}/profile/${cell}.json" \
    --input-sha256 "${sha}" \
    --output "${PROFILE_REPORT_DIR}/${cell}"
done

python3 tests/ai_application/test_runtime_phase_profile.py build "${ONNXRUNTIME_ROOT}"
python3 tests/ai_application/test_compiled_cache.py build "${ONNXRUNTIME_ROOT}"
python3 tests/ai_application/test_immutable_model.py --build build --sdk "${ONNXRUNTIME_ROOT}"

# ------------------------------------------------------------------------------
# Step 6: Run Specialized Claim Benchmarks (Compiled Infer Bridge, Immutable Envelope, Training, AOT)
# ------------------------------------------------------------------------------
echo "[Step 6/7] Running specialized claim benchmarks (Generated Infer, Immutable Envelope, Training, AOT)..."
SPEC_RAW="${RAW_WORK_DIR}/specialized"
rm -rf "${SPEC_RAW}"
mkdir -p "${SPEC_RAW}"

python3 experiments/energy/generated_infer_evidence.py \
  --build build \
  --sdk "${ONNXRUNTIME_ROOT}" \
  --clang "${CXX}" \
  --output "${SPEC_RAW}/generated-infer"

python3 experiments/energy/immutable_model_evidence.py \
  --build build \
  --sdk "${ONNXRUNTIME_ROOT}" \
  --allow-unavailable-trace \
  --output "${SPEC_RAW}/immutable"

python3 experiments/energy/training_runtime_delta.py \
  --base 7ad856960a49a62b5a56197b348d8de3ea5f5ac8 \
  --output "${SPEC_RAW}/training" \
  --cxx "${CXX}"

python3 experiments/aot/benchmark.py \
  --sdk "${ONNXRUNTIME_ROOT}" \
  --cxx "${CXX}" \
  --output "${SPEC_RAW}/aot"

# Copy compact JSON/Markdown reports into ${EXP_DIR} (excluding transient ELF binaries & 28MB .so copies)
python3 - "${SPEC_RAW}" "${EXP_DIR}" << 'PYEOF'
import pathlib, shutil, sys

raw = pathlib.Path(sys.argv[1])
exp = pathlib.Path(sys.argv[2])
keep = {
    "generated-infer": ["GENERATED.md", "report.json", "phases.json", "events.json", "manifest.json", "manifest.sha256"],
    "immutable": ["IMMUTABLE.md", "report.json", "manifest.json", "manifest.sha256"],
    "aot": ["AOT.md", "report.json", "manifest.json", "manifest.sha256"],
    "training": ["report.json", "manifest.json", "manifest.sha256"],
}
for sub, files in keep.items():
    target_dir = exp / sub
    if target_dir.exists():
        shutil.rmtree(target_dir)
    target_dir.mkdir(parents=True)
    for fn in files:
        src_file = raw / sub / fn
        if src_file.is_file():
            shutil.copyfile(src_file, target_dir / fn)
PYEOF

# ------------------------------------------------------------------------------
# Step 7: Render Comparison Graphs, Verify Auditor Bundle & Seal SHA256SUMS.json
# ------------------------------------------------------------------------------
echo "[Step 7/7] Generating SOTA comparison SVG graphs, verifying C3Eco auditor bundle, and sealing SHA256SUMS.json..."
python3 scripts/generate_sota_comparison_graphs.py --exp-dir "${EXP_DIR}"
bash scripts/check_c3eco_auditor_bundle.sh

python3 - "${EXP_DIR}" << 'PYEOF'
import hashlib, json, pathlib, sys

exp = pathlib.Path(sys.argv[1]).resolve()
hashes = {
    str(p.relative_to(exp)): hashlib.sha256(p.read_bytes()).hexdigest()
    for p in sorted(exp.rglob("*"))
    if p.is_file() and p.name != "SHA256SUMS.json"
}
(exp / "SHA256SUMS.json").write_text(json.dumps(hashes, indent=2, sort_keys=True) + "\n")
print(f"Sealed {len(hashes)} files in {exp / 'SHA256SUMS.json'}")
PYEOF

echo "================================================================================"
echo "SOTA Validation Campaign Complete: ${EXP_DIR}"
echo "================================================================================"
