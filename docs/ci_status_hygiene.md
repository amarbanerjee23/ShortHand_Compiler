# CI status hygiene

ShortHand CI publishes two stable commit-status contexts:

- `ci / ubuntu (push)`
- `ci / ubuntu (pull_request)`

GitHub commit statuses are scoped to a commit SHA rather than a branch name. A newly created feature branch can initially point at the same SHA as the default branch. If that feature-branch run is superseded and cancelled, publishing a failure status from the cancelled run would overwrite the otherwise valid status on the shared SHA.

The CI workflow therefore keeps `cancel-in-progress: true` for efficient superseded-run handling, but the event-specific status publisher is guarded with `always() && !cancelled()`. A cancelled run does not publish a terminal custom status. Genuine completed failures still publish `failure`, and genuine completed successes still publish `success`.

The `scripts/check_ci_status_hygiene.sh` regression guard verifies the concurrency grouping, cancellation policy, target SHA expression, cancellation-safe publisher condition, and stable status context naming.

This policy must remain in place before feature work such as PR70 is resumed so branch creation or rapid follow-up pushes cannot contaminate the default-branch status for a shared commit SHA.
