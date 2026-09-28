# CI energy calibration profiles

This directory contains **checked, versioned energy models**, not generic power
guesses. The CI evidence engine ignores malformed profiles and refuses to apply
a profile whose hardware contract does not match the current runner.

No default CPU calibration profile is checked in by PR111. This is intentional:
a historical pJ/FLOP value, TDP, cloud-vCPU estimate, or coefficient fitted on a
different CPU is not silently promoted to joules on GitHub-hosted hardware.

## Evidence classes

- **E2** profiles are fitted/validated against measured energy and must set
  `"validated_calibration": true`.
- **E3** profiles are analytical operation/memory models. They remain estimates
  even when their coefficients come from a publication.
- E0/E1 are measurements and therefore are not represented by model profiles.

## Required profile shape

```json
{
  "schema": "shorthand.energy.calibration_profile.v1",
  "profile_id": "example-cpu-pmc-v1",
  "evidence_class": "E2",
  "validated_calibration": true,
  "hardware_match": {
    "vendor_id": "GenuineIntel",
    "cpu_family": "6",
    "model": "143",
    "machine": "x86_64",
    "model_name_regex": "Xeon"
  },
  "coefficients": {
    "elapsed_seconds": 0.0,
    "instructions": 0.0,
    "cycles": 0.0,
    "cache_misses": 0.0,
    "intercept_joules": 0.0
  },
  "uncertainty_percent": 5.0,
  "source": "retained calibration dataset or peer-reviewed model",
  "calibration_provenance": "location/hash/citation for calibration and validation evidence"
}
```

Supported feature names are intentionally bounded in
`experiments/energy/ci_energy_evidence.py`. A model requiring a new feature
must update code and tests rather than smuggling arbitrary expressions into a
profile.

## Acceptance requirements for an E2 profile

A production E2 profile must retain:

1. exact CPU identity or a justified validated hardware class;
2. frequency/DVFS policy used for calibration;
3. the raw calibration observations or immutable reference;
4. measured joules source (for example package RAPL or an external calibrated
   instrument);
5. train/validation split or equivalent out-of-sample validation;
6. fit error and the uncertainty reported by this profile;
7. coefficient units and feature definitions;
8. tool/version identity and profile SHA-256.

Calibration error must be reported with every estimated value. A profile may
never authorize a comparative energy, carbon, certification, or
"lowest-energy" claim on its own.

## E3 analytical profiles

E3 may encode published or independently derived energy-per-operation and
energy-per-byte coefficients, but the profile must state the hardware/process
scope and uncertainty. Published historical values such as 45 nm reference
figures are useful scientific context; they are not valid calibration for an
unrelated modern GitHub runner.

If no compatible profile is available, CI records the absence and continues
with correctness/latency evidence. It does not invent joules.
