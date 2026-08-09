# CI status hygiene test contract

The executable guard is `scripts/check_ci_status_hygiene.sh` and is run by the normal Ubuntu CI workflow before compiler build infrastructure is initialized.

It fails if any of the following regress:

1. push and pull-request runs no longer use event/ref-isolated concurrency groups;
2. superseded runs stop using `cancel-in-progress: true`;
3. the custom status publisher becomes unconditional after workflow cancellation;
4. pull-request statuses stop targeting the pull-request head SHA;
5. the stable `ci / ubuntu (...)` context naming changes unexpectedly.

This is a CI-policy regression test, not a replacement for compiler, sanitizer, conformance, CMake, CTest, or enterprise-hardening checks.
