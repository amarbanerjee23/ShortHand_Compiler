# C3-ECO conformance matrix

This matrix maps ShortHand production-readiness controls to the C3-ECO Green Software Certification draft used for PR #16 planning. The repository produces candidate evidence reports only; it does not issue certificates.

| C3-ECO area | Required control in ShortHand | Enforced by |
| --- | --- | --- |
| System identity | Product, version, owner, release date, software class, deployment mode, and certification scope must be declared. | `c3eco_gate`, `.greenai` examples |
| Functional unit | Functional unit and success criteria must be explicit and repeatable. | `c3eco_gate`, `green_ai_tool validate` |
| Boundary | Included and excluded layers must be listed. AI provider/deployer boundaries must be explicit. | `c3eco_gate`, candidate manifests |
| Measurement quality | MQ class must be present. Strict AI candidate manifests require MQ2 or higher. | `c3eco_gate` |
| Data quality | DQ class must be present. Strict AI candidate manifests require DQ2 or higher. | `c3eco_gate` |
| Carbon calculation | Carbon method, region, grid factor, and location-based footprint support must be present. | `green_ai_tool`, `c3eco_gate` |
| Offset separation | Offsets must never be allowed in the core score. | `green_ai_tool validate`, `c3eco_gate` |
| Security floor | Efficiency must not be achieved by weakening security. | `.greenai` required fields, production gate |
| Accessibility and safety floor | Efficiency must not be achieved by weakening accessibility, safety, privacy, reliability, or expected functionality. | `.greenai` required fields, production gate |
| Repeatability | Evidence and workload references must be retained. | `audit_evidence`, `evidence_retention_months`, tests |
| Claims integrity | Evidence report wording must avoid unsupported certification claims. | `production_readiness_gate.sh`, README policy |
| No quality degradation | Quality guardrails must remain true under the certified mode. | `success_criteria`, `no_quality_degradation` |
| Materiality | Individual material components and cumulative omissions must be controlled. | `materiality` block and `c3eco_gate` |
| AI necessity | Heavy AI use must be justified against smaller or non-AI alternatives where feasible. | `model_necessity_justification`, `smaller_model_evaluated` |
| Inference energy | Inference energy must be measured or transparently estimated for AI candidates. | `green_infer.measure_energy_per_inference`, budgets, measurements |
| Token efficiency | GenAI token budgets must be declared and checked. | `llm_task.green_llm`, budgets |
| RAG/vector cost | Retrieval top-k, embedding cache, chunk reuse, and reranking cost must be bounded. | `rag_pipeline.green_rag`, budgets |
| Model routing/cascade | Smaller or lower-energy model routing should be represented when used. | Candidate manifest fields and architecture policy |
| Evaluation energy | Evaluation and monitoring energy can be included in measurements when in boundary. | `green_ai_tool` energy fields |
| Cloud/shared allocation | Cloud region, runtime, target, and third-party API boundaries must be explicit. | `target`, `boundary`, `carbon` |
| Eco-regression | Resource and carbon regressions must be checked against a baseline. | `green_ai_tool check`, smoke tests |
| CI/CD efficiency | CI itself is treated as a measurable developer-tool workload. | CI gates and production-readiness script |

## Mandatory merge rule

A PR that changes AI, Green AI, compiler validation, evidence reporting, or release documentation should not be merged until all of the following succeed:

```bash
bash setup_build_infra.sh
bash scripts/validate_language.sh --strict
bash scripts/smoke_test.sh
make -C Compiler_new_ws/Short_Hand/src test
make -C Compiler_new_ws/Short_Hand/src sanitize
make -C Compiler_new_ws/Short_Hand/src test-c3eco
bash scripts/production_readiness_gate.sh
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
```

## Evidence-only status

The ShortHand tools may create evidence reports, conformance-candidate manifests, and regression checks. A final certificate still requires the external certification workflow, auditor competence, authorized certification body, scope approval, and registry process required by the applicable scheme owner.
