# PR90 consolidation checkpoint

The combined branch integrates the earlier PR90 head `13156742a5a6a73d48e464c5fa02e3543a37006e` with the hardened candidate `c76bbad06c2503c742c1145d778e658b6b83c4e8`. The existing PR90 commit history is retained, and the branch is updated only by fast-forward.

| Area | Consolidated behavior |
| --- | --- |
| Input validation | Structured, bounded JSON parsing; duplicate-key rejection; complete typed-profile links; identity/scope binding; record and aggregate workbook reconciliation. |
| Scoring | One engine with a separate native scoring core; complete 76-criterion catalog; all G1-G14 gates; evidence caps, weighted N/A treatment and penalty deductions. |
| Quality | MQ and DQ ceilings, independent-review requirements for upgrades, and conservative measured uncertainty. |
| Claims | Candidate-text safety; external authority required for certification; unsupported requests fail G10; comparative claims remain deferred to PR95. |
| Regressions | Complete materiality allocation, >10% eco-regression action/caps, quality-degradation failure and surveillance cadence. |
| Outputs | Deterministic JSON and optional Markdown with fixed non-certification and non-production boundaries. |
| Qualification | Real PR88/PR89 producers, independent native scoring tests, expanded end-to-end negatives, Make/CTest/sanitizer/packaging integration and all inherited gates. |

The earlier hosted head failed `Compiler test strategy and coverage audit` with `expected 35 compiler test coverage rows, found 34`. TST035 was present but lacked a final newline, so the shell record-reading loop omitted it. The consolidated inventory retains all 35 records with a terminating newline and keeps the strict gate intact.

The candidate-directory interface and [current assessment contract](c3eco_certification_assessment.md) replace the earlier draft five-path scorecard interface before PR90 is merged. No legacy assessment implementation is installed alongside it.

PR91 remains responsible for signed auditor evidence and retention. PR95 remains responsible for repeated equivalent-workload ShortHand-versus-Python energy qualification. Certification and production claims remain false.
