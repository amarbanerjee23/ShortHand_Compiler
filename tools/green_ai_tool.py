#!/usr/bin/env python3
"""Green AI evidence tooling for ShortHand sustainability manifests.

This parser intentionally supports a small, auditable DSL rather than YAML so the
syntax can mirror the requested C3-ECO-style blocks while staying dependency-free.
"""
from __future__ import annotations

import argparse
import json
import re
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Dict, List, Optional, Tuple

Token = Tuple[str, str, int]

TOKEN_RE = re.compile(
    r'''(?P<WS>[ \t\r\n]+)|(?P<COMMENT>\#.*)|(?P<STRING>"(?:\\.|[^"\\])*")|(?P<NUMBER>-?\d+(?:\.\d+)?)|(?P<IDENT>[A-Za-z_][A-Za-z0-9_\-\.]*)|(?P<PUNCT>[{}:\[\],])'''
)

MQ = {"MQ0", "MQ1", "MQ2", "MQ3", "MQ4"}
DQ = {"DQ0", "DQ1", "DQ2", "DQ3", "DQ4"}
STRICT_REQUIRED = [
    ("green", "functional_unit"),
    ("green", "boundary"),
    ("green", "measurement_quality"),
    ("green", "data_quality"),
    ("carbon", "region"),
    ("carbon", "grid_factor_kgco2e_per_kwh"),
]
BUDGET_KEYS = {
    "energy_per_inference_j_max",
    "energy_per_1000_tokens_wh_max",
    "training_energy_kwh_max",
    "carbon_per_1000_inferences_gco2e_max",
    "memory_peak_mb_max",
    "network_mb_per_task_max",
}
REGRESSION_FIELDS = [
    "energy_per_functional_unit_kwh",
    "carbon_per_functional_unit_gco2e",
    "memory_peak_mb",
    "network_mb_per_task",
    "tokens_per_task",
    "p95_latency_energy_j",
    "model_size_mb",
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
        self.tokens = self.lex(text)
        self.pos = 0

    @staticmethod
    def lex(text: str) -> List[Token]:
        tokens: List[Token] = []
        idx = 0
        while idx < len(text):
            m = TOKEN_RE.match(text, idx)
            if not m:
                raise ParseError(f"Unexpected character at byte {idx}: {text[idx:idx+20]!r}")
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
            raise ParseError(f"Expected {value!r} at byte {tok[2]}, found {tok[1]!r}")
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
                raise ParseError(f"Expected block header at byte {tok[2]}, found {tok[1]!r}")
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
                raise ParseError(f"Expected key at byte {key_tok[2]}, found {key_tok[1]!r}")
            key = key_tok[1]
            if self.peek()[1] == ":":
                self.advance()
                result[key] = self.parse_value()
            elif self.peek()[1] == "{":
                self.advance()
                result[key] = self.parse_body()
                self.expect("}")
            else:
                raise ParseError(f"Expected ':' or '{{' after key {key!r} at byte {self.peek()[2]}")
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
        raise ParseError(f"Unexpected value at byte {tok[2]}: {tok[1]!r}")

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

def parse_functional_unit_count(unit: Any) -> float:
    if isinstance(unit, (int, float)):
        return float(unit)
    if not isinstance(unit, str):
        return 1.0
    m = re.match(r"^(\d+(?:\.\d+)?)_", unit)
    return float(m.group(1)) if m else 1.0

def operational_carbon_gco2e(energy_kwh: float, grid_factor: float) -> float:
    return energy_kwh * grid_factor * 1000.0

def validate(blocks: List[Block], strict_override: Optional[str] = None) -> Tuple[List[str], List[str]]:
    by = blocks_by_kind(blocks)
    green = first(by, "green")
    carbon = first(by, "carbon")
    budgets = first(by, "budgets")
    warnings: List[str] = []
    errors: List[str] = []
    mode = strict_override or green.get("green_mode", "advisory")
    if mode not in {"off", "advisory", "strict"}:
        errors.append("green.green_mode must be one of off, advisory, strict")
    if mode == "off":
        return warnings, errors
    for block_name, key in STRICT_REQUIRED:
        body = green if block_name == "green" else carbon
        if key not in body:
            msg = f"missing required {block_name}.{key}"
            (errors if mode == "strict" else warnings).append(msg)
    if green.get("measurement_quality") and green["measurement_quality"] not in MQ:
        errors.append("green.measurement_quality must be MQ0, MQ1, MQ2, MQ3, or MQ4")
    if green.get("data_quality") and green["data_quality"] not in DQ:
        errors.append("green.data_quality must be DQ0, DQ1, DQ2, DQ3, or DQ4")
    if carbon.get("offsets_allowed_in_core_score") is True:
        errors.append("carbon.offsets_allowed_in_core_score must be false; offsets cannot reduce core footprint")
    if carbon and carbon.get("accounting_method") not in {None, "location_based", "market_based"}:
        errors.append("carbon.accounting_method must be location_based or market_based")
    for key in budgets:
        if key not in BUDGET_KEYS:
            warnings.append(f"unknown budget key budgets.{key}")
    for block in by.get("llm_task", []):
        green_llm = block.body.get("green_llm", {})
        if green_llm and "token_budget_per_task" not in green_llm:
            (errors if mode == "strict" else warnings).append(f"llm_task {block.name} missing green_llm.token_budget_per_task")
        if green_llm and green_llm.get("retrieve_top_k_max") is None and "rag" in str(block.body).lower():
            warnings.append(f"llm_task {block.name} may be RAG but has no retrieve_top_k_max")
    for block in by.get("train_model", []):
        if "green_train" not in block.body:
            (errors if mode == "strict" else warnings).append(f"train_model {block.name} missing green_train block")
    for block in by.get("infer_endpoint", []):
        if "green_infer" not in block.body:
            (errors if mode == "strict" else warnings).append(f"infer_endpoint {block.name} missing green_infer block")
    if by.get("model") and not by.get("target"):
        (errors if mode == "strict" else warnings).append("model declared without deployment target")
    return warnings, errors

def build_report(path: Path, blocks: List[Block], strict: Optional[str]) -> Dict[str, Any]:
    by = blocks_by_kind(blocks)
    warnings, errors = validate(blocks, strict)
    green = first(by, "green")
    carbon = first(by, "carbon")
    budgets = first(by, "budgets")
    measurements = first(by, "measurements")
    unit = green.get("functional_unit", "1_unit")
    unit_count = float(measurements.get("functional_unit_count", parse_functional_unit_count(unit)))
    training_kwh = float(measurements.get("training_energy_kwh", 0.0))
    inference_kwh = float(measurements.get("inference_energy_kwh", 0.0))
    idle_kwh = float(measurements.get("idle_energy_kwh", 0.0))
    network_kwh = float(measurements.get("network_energy_kwh", 0.0))
    storage_kwh = float(measurements.get("storage_energy_kwh", 0.0))
    total_kwh = training_kwh + inference_kwh + idle_kwh + network_kwh + storage_kwh
    grid = float(carbon.get("grid_factor_kgco2e_per_kwh", 0.0))
    total_carbon = operational_carbon_gco2e(total_kwh, grid)
    per_unit_kwh = total_kwh / unit_count if unit_count else 0.0
    per_unit_carbon = total_carbon / unit_count if unit_count else 0.0
    budget_status = {
        "training_energy_kwh_max": training_kwh <= float(budgets.get("training_energy_kwh_max", float("inf"))),
        "carbon_per_1000_inferences_gco2e_max": per_unit_carbon <= float(budgets.get("carbon_per_1000_inferences_gco2e_max", float("inf"))),
        "memory_peak_mb_max": float(measurements.get("memory_peak_mb", 0.0)) <= float(budgets.get("memory_peak_mb_max", float("inf"))),
        "network_mb_per_task_max": float(measurements.get("network_mb_per_task", 0.0)) <= float(budgets.get("network_mb_per_task_max", float("inf"))),
    }
    evidence = {
        "product": green.get("product", path.stem),
        "dsl_file": str(path),
        "certification_profile": green.get("certification_profile"),
        "green_mode": green.get("green_mode", "advisory"),
        "functional_unit": unit,
        "functional_unit_count": unit_count,
        "boundary": green.get("boundary", {}),
        "carbon": carbon,
        "budgets": budgets,
        "models": [dict(kind=b.kind, name=b.name, **b.body) for b in by.get("model", [])],
        "training": [dict(kind=b.kind, name=b.name, **b.body) for b in by.get("train_model", [])],
        "inference": [dict(kind=b.kind, name=b.name, **b.body) for b in by.get("infer_endpoint", [])],
        "llm_tasks": [dict(kind=b.kind, name=b.name, **b.body) for b in by.get("llm_task", [])],
        "model_routes": [dict(kind=b.kind, name=b.name, **b.body) for b in by.get("model_route", [])],
        "targets": [dict(kind=b.kind, name=b.name, **b.body) for b in by.get("target", [])],
        "data_pipelines": [dict(kind=b.kind, name=b.name, **b.body) for b in by.get("data_pipeline", [])],
        "measurements": measurements,
        "derived": {
            "training_energy_kwh": training_kwh,
            "inference_energy_kwh": inference_kwh,
            "idle_energy_kwh": idle_kwh,
            "network_energy_kwh": network_kwh,
            "storage_energy_kwh": storage_kwh,
            "total_operational_energy_kwh": total_kwh,
            "operational_carbon_gco2e": total_carbon,
            "energy_per_functional_unit_kwh": per_unit_kwh,
            "carbon_per_functional_unit_gco2e": per_unit_carbon,
            "memory_peak_mb": measurements.get("memory_peak_mb"),
            "network_mb_per_task": measurements.get("network_mb_per_task"),
            "tokens_per_task": measurements.get("tokens_per_task"),
            "p95_latency_energy_j": measurements.get("p95_latency_energy_j"),
            "model_size_mb": measurements.get("model_size_mb"),
        },
        "measurement_quality": green.get("measurement_quality", "MQ1" if total_kwh else "MQ0"),
        "data_quality": green.get("data_quality", "DQ1" if grid else "DQ0"),
        "budget_status": budget_status,
        "warnings": warnings,
        "errors": errors,
        "audit_evidence_references": green.get("audit_evidence", []),
        "certification_note": "Evidence report only; this tool does not grant certification.",
    }
    return evidence

def load_manifest(path: Path) -> List[Block]:
    return Parser(path.read_text()).parse()

def cmd_validate(args: argparse.Namespace) -> int:
    blocks = load_manifest(Path(args.file))
    warnings, errors = validate(blocks, args.strict)
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
    if args.output:
        Path(args.output).write_text(json.dumps(report, indent=2, sort_keys=True) + "\n")
    else:
        print(json.dumps(report, indent=2, sort_keys=True))
    return 1 if report["errors"] else 0

def cmd_check(args: argparse.Namespace) -> int:
    path = Path(args.file)
    current = build_report(path, load_manifest(path), args.strict)
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
        if float(cur) > float(base) * (1.0 + threshold):
            regressions.append({"field": field, "baseline": base, "current": cur, "threshold_percent": args.threshold_percent})
    result = {"passed": not regressions and not current["errors"], "regressions": regressions, "errors": current["errors"], "warnings": current["warnings"]}
    if args.output:
        Path(args.output).write_text(json.dumps(result, indent=2, sort_keys=True) + "\n")
    else:
        print(json.dumps(result, indent=2, sort_keys=True))
    return 0 if result["passed"] else 1

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
    sp.add_argument("file")
    sp.add_argument("--baseline", required=True)
    sp.add_argument("--threshold-percent", type=float, default=10.0)
    sp.add_argument("--strict", choices=["off", "advisory", "strict"])
    sp.add_argument("--output")
    sp.set_defaults(func=cmd_check)
    args = ap.parse_args(argv)
    return args.func(args)

if __name__ == "__main__":
    raise SystemExit(main())
