#!/usr/bin/env python3
"""Green AI evidence tooling for ShortHand sustainability manifests.

The .greenai format is a tiny dependency-free sidecar DSL for audit evidence.
It intentionally complements executable .short programs instead of replacing the
C++ compiler grammar with certification metadata.
"""
from __future__ import annotations

import argparse
import json
import re
import sys
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Dict, List, Optional, Tuple

TOOL_VERSION = "0.3.0"
REPORT_SCHEMA_VERSION = "green-ai-evidence/v1"
CERTIFICATION_NOTE = "Evidence report only; this tool does not grant certification."

Token = Tuple[str, str, int]
TOKEN_RE = re.compile(
    r'''(?P<WS>[ \t\r\n]+)|(?P<COMMENT>\#.*)|(?P<STRING>"(?:\\.|[^"\\])*")|(?P<NUMBER>-?\d+(?:\.\d+)?)|(?P<IDENT>[A-Za-z_][A-Za-z0-9_\-\.]*)|(?P<PUNCT>[{}:\[\],])'''
)
MQ = {"MQ0", "MQ1", "MQ2", "MQ3", "MQ4"}
DQ = {"DQ0", "DQ1", "DQ2", "DQ3", "DQ4"}
STRICT_REQUIRED = [
    ("green", "functional_unit"),
    ("green", "success_criteria"),
    ("green", "boundary"),
    ("green", "measurement_quality"),
    ("green", "data_quality"),
    ("carbon", "accounting_method"),
    ("carbon", "region"),
    ("carbon", "grid_factor_kgco2e_per_kwh"),
    ("carbon", "offsets_allowed_in_core_score"),
]
BUDGET_KEYS = {
    "energy_per_inference_j_max": ("energy_per_inference_j", "J"),
    "energy_per_1000_inferences_wh_max": ("energy_per_1000_inferences_wh", "Wh"),
    "energy_per_1000_tokens_wh_max": ("energy_per_1000_tokens_wh", "Wh"),
    "training_energy_kwh_max": ("training_energy_kwh", "kWh"),
    "fine_tuning_energy_kwh_max": ("fine_tuning_energy_kwh", "kWh"),
    "evaluation_energy_kwh_max": ("evaluation_energy_kwh", "kWh"),
    "carbon_per_1000_inferences_gco2e_max": ("carbon_per_1000_inferences_gco2e", "gCO2e"),
    "carbon_per_1000_tokens_gco2e_max": ("carbon_per_1000_tokens_gco2e", "gCO2e"),
    "carbon_per_training_run_kgco2e_max": ("carbon_per_training_run_kgco2e", "kgCO2e"),
    "p95_latency_ms_max": ("p95_latency_ms", "ms"),
    "p99_latency_ms_max": ("p99_latency_ms", "ms"),
    "memory_peak_mb_max": ("memory_peak_mb", "MB"),
    "gpu_memory_peak_mb_max": ("gpu_memory_peak_mb", "MB"),
    "cpu_seconds_per_unit_max": ("cpu_seconds_per_unit", "s"),
    "network_mb_per_task_max": ("network_mb_per_task", "MB"),
    "storage_gb_month_max": ("storage_gb_month", "GB-month"),
    "token_budget_per_task_max": ("tokens_per_task", "tokens"),
    "retrieval_top_k_max": ("retrieval_top_k", "documents"),
    "cross_region_transfer_gb_max": ("cross_region_transfer_gb", "GB"),
    "update_payload_mb_max": ("update_payload_mb", "MB"),
    "idle_overhead_watts_max": ("idle_overhead_watts", "W"),
    "model_size_mb_max": ("model_size_mb", "MB"),
}
MEASUREMENT_FIELDS = {
    "functional_unit_count", "training_energy_kwh", "fine_tuning_energy_kwh",
    "evaluation_energy_kwh", "inference_energy_kwh", "idle_energy_kwh",
    "network_energy_kwh", "storage_energy_kwh", "client_energy_kwh",
    "ci_cd_energy_kwh", "update_energy_kwh", "third_party_ai_api_energy_kwh",
    "embodied_carbon_gco2e", "network_carbon_gco2e", "storage_carbon_gco2e",
    "third_party_ai_api_carbon_gco2e", "memory_peak_mb", "gpu_memory_peak_mb",
    "cpu_seconds_per_unit", "network_mb_per_task", "storage_gb_month",
    "tokens_per_task", "input_tokens_per_task", "output_tokens_per_task",
    "p50_latency_ms", "p95_latency_ms", "p99_latency_ms", "p95_latency_energy_j",
    "model_size_mb", "cache_hit_rate_percent", "small_model_routing_percent",
    "retrieval_top_k", "cross_region_transfer_gb", "update_payload_mb",
    "idle_overhead_watts",
}
REGRESSION_FIELDS = [
    "energy_per_functional_unit_kwh", "carbon_per_functional_unit_gco2e",
    "training_energy_kwh", "inference_energy_kwh", "memory_peak_mb",
    "gpu_memory_peak_mb", "network_mb_per_task", "tokens_per_task",
    "input_tokens_per_task", "output_tokens_per_task", "p95_latency_ms",
    "p95_latency_energy_j", "model_size_mb", "storage_gb_month",
    "cross_region_transfer_gb",
]
ENERGY_COMPONENTS = [
    "training_energy_kwh", "fine_tuning_energy_kwh", "evaluation_energy_kwh",
    "inference_energy_kwh", "idle_energy_kwh", "network_energy_kwh",
    "storage_energy_kwh", "client_energy_kwh", "ci_cd_energy_kwh",
    "update_energy_kwh", "third_party_ai_api_energy_kwh",
]
ADDITIVE_CARBON_COMPONENTS = [
    "embodied_carbon_gco2e", "network_carbon_gco2e", "storage_carbon_gco2e",
    "third_party_ai_api_carbon_gco2e",
]

@dataclass
class Block:
    kind: str
    name: Optional[str]
    body: Dict[str, Any]

class ParseError(ValueError):
    pass

class Parser:
    def __init__(self, text: str):
        self.text = text
        self.tokens = self.lex(text)
        self.pos = 0

    def location(self, offset: int) -> str:
        line = self.text.count("\n", 0, offset) + 1
        last_newline = self.text.rfind("\n", 0, offset)
        col = offset + 1 if last_newline < 0 else offset - last_newline
        return f"line {line}, column {col}, byte {offset}"

    def error(self, message: str, offset: int) -> ParseError:
        return ParseError(f"{message} at {self.location(offset)}")

    @staticmethod
    def lex(text: str) -> List[Token]:
        tokens: List[Token] = []
        idx = 0
        while idx < len(text):
            m = TOKEN_RE.match(text, idx)
            if not m:
                line = text.count("\n", 0, idx) + 1
                last_newline = text.rfind("\n", 0, idx)
                col = idx + 1 if last_newline < 0 else idx - last_newline
                raise ParseError(f"Unexpected character at line {line}, column {col}, byte {idx}: {text[idx:idx+20]!r}")
            kind = m.lastgroup or ""
            value = m.group(kind)
            if kind not in {"WS", "COMMENT"}:
                tokens.append((kind, value, idx))
            idx = m.end()
        tokens.append(("EOF", "", len(text)))
        return tokens

    def peek(self) -> Token:
        return self.tokens[self.pos]

    def advance(self) -> Token:
        tok = self.peek()
        self.pos += 1
        return tok

    def expect(self, value: str) -> Token:
        tok = self.advance()
        if tok[1] != value:
            raise self.error(f"Expected {value!r}, found {tok[1]!r}", tok[2])
        return tok

    def parse(self) -> List[Block]:
        blocks: List[Block] = []
        while self.peek()[0] != "EOF":
            blocks.append(self.parse_block())
        return blocks

    def parse_block(self) -> Block:
        header: List[str] = []
        while self.peek()[1] != "{":
            tok = self.advance()
            if tok[0] not in {"IDENT", "STRING"}:
                raise self.error(f"Expected block header, found {tok[1]!r}", tok[2])
            header.append(self.unquote(tok[1]))
        self.expect("{")
        body = self.parse_body()
        self.expect("}")
        kind = "_".join(header[:-1]) if len(header) > 1 else header[0]
        name = header[-1] if len(header) > 1 else None
        return Block(kind=kind, name=name, body=body)

    def parse_body(self) -> Dict[str, Any]:
        result: Dict[str, Any] = {}
        while self.peek()[1] != "}":
            key_tok = self.advance()
            if key_tok[0] != "IDENT":
                raise self.error(f"Expected key, found {key_tok[1]!r}", key_tok[2])
            key = key_tok[1]
            if self.peek()[1] == ":":
                self.advance()
                result[key] = self.parse_value()
            elif self.peek()[1] == "{":
                self.advance()
                result[key] = self.parse_body()
                self.expect("}")
            else:
                raise self.error(f"Expected ':' or '{{' after key {key!r}", self.peek()[2])
        return result

    def parse_value(self) -> Any:
        tok = self.advance()
        if tok[0] == "STRING":
            return self.unquote(tok[1])
        if tok[0] == "NUMBER":
            return float(tok[1]) if "." in tok[1] else int(tok[1])
        if tok[0] == "IDENT":
            if tok[1] == "true":
                return True
            if tok[1] == "false":
                return False
            return tok[1]
        if tok[1] == "[":
            values = []
            if self.peek()[1] != "]":
                while True:
                    values.append(self.parse_value())
                    if self.peek()[1] != ",":
                        break
                    self.advance()
            self.expect("]")
            return values
        if tok[1] == "{":
            body = self.parse_body()
            self.expect("}")
            return body
        raise self.error(f"Unexpected value {tok[1]!r}", tok[2])

    @staticmethod
    def unquote(value: str) -> str:
        if value.startswith('"'):
            return bytes(value[1:-1], "utf-8").decode("unicode_escape")
        return value

def blocks_by_kind(blocks: List[Block]) -> Dict[str, List[Block]]:
    by: Dict[str, List[Block]] = {}
    for b in blocks:
        by.setdefault(b.kind, []).append(b)
    return by

def first(by: Dict[str, List[Block]], kind: str) -> Dict[str, Any]:
    return by.get(kind, [Block(kind, None, {})])[0].body

def as_number(value: Any, default: Optional[float] = 0.0) -> Optional[float]:
    if value is None:
        return default
    if isinstance(value, (int, float)):
        return float(value)
    try:
        return float(value)
    except (TypeError, ValueError):
        return default

def parse_functional_unit_count(unit: Any) -> float:
    if isinstance(unit, (int, float)):
        return float(unit)
    if not isinstance(unit, str):
        return 1.0
    m = re.match(r"^(\d+(?:\.\d+)?)_", unit)
    return float(m.group(1)) if m else 1.0

def operational_carbon_gco2e(energy_kwh: float, grid_factor: float) -> float:
    return energy_kwh * grid_factor * 1000.0

def maybe_add(messages: List[str], mode: str, errors: List[str], warning: str) -> None:
    (errors if mode == "strict" else messages).append(warning)

def classify_mode(green: Dict[str, Any], strict_override: Optional[str]) -> str:
    return strict_override or green.get("green_mode", "advisory")

def validate(blocks: List[Block], strict_override: Optional[str] = None) -> Tuple[List[str], List[str]]:
    by = blocks_by_kind(blocks)
    green = first(by, "green")
    carbon = first(by, "carbon")
    budgets = first(by, "budgets")
    measurements = first(by, "measurements")
    warnings: List[str] = []
    errors: List[str] = []
    mode = classify_mode(green, strict_override)
    if mode not in {"off", "advisory", "strict"}:
        errors.append("green.green_mode must be one of off, advisory, strict")
        mode = "strict"
    if mode == "off":
        return warnings, errors

    for block_name, key in STRICT_REQUIRED:
        body = green if block_name == "green" else carbon
        if key not in body:
            maybe_add(warnings, mode, errors, f"missing required {block_name}.{key}")
    if not budgets:
        maybe_add(warnings, mode, errors, "missing required budgets block")
    if carbon.get("offsets_allowed_in_core_score") is not False:
        maybe_add(warnings, mode, errors, "carbon.offsets_allowed_in_core_score must be false; offsets cannot reduce core footprint")
    if carbon and carbon.get("accounting_method") not in {"location_based", "market_based"}:
        errors.append("carbon.accounting_method must be location_based or market_based")
    if "grid_factor_kgco2e_per_kwh" not in carbon:
        maybe_add(warnings, mode, errors, "missing carbon.grid_factor_kgco2e_per_kwh; carbon is not valid certification evidence")
    if green.get("measurement_quality") and green["measurement_quality"] not in MQ:
        errors.append("green.measurement_quality must be MQ0, MQ1, MQ2, MQ3, or MQ4")
    if green.get("data_quality") and green["data_quality"] not in DQ:
        errors.append("green.data_quality must be DQ0, DQ1, DQ2, DQ3, or DQ4")
    if mode == "strict" and green.get("measurement_quality") == "MQ0":
        errors.append("green.measurement_quality MQ0 is not sufficient for strict certification-readiness evidence")
    if mode == "strict" and green.get("data_quality") == "DQ0":
        errors.append("green.data_quality DQ0 is not sufficient for strict certification-readiness evidence")
    if green.get("measurement_quality") == "MQ3":
        if not measurements.get("repeated_runs") or not measurements.get("idle_baseline_watts"):
            maybe_add(warnings, mode, errors, "MQ3 requires measurements.repeated_runs and measurements.idle_baseline_watts")
    if green.get("measurement_quality") == "MQ4" and not green.get("audit_evidence"):
        maybe_add(warnings, mode, errors, "MQ4 requires green.audit_evidence or independent verification reference")

    for key, value in budgets.items():
        if key not in BUDGET_KEYS:
            warnings.append(f"unknown budget key budgets.{key}")
            continue
        if as_number(value, None) is None or as_number(value, 0.0) <= 0:
            errors.append(f"budget {key} must be a positive number")
    for key in measurements:
        if key not in MEASUREMENT_FIELDS:
            warnings.append(f"unknown measurements key measurements.{key}")

    for block in by.get("train_model", []):
        if "green_train" not in block.body:
            maybe_add(warnings, mode, errors, f"train_model {block.name} missing green_train block")
    for block in by.get("infer_endpoint", []):
        if "green_infer" not in block.body:
            maybe_add(warnings, mode, errors, f"infer_endpoint {block.name} missing green_infer block")
    for block in by.get("llm_task", []):
        green_llm = block.body.get("green_llm", {})
        if "max_input_tokens" not in block.body or "max_output_tokens" not in block.body:
            maybe_add(warnings, mode, errors, f"llm_task {block.name} missing max_input_tokens or max_output_tokens")
        if "token_budget_per_task" not in green_llm:
            maybe_add(warnings, mode, errors, f"llm_task {block.name} missing green_llm.token_budget_per_task")
    for block in by.get("rag_pipeline", []):
        green_rag = block.body.get("green_rag", {})
        if "retrieve_top_k_max" not in green_rag:
            maybe_add(warnings, mode, errors, f"rag_pipeline {block.name} missing green_rag.retrieve_top_k_max")
        if not green_rag.get("embedding_cache_enabled") and not green_rag.get("embedding_cache_disabled_reason"):
            maybe_add(warnings, mode, errors, f"rag_pipeline {block.name} missing embedding cache or disabled reason")
    if by.get("model") and not by.get("target"):
        maybe_add(warnings, mode, errors, "model declared without deployment target")
    for block in by.get("model", []):
        green_model = block.body.get("green_model", {})
        size = as_number(green_model.get("model_size_mb", block.body.get("model_size_mb")), 0.0) or 0.0
        params = as_number(green_model.get("parameters_count"), 0.0) or 0.0
        large = size >= 500 or params >= 100_000_000
        if large and not green_model.get("model_necessity_justification"):
            maybe_add(warnings, mode, errors, f"model {block.name} is large but missing green_model.model_necessity_justification")
        if large and green_model.get("smaller_model_evaluated") is not True:
            warnings.append(f"model {block.name} is large; document smaller_model_evaluated: true when available")
    return warnings, errors

def budget_actuals(measurements: Dict[str, Any], derived: Dict[str, Any]) -> Dict[str, Any]:
    actuals = dict(measurements)
    unit_count = as_number(derived.get("functional_unit_count"), 1.0) or 1.0
    inference_kwh = as_number(derived.get("inference_energy_kwh"), 0.0) or 0.0
    actuals.update(derived)
    actuals["energy_per_inference_j"] = (inference_kwh * 3_600_000.0 / unit_count) if unit_count else None
    actuals["energy_per_1000_inferences_wh"] = (inference_kwh * 1000.0 / unit_count * 1000.0) if unit_count else None
    actuals["energy_per_1000_tokens_wh"] = (inference_kwh * 1000.0 / unit_count * 1000.0) if unit_count else None
    actuals["carbon_per_1000_inferences_gco2e"] = derived.get("carbon_per_functional_unit_gco2e")
    actuals["carbon_per_1000_tokens_gco2e"] = derived.get("carbon_per_functional_unit_gco2e")
    actuals["carbon_per_training_run_kgco2e"] = (as_number(derived.get("total_carbon_gco2e"), 0.0) or 0.0) / 1000.0
    return actuals

def evaluate_budgets(budgets: Dict[str, Any], measurements: Dict[str, Any], derived: Dict[str, Any], warnings: List[str]) -> List[Dict[str, Any]]:
    actuals = budget_actuals(measurements, derived)
    results = []
    for budget, limit in budgets.items():
        if budget not in BUDGET_KEYS:
            results.append({"budget": budget, "limit": limit, "actual": None, "unit": None, "status": "unknown"})
            continue
        actual_key, unit = BUDGET_KEYS[budget]
        actual = actuals.get(actual_key)
        numeric_limit = as_number(limit, None)
        if actual is None:
            status = "not_evaluated"
        elif numeric_limit is None:
            status = "unknown"
        else:
            status = "pass" if float(actual) <= numeric_limit else "fail"
        results.append({"budget": budget, "limit": limit, "actual": actual, "unit": unit, "status": status})
    supported = {r["budget"] for r in results}
    for required in ["energy_per_inference_j_max", "training_energy_kwh_max", "memory_peak_mb_max"]:
        if required not in supported:
            warnings.append(f"budget {required} not declared; related guardrail is not evaluated")
    return results

def component_carbon(measurements: Dict[str, Any], grid_factor: Optional[float]) -> Tuple[Dict[str, Any], Optional[float], float]:
    energy_components = {k: as_number(measurements.get(k), 0.0) or 0.0 for k in ENERGY_COMPONENTS}
    total_energy = sum(energy_components.values())
    op_carbon = operational_carbon_gco2e(total_energy, grid_factor) if grid_factor is not None else None
    additive = {k: as_number(measurements.get(k), 0.0) or 0.0 for k in ADDITIVE_CARBON_COMPONENTS}
    return {"energy_kwh": energy_components, "operational_carbon_gco2e": op_carbon, "additive_carbon_gco2e": additive}, op_carbon, total_energy

def build_report(path: Path, blocks: List[Block], strict: Optional[str]) -> Dict[str, Any]:
    by = blocks_by_kind(blocks)
    warnings, errors = validate(blocks, strict)
    green = first(by, "green")
    carbon = first(by, "carbon")
    budgets = first(by, "budgets")
    measurements = first(by, "measurements")
    mode = classify_mode(green, strict)
    unit = green.get("functional_unit", "1_unit")
    unit_count = float(measurements.get("functional_unit_count", parse_functional_unit_count(unit)))
    grid_factor = as_number(carbon.get("grid_factor_kgco2e_per_kwh"), None)
    component, operational_carbon, total_kwh = component_carbon(measurements, grid_factor)
    additive_total = sum(component["additive_carbon_gco2e"].values())
    total_carbon = (operational_carbon + additive_total) if operational_carbon is not None else None
    per_unit_kwh = total_kwh / unit_count if unit_count else None
    per_unit_carbon = total_carbon / unit_count if total_carbon is not None and unit_count else None
    market_factor = as_number(carbon.get("market_based_grid_factor_kgco2e_per_kwh"), None)
    market_carbon = operational_carbon_gco2e(total_kwh, market_factor) if market_factor is not None else None
    derived = {
        "functional_unit_count": unit_count,
        "training_energy_kwh": component["energy_kwh"].get("training_energy_kwh", 0.0),
        "fine_tuning_energy_kwh": component["energy_kwh"].get("fine_tuning_energy_kwh", 0.0),
        "evaluation_energy_kwh": component["energy_kwh"].get("evaluation_energy_kwh", 0.0),
        "inference_energy_kwh": component["energy_kwh"].get("inference_energy_kwh", 0.0),
        "idle_energy_kwh": component["energy_kwh"].get("idle_energy_kwh", 0.0),
        "network_energy_kwh": component["energy_kwh"].get("network_energy_kwh", 0.0),
        "storage_energy_kwh": component["energy_kwh"].get("storage_energy_kwh", 0.0),
        "total_operational_energy_kwh": total_kwh,
        "operational_carbon_gco2e": operational_carbon,
        "total_carbon_gco2e": total_carbon,
        "energy_per_functional_unit_kwh": per_unit_kwh,
        "carbon_per_functional_unit_gco2e": per_unit_carbon,
        "memory_peak_mb": measurements.get("memory_peak_mb"),
        "gpu_memory_peak_mb": measurements.get("gpu_memory_peak_mb"),
        "cpu_seconds_per_unit": measurements.get("cpu_seconds_per_unit"),
        "network_mb_per_task": measurements.get("network_mb_per_task"),
        "storage_gb_month": measurements.get("storage_gb_month"),
        "tokens_per_task": measurements.get("tokens_per_task"),
        "input_tokens_per_task": measurements.get("input_tokens_per_task"),
        "output_tokens_per_task": measurements.get("output_tokens_per_task"),
        "p95_latency_ms": measurements.get("p95_latency_ms"),
        "p95_latency_energy_j": measurements.get("p95_latency_energy_j"),
        "model_size_mb": measurements.get("model_size_mb"),
        "cross_region_transfer_gb": measurements.get("cross_region_transfer_gb"),
        "retrieval_top_k": measurements.get("retrieval_top_k"),
    }
    budget_results = evaluate_budgets(budgets, measurements, derived, warnings)
    unsupported = [r["budget"] for r in budget_results if r["status"] in {"not_evaluated", "unknown"}]
    report = {
        "report_schema_version": REPORT_SCHEMA_VERSION,
        "generated_at": datetime.now(timezone.utc).isoformat(),
        "tool_version": TOOL_VERSION,
        "product": green.get("product", path.stem),
        "dsl_file": str(path),
        "green_mode": mode,
        "certification_profile": green.get("certification_profile"),
        "functional_unit": unit,
        "functional_unit_count": unit_count,
        "success_criteria": green.get("success_criteria", {}),
        "boundary": green.get("boundary", {}),
        "carbon": carbon,
        "measurement_quality": green.get("measurement_quality", "MQ1" if total_kwh else "MQ0"),
        "data_quality": green.get("data_quality", "DQ1" if grid_factor is not None else "DQ0"),
        "budgets": budgets,
        "budget_results": budget_results,
        "models": [dict(kind=b.kind, name=b.name, **b.body) for b in by.get("model", [])],
        "training": [dict(kind=b.kind, name=b.name, **b.body) for b in by.get("train_model", [])],
        "inference": [dict(kind=b.kind, name=b.name, **b.body) for b in by.get("infer_endpoint", [])],
        "llm_tasks": [dict(kind=b.kind, name=b.name, **b.body) for b in by.get("llm_task", [])],
        "rag_pipelines": [dict(kind=b.kind, name=b.name, **b.body) for b in by.get("rag_pipeline", [])],
        "model_routes": [dict(kind=b.kind, name=b.name, **b.body) for b in by.get("model_route", [])],
        "targets": [dict(kind=b.kind, name=b.name, **b.body) for b in by.get("target", [])],
        "data_pipelines": [dict(kind=b.kind, name=b.name, **b.body) for b in by.get("data_pipeline", [])],
        "measurements": measurements,
        "derived": derived,
        "component_carbon": component,
        "location_based_carbon_gco2e": operational_carbon if carbon.get("accounting_method") == "location_based" else None,
        "market_based_carbon_gco2e": market_carbon,
        "offsets_reported_separately": carbon.get("offsets_gco2e", 0),
        "warnings": warnings,
        "errors": errors,
        "assumptions": green.get("assumptions", []),
        "unsupported_or_unmeasured_components": unsupported,
        "audit_evidence_references": green.get("audit_evidence", []),
        "certification_note": CERTIFICATION_NOTE,
    }
    return report

def load_manifest(path: Path) -> List[Block]:
    return Parser(path.read_text()).parse()

def load_current(path: Path, strict: Optional[str]) -> Dict[str, Any]:
    if path.suffix == ".json":
        return json.loads(path.read_text())
    return build_report(path, load_manifest(path), strict)

def remediation_hint(field: str) -> str:
    if "carbon" in field:
        return "review energy sources, grid factor, region, and additive carbon components"
    if "energy" in field:
        return "profile workload, reduce idle time, batch efficiently, and consider smaller or compressed models"
    if "token" in field:
        return "tighten prompts, retrieval context, cache repeated prompts, or route to smaller models"
    if "memory" in field:
        return "reduce batch size, quantize, prune, or stream data"
    if "network" in field or "cross_region" in field:
        return "co-locate data/model serving and reduce transfer volume"
    if "model_size" in field:
        return "document model necessity or evaluate quantization/distillation/pruning"
    return "investigate regression and document mitigation"

def cmd_validate(args: argparse.Namespace) -> int:
    warnings, errors = validate(load_manifest(Path(args.file)), args.strict)
    for w in warnings:
        print(f"warning: {w}", file=sys.stderr)
    for e in errors:
        print(f"error: {e}", file=sys.stderr)
    if errors:
        return 1
    print("Green AI manifest validation passed")
    return 0

def cmd_report(args: argparse.Namespace) -> int:
    path = Path(args.file)
    report = build_report(path, load_manifest(path), args.strict)
    output = json.dumps(report, indent=2, sort_keys=True) + "\n"
    if args.output:
        Path(args.output).write_text(output)
    else:
        print(output, end="")
    return 1 if report["errors"] else 0

def cmd_check(args: argparse.Namespace) -> int:
    current = load_current(Path(args.file), args.strict)
    baseline = json.loads(Path(args.baseline).read_text())
    threshold = args.threshold_percent / 100.0
    regressions = []
    cur_d = current.get("derived", {})
    base_d = baseline.get("derived", {})
    for field in REGRESSION_FIELDS:
        cur = cur_d.get(field)
        base = base_d.get(field)
        if cur is None or base in (None, 0):
            continue
        change = (float(cur) - float(base)) / float(base) * 100.0
        if change > args.threshold_percent:
            regressions.append({
                "field": field,
                "baseline": base,
                "current": cur,
                "change_percent": change,
                "threshold_percent": args.threshold_percent,
                "status": "fail",
                "remediation_hint": remediation_hint(field),
            })
    mode = args.strict or current.get("green_mode", "advisory")
    passed = not current.get("errors") and not regressions
    result = {"passed": passed, "mode": mode, "regressions": regressions, "errors": current.get("errors", []), "warnings": current.get("warnings", [])}
    output = json.dumps(result, indent=2, sort_keys=True) + "\n"
    if args.output:
        Path(args.output).write_text(output)
    else:
        print(output, end="")
    return 0 if passed else 1

def main(argv: Optional[List[str]] = None) -> int:
    ap = argparse.ArgumentParser(description="Validate and report ShortHand Green AI evidence manifests")
    sub = ap.add_subparsers(required=True)
    for name, fn in [("validate", cmd_validate), ("report", cmd_report)]:
        sp = sub.add_parser(name)
        sp.add_argument("file")
        sp.add_argument("--strict", choices=["off", "advisory", "strict"])
        if name == "report":
            sp.add_argument("--output")
        sp.set_defaults(func=fn)
    sp = sub.add_parser("check")
    sp.add_argument("file", help="current .greenai manifest or previously generated report JSON")
    sp.add_argument("--baseline", required=True)
    sp.add_argument("--threshold-percent", type=float, default=10.0)
    sp.add_argument("--strict", choices=["off", "advisory", "strict"])
    sp.add_argument("--output")
    sp.set_defaults(func=cmd_check)
    args = ap.parse_args(argv)
    return args.func(args)

if __name__ == "__main__":
    raise SystemExit(main())
