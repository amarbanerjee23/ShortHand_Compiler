# Typed C3-ECO certification profile

c3eco_profile_contract: shorthand.c3eco.profile.v2
language_version: beta-0.7
profile_schema: schemas/c3eco/profile_v2.schema.json
migration_schema: shorthand.c3eco.profile_migration.v1
production_claim: false
official_certification_granted: false

## Purpose

The typed profile links product identity, a useful-work denominator, a representative workload, a product boundary, AI lifecycle responsibility, safeguards and a validity window into one compiler-checked candidate profile. It closes the language and schema portions of C3-ECO draft controls G1, G2 and G3. It strengthens, but does not complete, safeguard control G7 and materiality control G14.

This profile does not measure energy, calculate carbon or cost, assign a score, select a certification level, issue a certificate or represent external auditor approval. Those capabilities remain assigned to PR89 through PR91. A conformant profile is preparation evidence only.

## Profile syntax

```short
certification_profile product_profile {
  profile_version 2;
  certification product_identity;
  functional_unit successful_inference_unit;
  workload representative_workload;
  boundary product_boundary;
  ai_lifecycle hosted_model_lifecycle;
  guardrails service_guardrails;
  valid_from "2026-08-29";
  valid_until "2027-08-29";
};
```

References are identifiers and are resolved after the complete source unit is collected, so forward references are deterministic. A missing target fails with `SHD5204`; a name bound to the wrong declaration kind fails with `SHD5205`.

## Native field types

Profile v2 fields use five AST value kinds:

| ShortHand literal | Profile type | Evidence JSON |
| --- | --- | --- |
| `"text"` | string | JSON string |
| `product_boundary` | identifier | JSON string plus `type:identifier` |
| `1000` | integer | JSON number plus `type:integer` |
| `0.90` | decimal | JSON number plus `type:decimal` |
| `true` or `false` | boolean | JSON boolean plus `type:boolean` |

Legacy `shorthand.c3eco.language.v1` declarations remain accepted with string fields. They are emitted as `legacy_migration_review_required`; they are not silently reinterpreted as v2 values.

## Product identity and validity

A linked `certification` declaration requires a numeric `MAJOR.MINOR.PATCH` version, non-empty owner, valid `YYYY-MM-DD` release date, supported software-class identifier, supported deployment-mode identifier, uppercase two- or three-letter geography code, and integer validity period from 1 through 366 days.

The profile itself requires valid ISO dates and `valid_from < valid_until`. These checks bind evidence to one named product, version, owner, release and assessment window.

## Functional unit and workload

A linked functional unit requires a positive integer denominator, a unit enum, a string success condition, a quality-metric enum and a decimal quality threshold from 0 through 1. Optional latency and error-rate limits are range checked.

The linked workload uses bounded integers for batch size, concurrency, warmup and measured runs. Traffic and cache state are closed enums. This prevents stringly typed denominators and accidental toy-workload metadata from being accepted as a typed profile.

## Boundary and materiality

Boundary components are closed identifiers. Every exclusion requires one string reason and one decimal materiality percentage. Cumulative exclusions must not exceed `materiality_threshold_percent`. Excluding `thirdparty_ai_api` additionally requires one of `conservative_estimate`, `included` or `provider_evidence` as the opaque-provider treatment.

This is a declaration and pre-measurement materiality control. PR89 still owns measured component reconciliation, allocation and double-counting checks.

## AI lifecycle and safeguards

AI lifecycle roles and scopes are closed enums. Training, fine-tuning and evaluation coverage are native booleans. Trainer and fine-tuner roles cannot exclude the lifecycle evidence they own.

The linked guardrails require enabled functional tests, security scan, accessibility and privacy telemetry, plus bounded accuracy, latency and error-rate values. A profile cannot describe efficiency by disabling one of these safeguard floors. Actual scoring and quality-equivalence decisions remain outside profile v2.

## Migration

```text
short_hand legacy.short c3eco-migrate --output migration.json
```

The deterministic `shorthand.c3eco.profile_migration.v1` artifact reports the source and target contracts, first unambiguous legacy references, missing review work and `official_certification_granted:false`. It does not guess typed numbers, booleans, enums, validity dates or materiality from prose strings. A current v2 source produces `already_current`; a legacy source produces `review_required`.

## Diagnostics

| Code | Meaning |
| --- | --- |
| `SHD5201` | Literal type or cardinality is invalid. |
| `SHD5202` | Closed domain or enum value is invalid. |
| `SHD5203` | Integer or decimal value is outside its allowed range. |
| `SHD5204` | Profile reference is unknown. |
| `SHD5205` | Profile reference resolves to the wrong declaration kind. |
| `SHD5206` | Validity date or ordering is invalid. |
| `SHD5207` | Exclusion evidence or cumulative materiality is invalid. |
| `SHD5208` | A v2-linked declaration is missing a v2-only field. |

`SHD5101` through `SHD5104` remain active for duplicate declarations, missing base fields, invalid fields and unsafe self-certification claims.

## Qualification

`scripts/check_c3eco_certification_profile.sh` executes the direct semantic/evidence unit, parser and AST positive path, native JSON type assertions, forward reference resolution, eight-code negative matrix, legacy migration, current-version migration, LLVM metadata when `llvm-dis` is available, contextual-keyword compatibility and claim-safety checks. The gate is registered in direct CI, Make, sanitizers and CTest parity.
