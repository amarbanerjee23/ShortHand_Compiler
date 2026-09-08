# PR90 integration checkpoint

This branch implements `shorthand.c3eco.assessment.v1` on top of the merged PR88 typed profile and PR89 instrument-backed measurement workbook.

The assessment engine is native C++17 and has no Python runtime dependency. It evaluates G1-G14 eligibility, the complete 76-criterion A-K scorecard, evidence sufficiency, MQ/DQ and uncertainty level caps, approved N/A reallocation, materiality, AI-specific domain handling, eco-regression and controlled claims. Outputs remain certification-readiness evidence only and always preserve `official_certification_granted:false` and `production_claim:false`.

The authoritative qualification entry point is `scripts/check_c3eco_assessment.sh`. CMake/CTest and installation integration are present. Repository-governance, Make parity, packaging lifecycle and exact hosted-CI qualification are the remaining integration phase for this PR.

PR95 remains responsible for equivalent-workload ShortHand-versus-Python measured energy comparison. PR91 remains responsible for retained/signed auditor bundles, replay, surveillance and external authority workflow.
