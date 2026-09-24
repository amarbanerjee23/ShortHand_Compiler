# Validation of the retained results

The [capture workflow](https://github.com/amarbanerjee23/ShortHand_Compiler/actions/runs/35747601906)
completed successfully on commit `d3f1c6d3fc98357c44689504daae15204de56b3f`.
All five stages passed. Nine experiment unit tests and the labelled synthetic
meter collector test also passed; synthetic energy was not used in these results.

After download, verification on 23 September 2026 confirmed:

- The 31,251,602-byte ZIP matches the GitHub artifact SHA-256 in `artifact.json`.
- All files in all four complete evidence bundles match their capture manifests.
- Both source campaigns replay exactly, including predictions and checksums.
- Independent C++/ONNX replay passes all prediction, score and top-three checks.
- All 50 Python/ONNX process pairs pass prediction and numerical score checks.
- All 16 reported comparisons, including their confidence intervals, reproduce
  exactly from the saved 280 paired observations and 600 runtime inner trials.
- Every runtime accuracy is 89.37117417918754%, matching the source reference.
- No result qualifies measured energy evidence.

`observations.json` retains every process duration and inner-trial measurement
used in the comparisons. Per-batch latency diagnostics, raw predictions/scores,
stdout/stderr and executable artifacts remain in the complete capture ZIP.
The GitHub artifact expires on 21 December 2026; the committed summaries,
observations, plans and metadata remain in repository history.

The exporter was refined after capture to keep compact trial windows, give the
plain-text energy probe a `.txt` extension, and explain the observed regressions.
Its digest is recorded in `summary.json`. These presentation changes do not
alter measured observations or comparison calculations.

The VM reported four available logical CPUs and an AMD EPYC 7763 processor.
These results represent a hosted VM, not a dedicated 64-core machine. See
`metadata/lscpu.json` and the per-campaign environment files for full details.

Earlier attempts that exceeded AIRuntime's report-work limit are preserved in
[failed-runs](../failed-runs/README.md). Their observations are not pooled with
this session or substituted into these tables.
