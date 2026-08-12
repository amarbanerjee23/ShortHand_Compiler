# External security, dependency and license policy

external_security_policy_version: shorthand.security.external.v1
roadmap_pr: PR76
github_pr: PR77
production_claim: false
tst018_status: implemented_after_required_scanners_pass
codeql_security_severity_threshold: 7.0
security_exception_max_days: 90

## Objective

PR77 makes external source/dependency security a fail-closed production gate rather than a release-documentation claim. It adds independent C/C++ SAST, vulnerability/secret/misconfiguration/license scanning, dependency-delta review, third-party license inventory, immutable GitHub Action pins and time-bounded exceptions. These controls supplement the existing ASan/LSan/UBSan/TSan and release-attestation gates; they do not replace them.

## Mandatory pull-request and push gate

The `security` job in `.github/workflows/ci.yml` is a dependency of the stable `ubuntu` aggregate. Both push and pull-request stable statuses therefore fail when the security job fails.

The job executes:

1. repository-local policy/negative tests,
2. GitHub CodeQL C/C++ analysis with the `security-extended` query suite and a manual compiler build,
3. `scripts/check_codeql_sarif.sh`, which rejects unexcepted findings with CodeQL `security-severity >= 7.0`,
4. Trivy filesystem scanning for vulnerabilities, secrets, misconfiguration and licenses,
5. `scripts/check_trivy_report.sh`, which rejects HIGH/CRITICAL vulnerabilities, any detected secret, HIGH/CRITICAL misconfiguration and HIGH/CRITICAL license findings,
6. a generated vulnerable `lodash@4.17.15` lockfile under `/tmp`, which Trivy must identify as a HIGH/CRITICAL vulnerable dependency,
7. GitHub dependency review on pull-request dependency deltas with HIGH severity and a redistribution license allowlist.

Scanner execution errors are CI failures. Trivy uses `exit-code: '0'` only to preserve JSON output for the repository-owned policy parser; the next mandatory step enforces the result and cannot be `continue-on-error`.

## Dependency and license scope

`security/third_party_inventory.tsv` distinguishes build tools, optional SDKs, linked runtime dependencies and vendored dependencies. A tool or SDK being named in the inventory is not a claim that its binaries are redistributed by ShortHand.

Build-only tools and optional SDK roots marked `redistributed=no` must still have an explicit version policy, known license and HTTPS source. Linked/vendored or redistributed dependencies must use a license in `security/license_allowlist.txt`; `LicenseRef-*`, unknown and unbounded versions are rejected for redistributed dependencies.

The repository's source SBOM remains useful source/release evidence, but it is not misrepresented as a complete package-version vulnerability inventory. Future vendored or package-managed dependencies must join dependency review and Trivy scanning. Optional AI SDKs that are absent from CI are not considered security-qualified by this PR; live optional-backend qualification remains Roadmap PR80.

## Exceptions

Security exceptions are exceptional, not a normal success path. Trivy exceptions live in `security/.trivyignore`; CodeQL exceptions live in `security/codeql_exceptions.tsv`.

Every active exception must identify a concrete finding/rule and path where applicable, an owner, a tracking ticket, a justification and an expiry date. Wildcards are rejected. Expiry must be after the current UTC date and no more than 90 days away. Expired or overlong exceptions fail CI. PR77 starts with zero active exceptions.

## Immutable CI dependencies

Every external GitHub Action used under `.github/workflows` must be pinned to an exact 40-hex commit SHA. Floating tags such as `@v6`, `@main` or branch refs fail `scripts/check_action_pinning.sh`. PR77 pins the existing CI/fuzz actions as well as CodeQL, Trivy and dependency review.

## Scheduled rescan

`.github/workflows/security.yml` reruns the repository Trivy policy on a daily schedule and by manual dispatch. This is additive detection for newly published CVEs; it does not publish the stable `ci / ubuntu (...)` contexts and therefore cannot overwrite PR/push qualification evidence.

## Negative evidence

`tests/security/test_security_policy_negative.sh` proves that prohibited redistributed licenses, unbounded versions, expired or wildcard exceptions, floating GitHub Actions, high-severity CodeQL SARIF, Trivy HIGH vulnerabilities and detected secrets all fail. The live CI additionally requires Trivy to detect the generated vulnerable dependency fixture.

## Claim boundary

Passing PR77 means the current repository source, declared dependency policy and CI dependency changes passed the configured external security gates at scan time. It does not prove the absence of vulnerabilities, validate SDKs that were not present, harden container/Kubernetes deployment, grant external certification or prove lower energy use than Python. Those remain later roadmap obligations.
