# Security Policy

ShortHand is currently suitable for controlled engineering pilots only. This policy defines the minimum security process for release-level branches.

## Supported scope

Security review applies to:

- compiler and parser code
- AI runtime abstraction code
- generated artifacts
- GreenAI evidence generation
- build and release scripts
- documentation that affects public claims

External AI SDKs, model files, datasets, and customer workloads are not vendored in this repository and must be reviewed separately before enterprise use.

## Reporting a vulnerability

Report suspected vulnerabilities privately to the repository owner or maintainer before publishing details. Include:

- affected commit or release branch
- reproduction steps
- expected and actual behavior
- whether the issue affects parsing, compilation, runtime execution, evidence output, generated code, or release packaging

## Response expectations

For controlled pilots, maintainers should:

1. acknowledge the report
2. reproduce the issue
3. classify severity
4. prepare a fix or mitigation
5. document the affected versions and safe workaround

## Security gates for release-level branches

A release-level branch must not remove existing validation gates. It should add or preserve:

- strict language validation
- smoke tests
- Makefile test suite
- sanitizer tests
- CMake build and CTest
- dependency and toolchain disclosure
- evidence-only claim policy

## Model and SDK safety

Model files and external SDKs must be treated as untrusted inputs unless they are verified, pinned, and sourced from a trusted channel. A runtime must not claim real inference when fallback behavior was used.

## Claim safety

Generated evidence reports must not claim external certification, carbon neutrality, or full industrial readiness unless an external review process and retained evidence support that claim.
