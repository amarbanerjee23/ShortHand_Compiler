# Retained failed captures

These captures are diagnostic evidence. Their source stages completed, but their
runtime stages failed, so they are not the final comparison session. No timing
outliers were removed and no unsuccessful capture was overwritten.

| Run | Tested commit | Source stages | Runtime failure |
| --- | --- | --- | --- |
| [35746158570](https://github.com/amarbanerjee23/ShortHand_Compiler/actions/runs/35746158570) | `90952fa79209f02e2996a0769017db344657d921` | Both passed | `application_report_size_limit` at batch 1, 20 repetitions, 3 trials |
| [35746430266](https://github.com/amarbanerjee23/ShortHand_Compiler/actions/runs/35746430266) | `f21ee4dc3fccf2051cc2d9f511f51bdd7d9de7d7` | Both passed | Same pre-existing application budget; this revision additionally aligned C++ ONNX warmups and verified scores/top-three labels |

The second capture was launched before the first capture returned results, to
correct the C++ ONNX methodology. After the report-budget failure was observed,
the next plan reduced runtime repetitions from 20 to 10 in every cell:
`1797 × 10 × 3 = 53,910` batches in the worst-case batch-1 cell, below 100,000.
The workload, dataset, session options, pair counts and correctness checks stayed
the same. Plan preparation now rejects oversized work before execution.

Each directory retains the stage index, exact failure traces, all successful
source pairs, source summaries, environment, frozen source plans and manifests.
`artifact.json` identifies the full original capture ZIP and its SHA-256. These
artifact archives are also available from their workflow runs until expiration.
