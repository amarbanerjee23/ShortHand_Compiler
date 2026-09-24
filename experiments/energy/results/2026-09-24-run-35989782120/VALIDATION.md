# Validation record

Capture: [workflow run 35989782120](https://github.com/amarbanerjee23/ShortHand_Compiler/actions/runs/35989782120)

The workflow completed all declared stages successfully on tested revision
`a8b646c0b3b38a39aa966bc692d031cf2af219e8` (the pull-request merge ref for
head `fcf0c88cc1ca3febb95e63d381fd44add22ad3ab`). The attached raw artifact is
identified in `artifact.json` and retained by GitHub through 2026-12-23.

| Check | Result |
| --- | --- |
| source-r10 | PASS |
| source-r100 | PASS |
| runtime-plan | PASS |
| python-runtime | PASS |
| cpp-runtime | PASS |
| physical-energy status | `physical_energy_measured: false` |
| raw artifact SHA-256 | `172ce9a76106c6c7d25aae4c96219988684bc1f605a483b0806437ece1f898a1` |

The compact export contains 18 comparison cells and 340 paired observations:
240 source pairs (four implemented baselines at both 10 and 100 repetitions)
and 100 runtime pairs (two baselines across five batch/thread cells). All
reported reductions are elapsed-time reductions only. There is no calibrated
whole-host AC trace, so this capture contains no joules-per-task or energy-
savings result.

`SHA256SUMS.json` covers every compact-export file except itself; the raw
artifact remains the replay source for process output, predictions, binaries,
LLVM IR and full manifests.
