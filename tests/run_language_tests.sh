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
run_green(){ [[ -x "$GREEN" ]] || { bad "green_ai_tool missing"; return; }; bash "$ROOT_DIR/tests/green_ai/test_green_ai_tool.sh" && ok green_ai; }
run_compiler_smoke(){ need_compiler || return 0; "$SHORT" "$ROOT_DIR/Compiler_new_ws/Short_Hand/examples/greenai_report.short" run >/tmp/shorthand_smoke.out && ok interpreter_greenai || bad interpreter_greenai; "$SHORT" "$ROOT_DIR/Compiler_new_ws/Short_Hand/examples/greenai_report.short" compile-bc >/tmp/shorthand_compile.out 2>&1 && ok compile_bitcode || bad compile_bitcode; }
run_negative(){ need_compiler || return 0; if "$SHORT" "$ROOT_DIR/tests/parser/invalid/missing_semicolon.short" run >/tmp/shorthand_neg.out 2>&1; then bad parser_rejects_missing_semicolon; else ok parser_rejects_missing_semicolon; fi; }
run_ai(){
  need_compiler || return 0
  "$SHORT" "$ROOT_DIR/Compiler_new_ws/Short_Hand/examples/ai_library_abstraction.short" run >/tmp/shorthand_ai_runtime.out 2>&1 || { cat /tmp/shorthand_ai_runtime.out; bad ai_library_abstraction_run; return; }
  grep -q 'AI inference fallback' /tmp/shorthand_ai_runtime.out && grep -q 'runtime_backend=fallback' /tmp/shorthand_ai_runtime.out && grep -q 'inference_status=not_executed' /tmp/shorthand_ai_runtime.out && grep -q 'reason=backend_not_available' /tmp/shorthand_ai_runtime.out && ok ai_runtime_fallback || { cat /tmp/shorthand_ai_runtime.out; bad ai_runtime_fallback; }
  "$SHORT" "$ROOT_DIR/Compiler_new_ws/Short_Hand/examples/ai_library_abstraction.short" evidence >/tmp/shorthand_ai_evidence.json 2>&1 || { cat /tmp/shorthand_ai_evidence.json; bad ai_backend_evidence; return; }
  grep -q 'backend_preference' /tmp/shorthand_ai_evidence.json && grep -q 'runtime_backend' /tmp/shorthand_ai_evidence.json && grep -q 'inference_status' /tmp/shorthand_ai_evidence.json && grep -q 'classifier' /tmp/shorthand_ai_evidence.json && grep -q 'int8' /tmp/shorthand_ai_evidence.json && grep -q '1,3,224,224' /tmp/shorthand_ai_evidence.json && grep -q 'Evidence report only; this tool does not grant certification.' /tmp/shorthand_ai_evidence.json && ok ai_backend_evidence || { cat /tmp/shorthand_ai_evidence.json; bad ai_backend_evidence; }
  for invalid in incompatible_backend unknown_model_infer unknown_tensor_infer missing_model_guardrail; do
    if "$SHORT" "$ROOT_DIR/tests/semantic/invalid/${invalid}.short" run >/tmp/shorthand_${invalid}.out 2>&1; then
      cat /tmp/shorthand_${invalid}.out; bad "semantic_${invalid}"
    else
      ok "semantic_${invalid}"
    fi
  done
}
case "$MODE" in
  unit) run_green ;;
  compiler|llvm|sanitize|all) run_compiler_smoke ;;
  negative) run_negative ;;
  ai) run_ai ;;
  *) run_green; run_compiler_smoke; run_negative; run_ai ;;
esac
printf 'Summary: pass=%d fail=%d warning=%d\n' "$pass" "$fail" "$warn"
[[ "$fail" -eq 0 ]]
