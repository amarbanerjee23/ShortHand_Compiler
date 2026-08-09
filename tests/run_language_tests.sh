#!/usr/bin/env bash
set -euo pipefail
MODE="${1:-all}"
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src"
BUILD_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/build"
SHORT="${BUILD_DIR}/short_hand"
GREEN="${BUILD_DIR}/green_ai_tool"
pass=0; fail=0; warn=0
ok(){ printf 'PASS %s\n' "$*"; pass=$((pass+1)); }
bad(){ printf 'FAIL %s\n' "$*"; fail=$((fail+1)); }
warning(){ printf 'WARN %s\n' "$*"; warn=$((warn+1)); }
need_compiler(){ [[ -x "$SHORT" ]] || { warning "short_hand unavailable; compiler-backed $MODE tests environment-limited"; return 1; }; }
run_green(){ [[ -x "$GREEN" ]] || { bad "green_ai_tool missing"; return; }; bash "$ROOT_DIR/tests/green_ai/test_green_ai_tool.sh" && ok green_ai || bad green_ai; }
run_compiler_smoke(){ need_compiler || return 0; "$SHORT" "$ROOT_DIR/Compiler_new_ws/Short_Hand/examples/greenai_report.short" run >/tmp/shorthand_smoke.out && ok interpreter_greenai || bad interpreter_greenai; "$SHORT" "$ROOT_DIR/Compiler_new_ws/Short_Hand/examples/greenai_report.short" compile-bc >/tmp/shorthand_compile.out 2>&1 && ok compile_bitcode || bad compile_bitcode; }
run_codegen(){ if need_compiler; then SHORTHAND_BIN="$SHORT" bash "$ROOT_DIR/tests/codegen/test_ai_metadata_ir.sh" && ok ai_metadata_ir || bad ai_metadata_ir; else warning "short_hand unavailable; codegen tests environment-limited"; fi; }
run_negative(){ need_compiler || return 0; if "$SHORT" "$ROOT_DIR/tests/parser/invalid/missing_semicolon.short" run >/tmp/shorthand_neg.out 2>&1; then bad parser_rejects_missing_semicolon; else ok parser_rejects_missing_semicolon; fi; for case_file in "$ROOT_DIR"/tests/semantic/invalid/*.short; do case_name=$(basename "$case_file" .short); if "$SHORT" "$case_file" run >/tmp/shorthand_semantic_neg.out 2>&1; then bad "semantic_rejects_${case_name}"; else ok "semantic_rejects_${case_name}"; fi; done; }
run_parser_robustness(){ need_compiler || return 0; if [[ "$MODE" == "sanitize" ]]; then SHORTHAND_BIN="$SHORT" SHORTHAND_PARSER_SANITIZER_MODE=1 bash "$ROOT_DIR/scripts/check_parser_robustness.sh" && ok parser_robustness_sanitized || bad parser_robustness_sanitized; else SHORTHAND_BIN="$SHORT" bash "$ROOT_DIR/scripts/check_parser_robustness.sh" && ok parser_robustness || bad parser_robustness; fi; }
run_modules(){ need_compiler || return 0; SHORTHAND_BIN="$SHORT" bash "$ROOT_DIR/scripts/check_module_ast_scaffold.sh" && ok module_ast_scaffold || bad module_ast_scaffold; }
run_module_resolution(){ need_compiler || return 0; SHORTHAND_BIN="$SHORT" bash "$ROOT_DIR/scripts/check_module_resolution.sh" && ok module_resolution || bad module_resolution; }
run_ai(){ if need_compiler; then SHORTHAND_BIN="$SHORT" bash "$ROOT_DIR/tests/ai_runtime/test_fallback.sh" && ok ai_runtime_fallback || bad ai_runtime_fallback; SHORTHAND_BIN="$SHORT" bash "$ROOT_DIR/tests/evidence/test_ai_evidence.sh" && ok ai_evidence_backend_fields || bad ai_evidence_backend_fields; bash "$ROOT_DIR/tests/ai_runtime/test_onnxruntime_backend_source.sh" && ok onnxruntime_backend_execution_source || bad onnxruntime_backend_execution_source; else warning "short_hand unavailable; AI tests environment-limited"; fi; }
run_ai_app(){ if [[ -x "${BUILD_DIR}/short_ai_app" ]]; then "${BUILD_DIR}/short_ai_app" missing.onnx "1" "0" >/tmp/shorthand_ai_app.out 2>&1 && bad ai_app_unexpected_success || ok ai_app_fallback_unavailable; else warning "short_ai_app unavailable; optional AI app tests environment-limited"; fi; }
case "$MODE" in
  unit) run_green ;;
  compiler) run_compiler_smoke; run_modules; run_module_resolution ;;
  sanitize) run_compiler_smoke; run_parser_robustness; run_modules; run_module_resolution ;;
  all) run_compiler_smoke; run_parser_robustness; run_modules; run_module_resolution ;;
  llvm) run_compiler_smoke; run_codegen; run_module_resolution ;;
  negative) run_negative; run_parser_robustness; run_modules; run_module_resolution ;;
  ai) run_ai ;;
  ai_app) run_ai_app ;;
  *) run_green; run_compiler_smoke; run_codegen; run_negative; run_parser_robustness; run_modules; run_module_resolution; run_ai ;;
esac
printf 'Summary: pass=%d fail=%d warning=%d\n' "$pass" "$fail" "$warn"
[[ "$fail" -eq 0 ]]
