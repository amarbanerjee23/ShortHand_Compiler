# Independent reproduction and draft-standard pilot

independent_pilot_contract: shorthand.c3eco.independent_pilot.v1
independent_pilot_status: implemented_candidate_verification_external_authenticity_pending
production_claim: false
official_certification_granted: false
level_claim_permitted: false
comparative_energy_claim: false

PR101 extends `shorthand_c3eco_audit` with `verify-pilot`. It consumes PR91 private bundles, replays their PR90 assessments and compares their PR89 instrumented workbooks. It verifies retained, signed records; it does not perform physical measurements, operate organizational storage, authenticate an organization's legal independence or issue a certificate. G8/S9/S12 and the physical performance/energy blockers remain partial.

## Receiving a pilot

```bash
shorthand_c3eco_audit verify-pilot /evidence/pilot /trust/pilot-trust.json 2026-09-17
```

The receiving reviewer supplies the trusted current date and an independently obtained trust policy outside the pilot directory. That policy pins the pilot ID, three distinct organization IDs and Ed25519 public keys, source revision, exact compiler profile and metadata digests, expected output digest, minimum repeats, maximum relative difference and maximum uncertainty. Key paths are portable relative paths from the trust policy directory. Neither the policy nor a trust key may come from inside the pilot. Key rotation or scope changes require a new reviewed policy. Possession of distinct keys alone does not prove organizational independence: validate identities, authority and conflicts of interest outside this verifier.

The pilot directory contains `review.json`, its hex-encoded `review.sig`, `operations.json`, retained `operations/` artifacts and separate top-level PR91 private-bundle directories. The signed review inventories every other file with its exact path, byte length and SHA-256. Each run entry specifies `reference` or `repeat` and its bundle directory. Use at least three and at most sixteen runs per organization. The original producer signs reference bundles, the reproducing organization signs repeat bundles and a third reviewer signs the review. All signatures use Ed25519. Existing PR91 bundle signing is unchanged.

Every run bundle includes `candidate/evidence/pilot-run.json`. Its attestation records the run ID, organization, date, revision, completed units, environment digest/reference and result digest/reference. Both referenced artifacts must exist inside `candidate/evidence/` and match their digests. The externally pinned output digest is the exact-result equivalence contract for this version. Tolerance-based numerical equivalence needs a separately versioned contract, not an arbitrary replacement digest.

The review must use `linux-x64-cpu-v1` and the existing `shorthand.c3eco.rules.v0.6+v0.7-20260718.v1` rule map. These are draft-standard pilot records, not adoption of a public standard. The published strict schemas are `schemas/c3eco/independent_pilot_v1.schema.json`, `pilot_trust_v1.schema.json`, `pilot_run_v1.schema.json`, `pilot_operations_v1.schema.json` and `pilot_verification_v1.schema.json`.

## Verification and comparison

The verifier rejects missing/unlisted/tampered artifacts, unsafe paths, links, duplicate IDs or bundle paths, shared signing identities, overlapping reference/repeat environments, reused workbooks or instrument samples, changed source/workload/output, unsupported claims and invalid dates or numeric bounds. It copies the signed inventory into private snapshots before replay, so later changes to external evidence cannot substitute different inputs. PR91's file-count, size, strict JSON, lifetime and assessment-replay checks remain mandatory. The global pilot limit is 1,024 files and 256 MiB; each file retains the 32 MiB input limit.

All source assessments must be current and replay exactly. The profile and metadata must match externally pinned baseline bytes. Each workbook's measurement date must match its attested run date. Completed functional units must be positive integers. Duplicate instrument/environment/timestamp samples and duplicate workbook bytes cannot count as independent repeats.

For each run, energy is `facility_energy_kwh * 3,600,000 / completed_units`; uncertainty is normalized the same way. Group means weight each run equally. The conservative difference is `100 * (abs(repeat_mean - reference_mean) + reference_mean_uncertainty + repeat_mean_uncertainty) / reference_mean`. It must fit the receiver's pinned tolerance, and each run's relative uncertainty must fit the pinned limit. Within each group, `100 * (abs(run_energy - group_mean) + run_uncertainty + group_mean_uncertainty) / group_mean` must also fit that tolerance: matching means cannot hide unstable opposing outliers. This is a conservative consistency check, not a statistical confidence interval, a test of lower consumption, or a generalization to other workloads/hardware. The verifier never infers comparative energy superiority.

## Organizational evidence and lifecycle

`operations.json` retains custodian and observation dates, storage immutability/encryption and access-review attestations, retention deadline, surveillance schedule, open major findings and unreviewed changes. Its storage, access, surveillance, independence and draft-review references must resolve to nonempty retained files. Restore source and result must be separate retained artifacts with identical bytes. Missing evidence is invalid input; failed controls, changed restore bytes, open findings or overdue surveillance produce a blocked decision.

Retention must cover every underlying bundle deadline and at least 24 months after pilot validity. Pilot validity cannot outlive a source bundle or exceed twelve months. The organizational surveillance schedule must be within six months of the recorded observation and not already due. This validates signed organizational attestations and a retained restore comparison; it neither creates WORM storage nor proves that years of retention have elapsed. Custodians must actually operate, monitor and independently review these controls.

## Signing the review

Serialize `review.json` as UTF-8 JSON with sorted object keys, no extra whitespace, ordered artifact inventory and one final LF. The signature payload is the exact bytes of `shorthand.c3eco.independent_pilot.v1`, LF, and `review.json`. Sign with `openssl pkeyutl -sign -rawin -inkey reviewer.pem -in payload -out signature.bin`; encode the 64-byte signature as 128 lowercase hex characters with no newline in `review.sig`. Keep private keys outside evidence directories. `tests/c3eco/audit/test_independent_pilot.py` demonstrates independent OpenSSL signing and contract construction, using explicitly synthetic organizational fixtures and actual compiler/workbook producers.

## Results and mandatory tests

Exit 0 emits `candidate_reproduction_consistent`; exit 3 emits `blocked_by_pilot_evidence` with sorted reason codes. Exit 2 rejects invalid or untrusted input and emits no success receipt. Successful receipts always keep certification, level, production, comparative-energy, physical-authenticity, organizational-independence and independent-storage-verification flags false. Store the receipt with its source review and trusted-policy digests. Verification writes only to stdout and temporary private snapshots; it does not publish reports, run commands from evidence or delete retained evidence.

Run `bash scripts/check_c3eco_auditor_bundle.sh` from a configured checkout. PR101 is mandatory inside the existing auditor gate, including CI, Make, CTest, GCC/Clang, sanitizers, CodeQL and installed SDK lifecycle. Tests exercise six signed real-producer fixture bundles, exact output equivalence, independent signature validation, deterministic replay, wrong trust, tampering, false claims, missing provenance, repeated evidence, numeric boundaries and blocked operational decisions. CI fixtures do not close physical field evidence or an independent certification decision.

After PR101, PR102 is the remaining planned implementation batch. Actual independent field repeats, complete family baselines, calibrated physical observations, retained organizational operation/review and the protected signed-release exercise still govern release eligibility; one remaining code batch is not a promise of general availability.
