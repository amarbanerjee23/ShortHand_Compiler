# Continuous CI energy evidence

ShortHand records the strongest energy evidence that the current CI hardware can
support. The pipeline is deliberately fail-closed: inability to read a hardware
counter or match a validated calibration profile produces an explicit
"unavailable" result instead of a synthetic joule value.

## Evidence classes

| Class | Meaning | CI field |
| --- | --- | --- |
| E0 | externally calibrated whole-system physical energy | `physical_system_joules` |
| E1 | hardware-reported component energy | `hardware_measured_joules` |
| E2 | architecture-matched calibrated counter-model estimate | `calibrated_joules_estimate` |
| E3 | analytical operation/memory estimate | `analytical_joules_estimate` |

E1/E2/E3 must never be described as E0.

## E1 CPU sources

The Linux implementation probes two cumulative component-energy interfaces:

1. **powercap/RAPL** package domains via `energy_uj`.
2. **AMD `amd_energy` HWMON** socket domains via
   `energyN_input` whose `energyN_label` is `Esocket*`.

Core-level AMD `Ecore*` values are retained as diagnostic discovery metadata
but are not summed with socket values. This prevents hierarchical
double-counting.

Linux HWMON defines `energy[1-*]_input` as cumulative energy in microjoules:
https://docs.kernel.org/hwmon/sysfs-interface.html

The Linux AMD energy driver documents RAPL-backed core and socket counters
exposed through HWMON and labels socket energy as `EsocketX`:
https://docs.kernel.org/5.10/hwmon/amd_energy.html

Linux powercap documentation:
https://docs.kernel.org/power/powercap/powercap.html

E1 is component energy for the observed process window, not process-attributed
energy and not whole-host AC energy. The CI experiment therefore uses balanced
runner order and a separate instrumented pass rather than contaminating primary
latency observations.

## E2 calibrated counter models

A PMC model may output joules only when:

- the current CPU satisfies the profile's hardware contract;
- every model feature is available;
- coefficients were fitted/validated against retained measured-energy data;
- uncertainty and validation error are retained;
- the profile SHA-256 is emitted with every result.

The model form is intentionally bounded, for example:

```text
E =
  beta_time * elapsed_seconds
+ beta_instr * instructions
+ beta_cycles * cycles
+ beta_branch * branches
+ beta_cache * cache_misses
+ beta_mem * memory_bytes
+ intercept
```

The exact features and coefficients belong to the calibration profile.

A methodological reference for PMC-based, DVFS-aware energy modeling is:

S. Mazzola et al., "Data-driven power modeling and monitoring via hardware
performance counter tracking", Journal of Systems Architecture 167 (2025),
103504. DOI: https://doi.org/10.1016/j.sysarc.2025.103504

That work demonstrates that calibrated PMC models can estimate energy
accurately on their evaluated platform. Its coefficients are **not** reused for
unrelated ShortHand CI CPUs.

## E3 analytical model

The analytical fallback follows the Energy Roofline decomposition:

```text
E_analytical =
  sum(operation_count_i * joules_per_operation_i)
+ sum(bytes_at_level_j * joules_per_byte_j)
+ static_power_term * elapsed_time
```

Reference:

J. W. Choi, D. Bedard, R. J. Fowler and R. W. Vuduc,
"A Roofline Model of Energy", IEEE IPDPS 2013.
DOI: https://doi.org/10.1109/IPDPS.2013.77

The model explicitly motivates accounting for both computation and memory
communication. ShortHand therefore treats FLOP/MAC count alone as insufficient.

Mark Horowitz's ISSCC 2014 paper is retained as background for the importance of
data-movement energy:

M. Horowitz, "Computing's energy problem (and what we can do about it)",
ISSCC 2014. DOI: https://doi.org/10.1109/ISSCC.2014.6757323

Historical per-operation values from that work are **not** applied directly to
modern hosted CPUs.

## Forbidden substitutions

The continuous evidence pipeline must not derive "measured joules" from:

- TDP multiplied by elapsed time;
- CPU utilisation multiplied by TDP;
- latency reduction percentages;
- cloud-vCPU nominal power;
- a coefficient set calibrated on an incompatible CPU;
- a historical pJ/FLOP number applied without compatible calibration.

Such information may be retained as context but cannot populate E0/E1/E2.

## Comparison metric

For the same runner, model, dataset, precision, quality threshold, batch and
thread cell:

```text
R_energy = J_per_correct_task(ShortHand) / J_per_correct_task(baseline)

delta_percent = 100 * (1 - R_energy)
```

The evidence class, source/method, boundary and calibration identity must match
before a ratio is accepted.

## Hosted CI limitation

Virtualized GitHub runners may hide powercap/HWMON and restrict PMCs. In that
case E1 is unavailable. If no hardware-matched E2/E3 profile is checked in, the
correct result is `energy unavailable`, while latency/correctness evidence is
still retained.

This is a scientific constraint, not a CI failure.
