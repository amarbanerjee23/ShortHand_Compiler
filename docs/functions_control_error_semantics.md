# Functions, control flow and deterministic errors

language_version: beta-0.5
control_flow_contract: shorthand.control_flow.v1
call_evaluation_order: left_to_right
goto_resolution: same_lexical_block_only
local_cleanup: lexical_block_exit_and_function_frame_exit
maximum_interpreter_call_depth: 256
maximum_interpreter_goto_transfers: 1000000
resource_limit_equivalence: terminating_programs_within_interpreter_budgets
runtime_abi_change: none
c3eco_alignment: repeatability_safeguards_evidence_integrity_no_quality_degradation
production_claim: false

## Executable contract

Beta-0.5 makes function calls first-class expressions. Calls accept zero or more arbitrary expressions, evaluate arguments from left to right, enforce exact parameter types and may be nested or recursive. The existing semicolon-terminated parameter declaration syntax remains accepted, and an empty parameter list is now valid.

Typed declarations may appear inside function and nested statement blocks. A function body shares its parameter scope; each nested braced block creates a child lexical scope. Names may shadow outer names but cannot be redeclared in the same scope. Interpreter frames release strings, arrays and scalars on every normal or control-flow exit. LLVM uses entry-block allocation with declaration-point initialization and scope-aware symbol lookup.

`if`/`else`, counted loops, condition loops, `break`, `continue`, `return`, labels and `goto` are executable in both interpreter and LLVM modes. A label is visible only in its containing statement block. `goto` cannot enter or leave another lexical block and cannot cross a local declaration boundary. These restrictions make cleanup and initialization behavior deterministic.

Cross-mode equivalence is qualified for terminating programs that remain within the interpreter's 256-call and 1,000,000-transfer safety budgets. The interpreter fails deterministically with `SHD7004` when either budget is exceeded; compiled recursion remains bounded by the host stack, and compiled non-terminating control flow remains bounded by the invoking process policy. PR87 owns cross-engine serving deadlines and resource ceilings. No unbounded-recursion or non-termination guarantee is made.

Non-void functions must return a value on every structured path. Undefined functions and labels, duplicate labels and declarations, illegal scope transfers, void-value consumption, arity errors and exact type mismatches fail before execution. Diagnostics SHD3017 through SHD3023 are stable beta-0.5 semantic codes. Existing SHD3001 through SHD3016 meanings remain unchanged; SHD3006 is retained as the historical pre-beta-0.5 unsupported-goto boundary.

## Compatibility and claim boundary

All valid beta-0.2 through beta-0.4 programs remain supported. Beta-0.5 intentionally expands acceptance to empty function parameter lists. The frozen runtime ABI remains version 1.0.0 with the same 25 public `short_*` symbols.

This contract strengthens repeatability, safeguards and evidence-processing integrity associated with C3-ECO controls G6, G7, G8 and G13. It does not demonstrate lower energy use, grant certification or make ShortHand production-ready.
