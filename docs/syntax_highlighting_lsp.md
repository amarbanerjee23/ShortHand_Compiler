# ShortHand syntax highlighting and LSP contract

lsp_editor_contract_version: shorthand.tooling.lsp.v1
language_version: beta-0.3
protocol_baseline: JSON-RPC 2.0 / LSP 3.17-compatible stdio subset
production_claim: false

## Purpose

This contract defines the first production-governed editor tooling baseline for ShortHand. It adds scanner-aligned syntax highlighting and a native compiled language server without changing ShortHand language semantics, runtime behavior or backend qualification claims.

## Transport and safety contract

`shorthand_lsp` uses stdio with `Content-Length` framing. Message bodies are bounded to 1 MiB, duplicate or malformed length headers fail closed, truncated messages fail closed, JSON nesting is bounded, duplicate JSON object keys are rejected and malformed JSON produces JSON-RPC parse error `-32700` when framing remains synchronized.

The server advertises UTF-16 positions and converts between UTF-8 source storage and UTF-16 editor columns. Shutdown followed by exit returns success. Exit without a preceding shutdown returns non-zero. No network listener, telemetry exporter or remote execution path is introduced.

## Language-server capabilities

The v1 server implements:

- `initialize`, `initialized`, `shutdown` and `exit`,
- full-document `textDocument/didOpen`, `didChange` and `didClose`,
- compiler-backed `textDocument/publishDiagnostics`,
- deterministic `textDocument/completion`,
- `textDocument/hover`,
- local and imported-module `textDocument/definition`,
- `textDocument/documentSymbol`,
- `$/cancelRequest` with JSON-RPC cancellation error `-32800` for a canceled queued request.

Compiler diagnostics are obtained from the real `short_hand` parser through an isolated temporary source file. The compiler executable is supplied through `SHORTHAND_COMPILER` or resolved as `short_hand` from `PATH`. If the compiler oracle is unavailable, the server publishes `SHLSP900` instead of reporting a false clean document.

Imported-module definition navigation reads the existing deterministic `shorthand.package` manifest and resolves the declared module path. It does not add an editor-only package format or silently search arbitrary filesystem paths.

## Syntax-highlighting contract

`editors/vscode/syntaxes/shorthand.tmLanguage.json` is a TextMate-compatible grammar aligned to the Flex scanner. It covers:

- `//`, `#` and block comments,
- escaped double-quoted strings,
- package/module/import/as preambles,
- base control-flow and declaration keywords,
- numeric and Boolean literals,
- operators,
- model, tensor and inference declarations,
- backend/format and precision tokens,
- current Green AI and measurement declarations.

The VS Code contribution binds `.short` files to `source.shorthand` and provides bracket, comment and auto-closing configuration. The grammar is declarative editor metadata only. Parser acceptance remains authoritative.

## Mandatory evidence

`scripts/check_lsp_editor.sh` must pass under GCC, Clang and ASan/UBSan in `.github/workflows/tooling.yml`. The same gate also executes from the inherited `ubuntu-core` feature-plan path so a green side workflow cannot mask a broken editor contract.

The gate requires:

1. machine-parseable editor JSON artifacts,
2. successful initialize/capability negotiation,
3. compiler-backed clean and failing diagnostics,
4. completion and hover responses,
5. local/document symbol evidence,
6. imported-module definition navigation through `shorthand.package`,
7. partial-document failure followed by clean recovery,
8. cancellation result `-32800`,
9. UTF-16 range correctness with multibyte UTF-8 source,
10. bounded malformed, duplicate and oversized framing negatives,
11. malformed JSON recovery,
12. explicit compiler-oracle-unavailable diagnostics,
13. non-zero exit when shutdown was not requested,
14. bounded protocol-session execution through `timeout`.

## Claim boundary

This PR does not claim semantic completion, rename/refactor support, workspace-wide indexing, semantic tokens, debugger integration, remote language-server hosting, backend execution or full IDE parity. Future syntax must extend both the compiler and editor corpora. TST021 closes only after the exact PR head passes this v1 contract and all inherited mandatory gates.
