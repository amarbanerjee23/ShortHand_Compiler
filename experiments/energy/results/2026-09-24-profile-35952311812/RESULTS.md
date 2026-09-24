# Reviewed PR107 diagnostic capture

Source: [runtime-profile run 35952311812](https://github.com/amarbanerjee23/ShortHand_Compiler/actions/runs/35952311812).
All five cells are retained. Instrumented wall-clock attribution on one hosted machine; no energy or uninstrumented speedup claim.

| Batch | Threads | Preprocessing | Prepared call | Output validation | Top-k/postprocessing |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 1 | 1 | 4.52% | 90.34% | 1.22% | 3.93% |
| 16 | 1 | 20.18% | 63.83% | 1.36% | 14.63% |
| 16 | 2 | 20.68% | 63.00% | 1.35% | 14.97% |
| 16 | 4 | 21.14% | 62.99% | 1.35% | 14.52% |
| 32 | 1 | 27.85% | 51.94% | 1.51% | 18.71% |

The prepared call accounts for 51.94–90.34% of this instrumented boundary. It includes backend input validation, ONNX invocation, output allocation/copy and telemetry. This capture does not identify which of these dominates. The next v2 capture partitions those components, with explicit residual overhead.

Stage totals exclude dataset slicing, reference comparison and trial accumulation. Do not compare them directly with September 22 application timings. Warmed ordinary execution supplies the quality reference; all profiled outputs are checked against it.

PR head: `00cfc0d275d123ba4119a674acc7e4ff0209be2e`. The executable used GitHub’s PR merge test revision recorded in `revision.txt`, also present in every raw profile. PR106 and PR107 were merged into feature branches; their status alone did not put these changes on master.

Archive artifact ID: `10788789758`. Downloaded ZIP SHA-256: `b293315799f2beab926466f5021d6a237b62f89afb965c362ac498bdbf68ef9a`. The archive hash was checked before extraction. Retained raw profile bytes passed the versioned reporter’s work-count, quality, claim and timing-partition checks.

Replay each JSON profile with `profile_report.py --input FILE --input-sha256 DIGEST --output NEW_DIRECTORY`, using `SHA256SUMS.json` for the retained digests. CMake options, compiler, CPU and executable identity are retained alongside the profiles.
