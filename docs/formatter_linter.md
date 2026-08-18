# ShortHand formatter and linter baseline

formatter_linter_contract_version: shorthand.tooling.format_lint.v1
roadmap_closure_pr: PR78
production_claim: false

## Purpose

The formatter and linter provide a deterministic source-style baseline for valid ShortHand programs without changing language tokens, token order or executable behavior. This is language tooling, not a claim that every future grammar feature or editor integration is complete.

## Build

```sh
make -C Compiler_new_ws/Short_Hand/src/tooling
```

The default executable is `Compiler_new_ws/Short_Hand/build/shorthand_tool`.

## CLI

```sh
shorthand_tool program.short format
shorthand_tool program.short format --output formatted.short
shorthand_tool program.short lint
shorthand_tool program.short lint --output lint.json
shorthand_tool program.short fix --output fixed.short
```

`fix` requires an explicit output path. It never rewrites the source file implicitly.

## Canonical formatter contract

The v1 formatter changes trivia only:

1. normalize CRLF/CR line endings to LF,
2. use four spaces per brace-nesting level for leading indentation,
3. remove trailing horizontal whitespace,
4. collapse repeated blank lines,
5. remove blank lines at end of file,
6. end non-empty formatted source with exactly one newline,
7. preserve comments, string contents, token spelling and token order.

Braces inside string literals and line/block comments do not affect indentation state.

The mandatory gate proves parser acceptance before and after formatting, byte-for-byte idempotence on a second format pass and identical interpreter output for the preservation fixture.

## Lint schema

Machine diagnostics use schema `shorthand.lint.v1`:

```json
{
  "schema": "shorthand.lint.v1",
  "path": "program.short",
  "diagnostics": [
    {
      "code": "SHL003",
      "severity": "warning",
      "line": 4,
      "column": 1,
      "message": "non-canonical indentation",
      "fixable": true
    }
  ]
}
```

Baseline rules:

| Code | Meaning | Safe fix |
| --- | --- | --- |
| SHL001 | tab used for leading indentation | replace with canonical spaces |
| SHL002 | trailing horizontal whitespace | remove trailing whitespace |
| SHL003 | indentation differs from brace-derived canonical indentation | re-indent line |
| SHL004 | final newline missing | add final newline |
| SHL005 | repeated or trailing blank line | collapse/remove blank line |
| SHL006 | non-LF line ending | normalize to LF |

`lint` exits non-zero when diagnostics exist. The JSON document is still emitted so CI and editor clients can consume the findings.

## Safety boundary

The formatter does not rename identifiers, reorder declarations/imports, rewrite expressions, change literals or apply semantic refactors. Safe-fix mode is therefore intentionally limited to the same trivia transformations as `format`.

The formatter/linter baseline does not replace parser, semantic, differential, sanitizer, security, portability, release or deployment gates. Syntax highlighting and LSP behavior remain roadmap PR79 work.
