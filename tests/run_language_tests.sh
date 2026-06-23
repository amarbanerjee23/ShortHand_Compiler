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
run_ai(){ if need_compiler; then SHORTHAND_BIN="$SHORT" bash "$ROOT_DIR/tests/ai_runtime/test_fallback.sh" && ok ai_runtime_fallback || bad ai_runtime_fallback; SHORTHAND_BIN="$SHORT" bash "$ROOT_DIR/tests/evidence/test_ai_evidence.sh" && ok ai_evidence_backend_fields || bad ai_evidence_backend_fields; else warning "short_hand unavailable; AI tests environment-limited"; fi; }
case "$MODE" in
  unit) run_green ;;
  compiler|llvm|sanitize|all) run_compiler_smoke ;;
  negative) run_negative ;;
  ai) run_ai ;;
  *) run_green; run_compiler_smoke; run_negative; run_ai ;;
esac
printf 'Summary: pass=%d fail=%d warning=%d\n' "$pass" "$fail" "$warn"
[[ "$fail" -eq 0 ]]
