# Frozen energy protocol: evidence integrity and scope

Version 2 repairs the experimental PR129 aggregation path. It preserves the
five-language integer-chain workload and adds no compiler/runtime change.
Historical v1 observations remain historical; they must not be relabelled v2.

## What this tool establishes

The runner retains whole-process timing, gross energy, signed idle-adjusted
energy, output hashes for every measured execution, executable/source/compiler
hashes, revision, qualification and idle observations. Failed measured executions
retain `failed-observation.json` and abort the run. Every variant must build and
produce the reference output. The comparison metric is **gross joules**; net
joules are diagnostic and may be negative. No zero clipping or selective dropping
of positive-energy rounds is used to obtain a favorable ratio.

Aggregation requires every frozen workload, variant and round. It recomputes idle
adjustments and summaries from observations and rejects changed summaries,
missing data, scaled runs, output mismatches and duplicate host identities.
A deterministic paired bootstrap gives descriptive gross-energy ratios.

**Replay is not sensor authentication.** The aggregate always retains
`energy_claim_authorized=false`, even if every descriptive interval is below one.
Independent provenance review is still required: host qualification and an
external command's printed numbers do not prove physical measurement. There is
no command-line override. Synthetic tests can replay but cannot authorize claims.
The runner does not yet retain authenticated sensor traces or a complete trusted
chain from compiler build inputs to binary; this limits public claim eligibility.

## Measurement boundaries

* RAPL is E1 hardware-reported package energy, not calibrated whole-system AC
  energy. Only package domains contribute; separate DRAM energy is excluded.
* An external meter command is labelled `external_meter_unvalidated`, not E0.
  Calibration, placement, resolution and synchronization need separate evidence.
* RAPL and AC readings are never required to be within an arbitrary percentage:
  their boundaries differ. A correlation model requires independent validation.
* This endpoint RAPL adapter rejects any interval that can wrap under a
  conservative 1 kW/package bound, even when the final counter is greater than
  the first. Long windows should use the existing sampled collector in
  `experiments/energy/ci_energy_evidence.py`. The 1 kW assumption is a guard,
  not a hardware rating or calibration; unsupported hardware needs a validated
  sampling bound before qualification.
* Measurement includes process startup and harness/sensor overhead. Sensors are
  read sequentially; their windows are not perfectly simultaneous. Do not use
  these results to claim isolated kernel energy or compare short sensor windows.
* A performance governor does not guarantee fixed frequency, stable temperature
  or isolation. Host facts are screening checks, not proof of a controlled lab.

## Usage

```bash
python3 experiments/energy/frozen_protocol/frozen_energy_protocol.py qualify --output qualification.json
python3 experiments/energy/frozen_protocol/frozen_energy_protocol.py run --output-dir results/host-A --cpu 2
python3 experiments/energy/frozen_protocol/frozen_energy_protocol.py aggregate --runs results/host-A/run.json --output aggregate.json
# Functional/time smoke run; unavailable energy stays null.
python3 experiments/energy/frozen_protocol/frozen_energy_protocol.py run --output-dir /tmp/dry --allow-unqualified --scale 0.05
python3 experiments/energy/frozen_protocol/test_frozen_energy_protocol.py
```

The chain workload is not representative of AI applications. The next comparison
campaign should extend existing held-out classification and batch inference,
using persistent Python/ORT and tuned C++/ORT with identical model, precision,
thread budget and quality. Native affine AOT needs its own same-work comparison.
Separate cold startup, warm execution and compilation amortization. Include
confidence intervals, losing cases and independent repetitions.

## Experimental PMU fitter

`pmu_energy_model.py` now solves the four-feature nonnegative ridge objective by
active-set enumeration. It rejects missing, boolean, negative and nonfinite
inputs. Profile hashes cover metadata as well as coefficients. In-sample fit
metrics do not establish calibration, and correlated coefficients are model
contributions rather than physical component attribution.

The CLI reports diagnostic model output separately and leaves `estimated_joules`
null. It does not produce an E2 profile. The existing
`ci_energy_evidence.py` E2/E3 framework remains the prediction integration point;
see `experiments/energy/calibration_profiles/README.md` for hardware matching,
raw calibration references, held-out validation and uncertainty requirements.
No compatible independently validated calibration was added by this change.

## No-meter strategy

Without an external meter, accessible package counters can support a scoped E1
result after independent provenance review. Without counters too, publish time
and resource metrics. A model may be validated on an independent dataset only
when its hardware, workloads, measurement boundary and operating conditions
justify transfer. Hold out workloads and sessions, retain prediction error and
reject extrapolation. Agreement between wrappers of the same RAPL counter is
not independent physical validation. Latency or TDP alone is not measured energy.
