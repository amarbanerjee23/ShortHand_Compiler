# Language Compatibility and Evolution Policy

language_compatibility_contract: shorthand.language.compatibility.v1
active_language_version: beta-0.7
base_grammar_version: beta-0.2
current_maturity: controlled_beta
production_claim: false

ShortHand beta-0.7 syntax is supported for controlled pilot users. Compatibility is governed by executable conformance fixtures, not documentation alone. The executable subset remains beta-0.5; beta-0.6 adds enterprise schema and ownership validation, and beta-0.7 adds the typed C3-ECO profile.

Covered constructs:

- tensor declarations
- model declarations
- inference statements
- GreenAI contracts
- GreenAI measurements
- evidence mode
- module/import/package preambles
- C3-ECO candidate-evidence declarations
- `shorthand.c3eco.profile.v2` typed profile links and migration output
- exact float, string and fixed typed-array execution
- `shorthand.type_memory.v1` descriptor, storage and ownership rules
- `shorthand.control_flow.v1` expression calls, lexical locals, recursion, structured returns and safe labels
- `shorthand.enterprise_language.v1` namespaced composite schemas and ownership plans
- `shorthand.package.v2`, `shorthand.lock.v2` and core FFI ABI 1.0.0

Changes to covered syntax must include migration notes, updated examples, positive/negative validation coverage, and interpreter/bitcode/native differential tests when executable behavior changes. A breaking change requires a new language version and an explicit migration path; it may not silently reinterpret a valid beta-0.7 program.

Evidence report schema changes should include a schema version and release note.

The frozen runtime ABI and separate core FFI ABI are independently versioned at 1.0.0. Language compatibility does not imply composite interpreter/LLVM lowering, owned string arrays, nested dependency resolution, a network registry or by-value composite FFI.
