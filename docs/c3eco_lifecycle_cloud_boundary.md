# C3-ECO lifecycle and cloud carbon boundary

contract: `shorthand.c3eco.lifecycle_cloud_boundary.v1`
status: candidate evidence contract
production_claim: false
comparative_energy_claim: false
official_certification_granted: false

PR100 closes the executable evidence gap for a declared lifecycle/cloud boundary. The native tool requires one attributable, provenance-linked record for data ingestion, storage, egress, model lifecycle, cloud compute and hardware lifecycle. It consumes references to PR89 instrumented workbooks; it does not estimate energy from capacity, utilisation, idle subtraction or an external cloud API.

The emitted JSON shape is fixed by `schemas/c3eco_lifecycle_cloud_boundary_v1.schema.json`.

Each row binds operational carbon to a `shared_resource_id`, allocation basis and allocation fraction. Allocations for the same resource may never exceed one. Data bytes, storage GB-hours and network GB are retained as material lifecycle activity rather than silently folded into compute. Hardware lifecycle records must identify an asset, embodied carbon, lifetime hours and bounded attributed usage; embodied carbon is allocated as `embodied × usage/lifetime × allocation`.

The output is deterministic and exposes the operational, embodied and total carbon allocations. It is intentionally candidate-only: factor provenance, workbook references and evidence references establish an auditable declared boundary, not independent authenticity, calibrated physical observation, certification, or a comparison with Python. Missing lifecycle phase, factor/workbook provenance, invalid hardware lifetime or shared-resource over-allocation fails closed.
