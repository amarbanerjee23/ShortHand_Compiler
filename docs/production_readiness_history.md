# Production readiness roadmap history

production_readiness_history_contract: immutable-milestone-anchors-v1
current_authoritative_plan: docs/production_readiness_pr_plan.md

This file preserves exact historical roadmap anchors that older CI task-stability gates protect. Active status, counts and sequencing live only in `docs/production_readiness_pr_plan.md`. Historical anchors below are audit evidence and must not be interpreted as current recommendations.

## Early foundation milestones

Language objectives consolidation applied in PR #57
Backend failure-mode finalization applied in PR #58
Runtime ABI and API stability applied in PR #59
Runtime state and thread-safety applied in PR #60
Production build packaging applied in PR #61
Prometheus scrape endpoint host adapter applied in PR #62

Recommended path from PR #51 onward: 29 PRs total.

PR59 - Runtime ABI and API version stability gate | MERGED
PR60 - Runtime state isolation and thread-safety policy | MERGED
PR61 - Production build packaging for runtime and AI bridge | MERGED
PR62 - Prometheus scrape endpoint host adapter | MERGED

remaining_planned_prs_after_pr61: 18
remaining_planned_prs_after_pr62: 17

Next recommended PR after PR #62:
PR63 - OTLP exporter adapter.

## Historical plan versions and estimates

production_readiness_plan_version: 2026-08-02-pr62
LAST_COMPLETED_PR: 62
After PR #62 is merged, approximately 17 implementation PRs remain.

production_readiness_plan_version: 2026-08-02-pr63
LAST_COMPLETED_PR: 63

After PR #65 is merged, approximately 14 implementation PRs remain.

production_readiness_plan_version: 2026-08-02-pr66
LAST_COMPLETED_PR: 66
PR66 - Full grammar and conformance matrix beta-0.2.
After PR #66 is merged, approximately 13 implementation PRs remain.

production_readiness_plan_version: 2026-08-02-pr67
LAST_COMPLETED_PR: 67
PR67 - Parser robustness and negative corpus hardening
After PR #67 is merged, approximately 12 implementation PRs remain.

production_readiness_plan_version: 2026-08-06-pr68
LAST_COMPLETED_PR: 68
PR68 - Module/import/package design and parser scaffold.

production_readiness_plan_version: 2026-08-06-pr69
LAST_COMPLETED_PR: 69

production_readiness_plan_version: 2026-08-09-pr70

Historical plan item: PR79 - MLIR lowering passes and production RC gate

The historical PR67 recommendation is superseded by the test re-audit.
