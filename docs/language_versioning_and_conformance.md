# ShortHand language versioning and conformance policy

## Purpose

This document defines the current beta language compatibility contract for ShortHand.

The current language contract marker is:

`shorthand.language.version: beta-0.1`

The current conformance contract marker is:

`shorthand.conformance.contract: beta-0.1`

These markers are intentionally conservative. They do not claim that ShortHand is a complete production language. They identify the language surface that current parser, semantic, diagnostics, runtime, evidence, and codegen tests are expected to protect.

## Current beta surface

The `beta-0.1` language surface includes the currently documented and tested forms for:

- core statements,
- `tensor` declarations,
- `model` declarations,
- `infer model(input) -> output;`,
- legacy `ai_infer` compatibility,
- `greenai_contract` declarations,
- `greenai_measure` declarations,
- GreenAI report and evidence paths,
- semantic validation for model, tensor, backend, input shape, and output shape compatibility,
- source diagnostics for supported semantic errors,
- runtime hook registration and bridge behavior,
- evidence output claim-safety fields.

## Compatibility rules

A change is compatible with `beta-0.1` when it:

1. keeps existing valid conformance fixtures accepted,
2. keeps existing invalid conformance fixtures rejected,
3. does not rename existing public syntax without a documented migration path,
4. does not remove runtime/evidence behavior that the conformance manifest depends on,
5. updates the EBNF draft, language spec, conformance manifest, and feature tracker when syntax or semantics change.

A change is breaking when it:

1. removes or renames an existing accepted keyword or declaration form,
2. changes the meaning of `infer model(input) -> output;`,
3. weakens semantic checks for tensor/model/backend compatibility,
4. makes fallback inference report successful execution without a real backend,
5. removes a conformance category or fixture without a replacement and rationale.

Breaking changes must create a new language version marker instead of silently editing the existing `beta-0.1` contract.

## Conformance manifest rules

Every non-comment entry in `tests/conformance/manifest.txt` must include:

```text
category | path | expectation | rationale
```

The manifest must include at least one entry for each required beta category:

- `parser-valid`
- `parser-invalid`
- `semantic-invalid`
- `diagnostics`
- `codegen`
- `runtime`
- `evidence`

The manifest must also retain the version marker:

`version | shorthand.language.version | beta-0.1 | Current beta language contract marker.`

## Review rule

Any PR that changes parser syntax, semantic meaning, runtime hook behavior, evidence output, or public language examples must update this document or explicitly state why the `beta-0.1` contract remains unchanged.
