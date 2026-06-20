#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT_DIR="${ROOT_DIR}/results/raw"
mkdir -p "$OUT_DIR"
echo "benchmark,compile_seconds,run_seconds,binary_bytes,status" > "$OUT_DIR/benchmark_summary.csv"
if [[ ! -x "${ROOT_DIR}/Compiler_new_ws/Short_Hand/build/short_hand" ]]; then
  echo "short_hand compiler unavailable; run setup_build_infra.sh first" >&2
  exit 1
fi
for src in "${ROOT_DIR}"/benchmarks/short/*.short; do
  [[ -e "$src" ]] || continue
  name="$(basename "$src" .short)"
  /usr/bin/time -f '%e' -o /tmp/sh_compile_time "${ROOT_DIR}/Compiler_new_ws/Short_Hand/build/short_hand" "$src" compile-native >/tmp/sh_bench_compile.out 2>&1 || { echo "$name,NA,NA,NA,compile_fail" >> "$OUT_DIR/benchmark_summary.csv"; continue; }
  bin="./${name}"
  /usr/bin/time -f '%e' -o /tmp/sh_run_time "$bin" >/tmp/sh_bench_run.out 2>&1 || { echo "$name,$(cat /tmp/sh_compile_time),NA,NA,run_fail" >> "$OUT_DIR/benchmark_summary.csv"; continue; }
  bytes=$(wc -c < "$bin")
  echo "$name,$(cat /tmp/sh_compile_time),$(cat /tmp/sh_run_time),$bytes,pass" >> "$OUT_DIR/benchmark_summary.csv"
done
