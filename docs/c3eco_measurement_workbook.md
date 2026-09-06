# C3-ECO measurement, carbon accounting and cost workbook

contract: `shorthand.c3eco.measurement_workbook.v1`
status: candidate evidence contract
production_claim: false
official_certification_granted: false

## Purpose

PR89 separates declared/modelled budgets from auditable measured evidence. The native `shorthand_c3eco_measure` tool ingests instrument-backed energy records, validates allocation and provenance, applies PUE, carbon factors, tariffs and uncertainty, and emits deterministic CSV and JSON workbooks.

This contract does not score certification levels, grant certification, claim comparative energy superiority, or reduce the base footprint using offsets or avoided impact. Those concerns remain separate.

## Required input

The input is UTF-8 TSV with exactly these columns:

`record_id, component, source_kind, instrument_id, calibration_id, calibration_date, measured_at, raw_energy_j, allocation_fraction, pue, carbon_factor_gco2e_per_kwh, factor_source, factor_date, tariff_per_kwh, tariff_currency, tariff_source, uncertainty_percent, measurement_quality, data_quality, evidence_ref`

The supported measured source kinds are:

- `physical_meter`
- `rapl`
- `accelerator_counter`
- `cloud_meter`

`modelled`, `declared_budget_only`, inferred watts, and unavailable telemetry are not accepted as measured evidence.

## Accounting rules

For each record:

- allocated IT energy J = raw instrument energy J x allocation fraction
- facility energy J = allocated IT energy J x PUE
- facility energy kWh = facility energy J / 3,600,000
- carbon kgCO2e = facility energy kWh x carbon factor gCO2e/kWh / 1000
- cost = facility energy kWh x tariff per kWh
- uncertainty energy = facility energy kWh x uncertainty percent / 100
- uncertainty carbon = carbon kgCO2e x uncertainty percent / 100

PUE is bounded to `[1,3]`. Allocation must be in `(0,1]`. Shared readings are grouped by instrument, measurement timestamp, raw energy and evidence reference; their cumulative allocation must not exceed 1.0. This prevents a single raw meter reading from being counted more than once.

Carbon-factor and calibration dates must not post-date the measurement. Factor source, tariff source, calibration identity and evidence reference are mandatory. MQ and DQ are explicit `high`, `medium` or `low` values.

## Determinism and claim safety

Rows are sorted by `record_id` before output. Reordering equivalent input therefore produces byte-identical workbooks.

The emitted JSON always contains:

- `measurement_status: measured_instrumented`
- `official_certification_granted: false`
- `base_footprint_not_reduced_by_offsets: true`
- instrument and calibration provenance
- carbon-factor provenance
- tariff provenance
- explicit uncertainty

PR90 owns eligibility/scoring/claims. PR95 owns equivalent-workload ShortHand/Python performance and energy comparison. A PR89 workbook alone cannot support a statement that ShortHand consumes less energy than Python.
