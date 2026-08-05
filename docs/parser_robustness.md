# Parser robustness and malformed-input contract

parser_robustness_status: bounded_fail_fast_negative_corpus_guarded

parser_robustness_contract_version: 1.0

language_version: beta-0.2

production_claim: false

## Scope

PR67 hardens the beta-0.2 scanner and parser against malformed, oversized and adversarial source files. It does not change the accepted grammar.

The parser remains fail-fast. It reports the first stable parser or scanner error and exits non-zero. Multi-error recovery is intentionally deferred because continuing after an invalid AI or Green AI declaration could create misleading downstream diagnostics.

## Production ceilings

The compiler enforces these maximums before semantic analysis:

| Guard | Production ceiling | Diagnostic |
| --- | ---: | --- |
| Regular source file size | 4 MiB | `SHD2004` |
| Scanner matches | 250,000 | `SHD2006` |
| Identifier, number or string token size | 16 KiB | `SHD2005` |
| Lexical delimiter nesting | 256 | `SHD2007` |

The environment variables below may lower a ceiling for testing or constrained deployments. They cannot raise the compiled production ceiling:

- `SHORTHAND_MAX_SOURCE_BYTES`
- `SHORTHAND_MAX_SCANNER_MATCHES`
- `SHORTHAND_MAX_TOKEN_BYTES`
- `SHORTHAND_MAX_NESTING_DEPTH`

Invalid, zero or above-ceiling values fall back to the production ceiling.

## Lexical diagnostics

The scanner provides explicit stable codes for failures that previously collapsed into a generic syntax error:

| Code | Meaning |
| --- | --- |
| `SHD2008` | Unexpected input byte |
| `SHD2009` | Unterminated block comment |
| `SHD2010` | Unterminated string literal |

A parser syntax failure remains `SHD2001`. Every error retains a source path and one-based source range.

## Executable evidence

`scripts/check_parser_robustness.sh` performs all of the following against the real compiler:

1. runs the versioned malformed-input corpus,
2. requires the expected stable code,
3. runs every static rejection twice and compares output and exit status,
4. enforces a wall-clock deadline,
5. rejects signal termination and sanitizer markers,
6. runs under a 512 MiB virtual-memory ceiling outside sanitizer mode,
7. generates source-size, scanner-work, token-size and nesting attacks,
8. executes a deterministic mutation smoke corpus,
9. accepts mutation success or coded rejection but never hangs or crashes.

The committed malformed corpus covers truncated blocks and expressions, unterminated strings and comments, unexpected bytes, malformed model and Green AI declarations, truncated inference, program-order violations and stray delimiters.

## Boundaries

This milestone does not claim:

- Unicode grapheme-aware columns,
- IDE-style multi-error recovery,
- coverage-guided fuzzing infrastructure,
- proof against every possible denial-of-service input,
- module or imported-file provenance,
- production readiness of the language as a whole.

The deterministic mutation gate is a reproducible CI baseline. Coverage-guided fuzzing can be added later without changing the beta-0.2 grammar contract.
