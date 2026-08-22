# Language Compatibility and Evolution Policy

language_compatibility_contract: shorthand.language.compatibility.v1
active_language_version: beta-0.5
base_grammar_version: beta-0.2
current_maturity: controlled_beta
production_claim: false

ShortHand beta-0.5 syntax is supported for controlled pilot users. Compatibility is governed by executable conformance fixtures, not documentation alone.

Covered constructs:

- tensor declarations
- model declarations
- inference statements
- GreenAI contracts
- GreenAI measurements
- evidence mode
- module/import/package preambles
- C3-ECO candidate-evidence declarations
- exact float, string and fixed typed-array execution
- `shorthand.type_memory.v1` descriptor, storage and ownership rules
- `shorthand.control_flow.v1` expression calls, lexical locals, recursion, structured returns and safe labels

Changes to covered syntax must include migration notes, updated examples, positive/negative validation coverage, and interpreter/bitcode/native differential tests when executable behavior changes. A breaking change requires a new language version and an explicit migration path; it may not silently reinterpret a valid beta-0.5 program.

Evidence report schema changes should include a schema version and release note.

The frozen runtime ABI is independently versioned at 1.0.0. Language compatibility does not imply source syntax for descriptor-only composites or support for owned string arrays. Source composite/ownership integration, packages, a supported standard library, long-term deprecation windows and stable FFI policy remain PR86 blockers.
