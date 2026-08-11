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
has_sanitizer_failure(){ grep -Eq 'AddressSanitizer|LeakSanitizer|UndefinedBehaviorSanitizer|runtime error:|SUMMARY:.*Sanitizer|Segmentation fault' "$1" 2>/dev/null; }
run_green(){ [[ -x "$GREEN" ]] || { bad "green_ai_tool missing"; return; }; bash "$ROOT_DIR/tests/green_ai/test_green_ai_tool.sh" && ok green_ai || bad green_ai; }
run_compiler_smoke(){ need_compiler || return 0; "$SHORT" "$ROOT_DIR/Compiler_new_ws/Short_Hand/examples/greenai_report.short" run >/tmp/shorthand_smoke.out 2>/tmp/shorthand_smoke.err && ok interpreter_greenai || bad interpreter_greenai; "$SHORT" "$ROOT_DIR/Compiler_new_ws/Short_Hand/examples/greenai_report.short" compile-bc >/tmp/shorthand_compile.out 2>/tmp/shorthand_compile.err && ok compile_bitcode || bad compile_bitcode; }
run_codegen(){ if need_compiler; then SHORTHAND_BIN="$SHORT" bash "$ROOT_DIR/tests/codegen/test_ai_metadata_ir.sh" && ok ai_metadata_ir || bad ai_metadata_ir; else warning "short_hand unavailable; codegen tests environment-limited"; fi; }
run_diagnostics(){ if need_compiler; then SHORTHAND_BIN="$SHORT" bash "$ROOT_DIR/tests/diagnostics/test_source_diagnostics.sh" && ok source_diagnostics || bad source_diagnostics; else warning "short_hand unavailable; diagnostics tests environment-limited"; fi; }
run_negative(){
  need_compiler || return 0
  local parser_out=/tmp/shorthand_neg.out
  if "$SHORT" "$ROOT_DIR/tests/parser/invalid/missing_semicolon.short" run >"$parser_out" 2>&1; then
    bad parser_rejects_missing_semicolon
  elif has_sanitizer_failure "$parser_out"; then
    bad parser_missing_semicolon_sanitizer_failure
  else
    ok parser_rejects_missing_semicolon
  fi
  for case_file in "$ROOT_DIR"/tests/semantic/invalid/*.short; do
    case_name=$(basename "$case_file" .short)
    case_out="/tmp/shorthand_semantic_neg_${case_name}.out"
    if "$SHORT" "$case_file" run >"$case_out" 2>&1; then
      bad "semantic_rejects_${case_name}"
    elif has_sanitizer_failure "$case_out"; then
      bad "semantic_${case_name}_sanitizer_failure"
    else
      ok "semantic_rejects_${case_name}"
    fi
  done
}
run_parser_robustness(){ need_compiler || return 0; if [[ "$MODE" == "sanitize" ]]; then SHORTHAND_BIN="$SHORT" SHORTHAND_PARSER_SANITIZER_MODE=1 bash "$ROOT_DIR/scripts/check_parser_robustness.sh" && ok parser_robustness_sanitized || bad parser_robustness_sanitized; else SHORTHAND_BIN="$SHORT" bash "$ROOT_DIR/scripts/check_parser_robustness.sh" && ok parser_robustness || bad parser_robustness; fi; }
run_modules(){ need_compiler || return 0; SHORTHAND_BIN="$SHORT" bash "$ROOT_DIR/scripts/check_module_ast_scaffold.sh" && ok module_ast_scaffold || bad module_ast_scaffold; }
run_module_resolution(){ need_compiler || return 0; bash "$ROOT_DIR/scripts/check_module_resolver_unit.sh" && SHORTHAND_BIN="$SHORT" bash "$ROOT_DIR/scripts/check_module_resolution.sh" && ok module_resolution || bad module_resolution; }
run_ai(){ if need_compiler; then SHORTHAND_BIN="$SHORT" bash "$ROOT_DIR/tests/ai_runtime/test_fallback.sh" && ok ai_runtime_fallback || bad ai_runtime_fallback; SHORTHAND_BIN="$SHORT" bash "$ROOT_DIR/tests/evidence/test_ai_evidence.sh" && ok ai_evidence_backend_fields || bad ai_evidence_backend_fields; bash "$ROOT_DIR/tests/ai_runtime/test_onnxruntime_backend_source.sh" && ok onnxruntime_backend_execution_source || bad onnxruntime_backend_execution_source; else warning "short_hand unavailable; AI tests environment-limited"; fi; }
run_ai_app(){ if [[ -x "${BUILD_DIR}/short_ai_app" ]]; then "${BUILD_DIR}/short_ai_app" missing.onnx "1" "0" >/tmp/shorthand_ai_app.out 2>&1 && bad ai_app_unexpected_success || ok ai_app_fallback_unavailable; else warning "short_ai_app unavailable; optional AI app tests environment-limited"; fi; }
case "$MODE" in
  unit) run_green; bash "$ROOT_DIR/scripts/check_module_resolver_unit.sh" && ok module_resolver_unit || bad module_resolver_unit ;;
  compiler) run_compiler_smoke; run_modules; run_module_resolution ;;
  sanitize) run_compiler_smoke; run_codegen; run_diagnostics; run_negative; run_parser_robustness; run_modules; run_module_resolution; run_ai ;;
  all) run_compiler_smoke; run_parser_robustness; run_modules; run_module_resolution ;;
  llvm) run_compiler_smoke; run_codegen; run_module_resolution ;;
  negative) run_negative; run_parser_robustness; run_modules; run_module_resolution ;;
  ai) run_ai ;;
  ai_app) run_ai_app ;;
  *) run_green; run_compiler_smoke; run_codegen; run_diagnostics; run_negative; run_parser_robustness; run_modules; run_module_resolution; run_ai ;;
esac
printf 'Summary: pass=%d fail=%d warning=%d\n' "$pass" "$fail" "$warn"
[[ "$fail" -eq 0 ]]
