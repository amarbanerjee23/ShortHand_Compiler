# C3-ECO auditor evidence lifecycle

c3eco_auditor_contract: shorthand.c3eco.auditor_bundle.v1
c3eco_audit_policy_contract: shorthand.c3eco.audit_policy.v1
rules_mapping_contract: shorthand.c3eco.rules.v0.6+v0.7-20260718.v1
current_github_pr: 91
claim_status: candidate_evidence_only
official_certification_granted: false
level_claim_permitted: false
comparative_energy_claim: false
production_claim: false

PR91 provides a native C++17 auditor tool. OpenSSL 3.x supplies Ed25519 signatures through its EVP API; the existing native SHA-256 implementation hashes artifacts. The tool links the same PR90 assessment implementation used by `shorthand_c3eco_assess`. Signature verification and byte-for-byte assessment replay are independent requirements. A signed but incorrect assessment is rejected.

## Commands and trust

Build with CMake, or `make -C Compiler_new_ws/Short_Hand/src c3eco_auditor_tool`. The installed executable is `shorthand_c3eco_audit`. OpenSSL development headers/libcrypto are mandatory, with no unsigned fallback. Ubuntu uses libssl-dev; macOS uses openssl@3; Windows UCRT64 uses mingw-w64-ucrt-x86_64-openssl. Applications and the operational runtime do not acquire this dependency; the separate auditor executable does.

```
shorthand_c3eco_audit pack candidate policy.json signer-private.pem new-bundle
shorthand_c3eco_audit verify new-bundle trusted-public.pem 2026-09-09
shorthand_c3eco_audit replay new-bundle trusted-public.pem 2029-09-09
shorthand_c3eco_audit export-public new-bundle trusted-public.pem 2026-09-09 signer-private.pem new-public-bundle
shorthand_c3eco_audit verify-public new-public-bundle trusted-public.pem 2026-09-09
shorthand_c3eco_audit retention-check new-bundle trusted-public.pem 2029-09-09
shorthand_c3eco_audit readiness readiness-intake.json
```

Signing accepts an Ed25519 PKCS#8 PEM key. Verification requires a separately supplied, trusted Ed25519 public PEM key. A bundle never supplies its own trust root. The signer id is SHA-256 of the raw 32-byte Ed25519 public key. The signature covers the exact canonical manifest bytes prefixed by `shorthand.c3eco.auditor_bundle.v1` and a newline. The signature file contains 128 lowercase hexadecimal characters. Canonical JSON sorts object keys, uses compact separators and a terminating newline. Artifact hashes cover exact bytes, including line endings.

The caller establishes signer authority and manages approved keys, rotation and revocation outside this offline tool. Remove a revoked key from the keys accepted for current verification; historical replay requires an explicitly selected historical trust decision. A custodian signature proves origin and integrity under that trust decision. It does not prove auditor independence, meter calibration, physical measurement, or official certification. Private keys must remain outside candidate evidence and must not be committed or bundled. This interface does not assert HSM or FIPS qualification.

## Private inputs and signed lineage

The candidate directory contains the eight unchanged PR90 inputs: profile.json, measurement.json, metadata.tsv, gates.tsv, criteria.tsv, materiality.tsv, claims.tsv and regressions.tsv. All other input files must be under evidence/. Every populated TSV reference column ending in `_ref`, and every workbook record's evidence_ref, must resolve to a nonempty file there. A `#fragment` denotes a location within the retained file; PR91 binds the complete file, without interpreting application-specific fragment semantics. Remote references must be captured locally and referenced through this retained evidence path.

The private manifest hashes the complete candidate snapshot, policy, generated assessment and compiled rule mapping. The verifier requires exact inventory equality, verifies the external trusted signature, validates policy/profile dates and all references, and re-runs PR90 on a private snapshot. PR90 revalidates typed profile links and recomputes workbook accounting. A copied, edited or separately generated assessment cannot bypass replay. Raw measurement truth, the original compiler source-to-profile generation, and physical calibration are auditor review responsibilities, not claims made by assessment replay.

Inputs are bounded to 1,024 files, 32 MiB per file and 256 MiB per bundle. Artifact paths use portable ASCII components. Absolute paths, traversal, symlinks, Windows reparse points, hard links, reserved Windows names and case collisions are rejected. Work in caller-owned directories with no concurrent writers. Private staging has owner-only directory permissions where the OS supports POSIX permissions. The caller must enforce equivalent filesystem ACLs on Windows and control access to stored bundles. Output publication uses a new sibling staging directory and rename, refuses an existing destination, and cleans failed stages. No input, existing output, or evidence store is deleted.

## Retention, surveillance and nonconformities

Policy is a strict versioned JSON object. The test fixture in tests/c3eco/audit/test_auditor_bundle.py supplies a complete example. Required fields are:

- schema, bundle_id and custodian;
- created_on, valid_until, retain_until and legal_hold;
- recertification_accepted and acceptance_ref;
- storage_ref, access_review_ref and restore_test_ref;
- last_surveillance_on and surveillance_ref;
- changes, nonconformities and public_scope_approved.

Dates use real Gregorian YYYY-MM-DD values. Bundle validity must remain within the typed certification profile and at most 12 months after creation. Retention must extend at least 24 calendar months after bundle validity ends. This is deliberately more conservative than the draft's minimum 24-month evidence-retention requirement. Month addition clamps month-end dates, including leap years.

Surveillance uses PR90's conservative cadence: six months for AI or high-scale SaaS, otherwise 12 months, capped by bundle validity. At the due date, current verification fails. After validity ends, current verification also fails. Historical replay remains available and reports the expired/due state. As-of dates are explicit for reproducibility; a current operational decision must supply the trusted current date, not a chosen earlier date.

Each change has id, kind, observed_on and review_ref. Architecture, model, runtime, database, cloud_region, provider, hardware and traffic changes require documented review. A missing review marks recertification review required. PR90 eco/quality regression triggers also remain binding. Each nonconformity has id, severity, status, due_on and resolution_ref. Open major/critical findings block current review immediately. Open minor findings block at their deadline. Closing a finding requires retained resolution evidence. Future evidence and duplicate ids are rejected.

`retention-check` reports whether policy permits disposal after the retention deadline. Legal hold, any open finding or pending recertification prevents eligibility. It never deletes evidence. Storage immutability, encryption, access control, backup/restore operations and actual retention remain custodian responsibilities, evidenced by the retained storage/access/restore records. `storage_retention_independently_verified` is always false: policy validation does not pretend to enforce WORM storage or attest that years of retention have elapsed.

Exit 0 means the requested operation succeeded. Exit 2 indicates invalid input, trust/signature failure, missing evidence, replay mismatch or invalid public output. Exit 3 means an intact private bundle fails current lifecycle requirements, or disposal is not policy-eligible. `verify` and `retention-check` still emit the structured reason/status for exit 3. `replay` can succeed for an expired bundle while retaining all false certification/production/level/comparative-claim flags.

## Redacted public output

Public export requires current successful private verification, explicit public_scope_approved, and the trusted bundle's signing key. The public report contains approved product identity/version, software class, functional unit, boundary, workload, dates and a digest linking the private manifest. It excludes raw evidence, contact/custodian details, evidence paths, private policy, scores and recommended certification levels. No arbitrary source prose is copied into Markdown.

The four-file public envelope has report.json, report.md, manifest.json and manifest.sig. Its independent signature covers its exact artifact inventory and source-manifest digest. `verify-public` validates this smaller contract and dates. It truthfully reports `private_assessment_replayed:false`: a redacted report cannot independently replay evidence it does not contain. Authorities need the private bundle for that purpose. Export and local report generation do not publish to any network service or certification registry.

## Source editions and inclusive readiness

The rule map freezes v0.6 canonical G1-G14 numbering and the 18 July 2026 v0.7 inclusion overlay, including the G5/G6 cost treatment and shifted security/evidence gates. The source is the supplied authority-review draft and eligibility documents, not an assertion of adoption as a legal or international standard. Conflicts preserve canonical ids and the stricter applicable requirement for independent review. The entire rule-map artifact is checked against the compiled version during replay; schema/rule changes require explicit migration rather than silent reinterpretation.

A separate readiness intake accepts a declared product id, functional unit, boundary and zero or more transparent estimates. With no estimates it reports Registered/MQ0; documented positive estimates with uncertainty and source/method report Candidate/MQ1. Estimates never enter the instrument-backed measurement workbook v1. Readiness produces no measured workbook, criterion score or Bronze recommendation. Full MQ1 certification assessment still requires an independently reviewed scheme workflow; this implementation does not advertise that broader route as complete.

## Mandatory qualification

scripts/check_c3eco_auditor_bundle.sh uses real PR88 and PR89 producers and the common PR90 fixture. Its native end-to-end tests exercise deterministic packaging, independently verified OpenSSL signatures, wrong keys, tampering, missing/unlisted artifacts, a validly re-signed false assessment, malformed schemas, path safety, profile validity, retention boundary, legal holds, surveillance, nonconformities, recertification, redaction and separate estimated readiness. The gate is mandatory in direct CI, Make test, CTest, sanitizer, compiler-matrix and installed SDK lifecycle paths. It does not skip a missing crypto dependency or substitute fixture-only profiles for the real producers.

The mandatory CodeQL build includes the measurement, assessment and auditor executables, so security analysis covers their native implementations alongside the compiler and runtime.

PR92-PR96 retain responsibility for generated MLIR, production lowering, representative AI workloads, measured comparative energy and the enterprise pilot. TST017 still requires a real protected signed-release exercise. PR91 implements auditor preparation, not certification operations or production readiness.
