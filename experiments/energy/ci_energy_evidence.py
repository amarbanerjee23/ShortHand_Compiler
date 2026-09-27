#!/usr/bin/env python3
"""Claim-safe CI energy evidence helpers.

Evidence classes:
  E0: externally calibrated whole-system physical measurement (not produced here)
  E1: hardware-reported component energy (Linux powercap/RAPL)
  E2: architecture-matched calibrated performance-counter model
  E3: explicitly analytical operation/memory model

This module never converts elapsed time, TDP, CPU utilisation, or an unmatched
reference coefficient into measured joules. Missing capabilities downgrade the
result instead of fabricating precision.
"""
from __future__ import annotations

import argparse
import datetime as _dt
import hashlib
import json
import math
import os
import pathlib
import platform
import re
import shutil
import subprocess
import sys
import threading
import time
from typing import Dict, Iterable, List, Optional, Tuple

SCHEMA = "shorthand.energy.ci_evidence.v1"
PROFILE_SCHEMA = "shorthand.energy.calibration_profile.v1"
FEATURES = (
    "elapsed_seconds",
    "instructions",
    "cycles",
    "branches",
    "branch_misses",
    "cache_references",
    "cache_misses",
    "memory_bytes",
    "fp_operations",
    "integer_operations",
)
MAX_PROFILE_BYTES = 1024 * 1024
MAX_COUNTER = 10**30


def _utc() -> str:
    return _dt.datetime.now(_dt.timezone.utc).isoformat()


def _sha(path: pathlib.Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def _read_text(path: pathlib.Path, limit: int = 65536) -> str:
    if path.is_symlink() or not path.is_file() or path.stat().st_size > limit:
        raise ValueError("unsafe evidence input: " + str(path))
    data = path.read_text(errors="strict")
    if "\x00" in data:
        raise ValueError("NUL in evidence input")
    return data


def _json(path: pathlib.Path):
    if path.stat().st_size > MAX_PROFILE_BYTES:
        raise ValueError("oversized JSON input")
    return json.loads(_read_text(path, MAX_PROFILE_BYTES))


def write_json(path: pathlib.Path, value) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, indent=2, sort_keys=True, allow_nan=False) + "\n")


def cpu_identity(cpuinfo: pathlib.Path = pathlib.Path("/proc/cpuinfo")) -> Dict[str, object]:
    result: Dict[str, object] = {
        "platform": platform.platform(),
        "machine": platform.machine(),
        "processor": platform.processor(),
    }
    if cpuinfo.is_file() and not cpuinfo.is_symlink():
        text = cpuinfo.read_text(errors="replace")[:1024 * 1024]
        first = text.split("\n\n", 1)[0]
        fields = {}
        for line in first.splitlines():
            if ":" in line:
                key, value = line.split(":", 1)
                fields[key.strip()] = value.strip()
        result.update({
            "vendor_id": fields.get("vendor_id"),
            "cpu_family": fields.get("cpu family"),
            "model": fields.get("model"),
            "model_name": fields.get("model name"),
            "stepping": fields.get("stepping"),
            "microcode": fields.get("microcode"),
        })
    return result


def _powercap_roots() -> Tuple[pathlib.Path, ...]:
    return (
        pathlib.Path("/sys/class/powercap"),
        pathlib.Path("/sys/devices/virtual/powercap"),
    )


def discover_rapl(root: Optional[pathlib.Path] = None) -> List[Dict[str, object]]:
    roots = (root,) if root else _powercap_roots()
    seen = set()
    domains: List[Dict[str, object]] = []
    for candidate in roots:
        if not candidate.exists() or not candidate.is_dir():
            continue
        try:
            base = candidate.resolve(strict=True)
        except OSError:
            continue
        for energy_path in candidate.rglob("energy_uj"):
            try:
                if energy_path.is_symlink() or not energy_path.is_file():
                    continue
                parent = energy_path.parent
                resolved = parent.resolve(strict=True)
                if root and not resolved.is_relative_to(base):
                    continue
                key = str(resolved)
                if key in seen:
                    continue
                seen.add(key)
                name_path = parent / "name"
                name = _read_text(name_path, 512).strip() if name_path.is_file() and not name_path.is_symlink() else parent.name
                range_path = parent / "max_energy_range_uj"
                max_range = None
                if range_path.is_file() and not range_path.is_symlink():
                    raw = _read_text(range_path, 128).strip()
                    if raw.isdigit():
                        max_range = int(raw)
                readable = os.access(energy_path, os.R_OK)
                domains.append({
                    "path": str(energy_path),
                    "name": name,
                    "max_energy_range_uj": max_range,
                    "readable": readable,
                    "contributes": name.startswith("package-"),
                })
            except (OSError, ValueError):
                continue
    domains.sort(key=lambda item: str(item["path"]))
    return domains


def perf_probe() -> Dict[str, object]:
    perf = shutil.which("perf")
    paranoid = None
    p = pathlib.Path("/proc/sys/kernel/perf_event_paranoid")
    try:
        if p.is_file():
            paranoid = int(p.read_text().strip())
    except (OSError, ValueError):
        pass
    return {"path": perf, "perf_event_paranoid": paranoid}


def load_profiles(directory: pathlib.Path) -> List[Dict[str, object]]:
    if not directory.is_dir():
        return []
    out = []
    for path in sorted(directory.glob("*.json")):
        try:
            value = _json(path)
            validate_profile(value)
            value = dict(value)
            value["_path"] = str(path)
            value["_sha256"] = _sha(path)
            out.append(value)
        except (ValueError, OSError, json.JSONDecodeError):
            continue
    return out


def validate_profile(profile: Dict[str, object]) -> None:
    if profile.get("schema") != PROFILE_SCHEMA:
        raise ValueError("invalid calibration profile schema")
    evidence = profile.get("evidence_class")
    if evidence not in ("E2", "E3"):
        raise ValueError("profile evidence class must be E2 or E3")
    if evidence == "E2" and profile.get("validated_calibration") is not True:
        raise ValueError("E2 requires validated_calibration=true")
    if not isinstance(profile.get("profile_id"), str) or not profile["profile_id"]:
        raise ValueError("missing profile_id")
    uncertainty = profile.get("uncertainty_percent")
    if not isinstance(uncertainty, (int, float)) or not math.isfinite(uncertainty) or not 0 <= uncertainty <= 100:
        raise ValueError("invalid profile uncertainty")
    coeff = profile.get("coefficients")
    if not isinstance(coeff, dict) or not coeff:
        raise ValueError("missing profile coefficients")
    for key, value in coeff.items():
        if key not in FEATURES and key != "intercept_joules":
            raise ValueError("unknown coefficient: " + str(key))
        if not isinstance(value, (int, float)) or not math.isfinite(value):
            raise ValueError("nonfinite coefficient")
    for key in ("source", "calibration_provenance"):
        if not isinstance(profile.get(key), str) or not profile[key].strip():
            raise ValueError("missing " + key)


def profile_matches(profile: Dict[str, object], hardware: Dict[str, object]) -> bool:
    match = profile.get("hardware_match", {})
    if not isinstance(match, dict):
        return False
    for key in ("vendor_id", "cpu_family", "model", "machine"):
        expected = match.get(key)
        if expected is not None and str(hardware.get(key)) != str(expected):
            return False
    regex = match.get("model_name_regex")
    if regex is not None:
        if not isinstance(regex, str) or len(regex) > 512:
            return False
        if re.search(regex, str(hardware.get("model_name") or "")) is None:
            return False
    return True


def select_profile(profiles: Iterable[Dict[str, object]], hardware: Dict[str, object],
                   evidence: str) -> Optional[Dict[str, object]]:
    matches = [p for p in profiles if p.get("evidence_class") == evidence and profile_matches(p, hardware)]
    if len(matches) > 1:
        raise ValueError("multiple matching energy profiles; make hardware_match unambiguous")
    return matches[0] if matches else None


def probe(profile_dir: Optional[pathlib.Path] = None, powercap_root: Optional[pathlib.Path] = None) -> Dict[str, object]:
    hardware = cpu_identity()
    rapl = discover_rapl(powercap_root)
    profiles = load_profiles(profile_dir) if profile_dir else []
    e2 = select_profile(profiles, hardware, "E2") if profiles else None
    e3 = select_profile(profiles, hardware, "E3") if profiles else None
    readable_packages = [d for d in rapl if d["contributes"] and d["readable"]]
    highest = "E1" if readable_packages else ("E2" if e2 else ("E3" if e3 else None))
    return {
        "schema": SCHEMA,
        "kind": "probe",
        "recorded_utc": _utc(),
        "hardware": hardware,
        "rapl_domains": rapl,
        "perf": perf_probe(),
        "matching_profiles": {
            "E2": None if e2 is None else {"profile_id": e2["profile_id"], "sha256": e2["_sha256"]},
            "E3": None if e3 is None else {"profile_id": e3["profile_id"], "sha256": e3["_sha256"]},
        },
        "highest_available_evidence_class": highest,
        "physical_system_energy_measured": False,
        "claim_authorized": False,
    }


def _number(value, name: str) -> float:
    if not isinstance(value, (int, float)) or not math.isfinite(value) or value < 0 or value > MAX_COUNTER:
        raise ValueError("invalid counter: " + name)
    return float(value)


def estimate(counters: Dict[str, object], profile: Dict[str, object],
             hardware: Dict[str, object], profile_sha256: Optional[str] = None) -> Dict[str, object]:
    validate_profile(profile)
    if not profile_matches(profile, hardware):
        raise ValueError("calibration profile does not match hardware")
    completed = counters.get("completed_correct_tasks", counters.get("completed"))
    if type(completed) is not int or completed <= 0:
        raise ValueError("completed_correct_tasks must be a positive integer")
    coefficients = profile["coefficients"]
    joules = float(coefficients.get("intercept_joules", 0.0))
    used = {}
    for feature, coefficient in coefficients.items():
        if feature == "intercept_joules":
            continue
        if feature not in counters:
            raise ValueError("missing required counter: " + feature)
        value = _number(counters[feature], feature)
        joules += value * float(coefficient)
        used[feature] = value
    if not math.isfinite(joules) or joules < 0:
        raise ValueError("energy model produced invalid joules")
    per_task = joules / completed
    evidence = str(profile["evidence_class"])
    key = "calibrated_joules_estimate" if evidence == "E2" else "analytical_joules_estimate"
    return {
        "schema": SCHEMA,
        "kind": "estimate",
        "evidence_class": evidence,
        "method": profile["profile_id"],
        "physical_system_energy_measured": False,
        "component_energy_measured": False,
        "hardware_measured_joules": None,
        "calibrated_joules_estimate": joules if evidence == "E2" else None,
        "analytical_joules_estimate": joules if evidence == "E3" else None,
        "joules_per_completed_correct_task": per_task,
        "completed_correct_tasks": completed,
        "model_uncertainty_percent": float(profile["uncertainty_percent"]),
        "hardware": hardware,
        "calibration_profile_sha256": profile_sha256,
        "features": used,
        "source": profile["source"],
        "calibration_provenance": profile["calibration_provenance"],
        "claim_authorized": False,
        "value_field": key,
        "recorded_utc": _utc(),
    }


def compare(shorthand: Dict[str, object], baseline: Dict[str, object]) -> Dict[str, object]:
    for item in (shorthand, baseline):
        if item.get("schema") != SCHEMA or item.get("kind") not in ("estimate", "measurement"):
            raise ValueError("invalid energy evidence")
        if item.get("joules_per_completed_correct_task") is None:
            raise ValueError("joules/task unavailable")
    if shorthand.get("evidence_class") != baseline.get("evidence_class"):
        raise ValueError("cannot compare different evidence classes")
    if shorthand.get("method") != baseline.get("method"):
        raise ValueError("cannot compare different measurement/model methods")
    if shorthand.get("calibration_profile_sha256") != baseline.get("calibration_profile_sha256"):
        raise ValueError("cannot compare different calibration profiles")
    a = _number(shorthand["joules_per_completed_correct_task"], "shorthand_joules_per_task")
    b = _number(baseline["joules_per_completed_correct_task"], "baseline_joules_per_task")
    if b == 0:
        raise ValueError("zero baseline energy")
    ratio = a / b
    return {
        "schema": SCHEMA,
        "kind": "comparison",
        "evidence_class": shorthand["evidence_class"],
        "method": shorthand["method"],
        "short_hand_to_baseline_energy_ratio": ratio,
        "energy_delta_percent": 100.0 * (1.0 - ratio),
        "physical_system_energy_measured": False,
        "claim_authorized": False,
        "recorded_utc": _utc(),
    }


def _read_counter(path: pathlib.Path) -> int:
    text = _read_text(path, 128).strip()
    if not text.isdigit():
        raise ValueError("malformed powercap counter")
    return int(text)


class RaplSampler:
    """Poll package counters so counter wraps can be accumulated explicitly."""

    def __init__(self, root: Optional[pathlib.Path] = None, interval_seconds: float = 0.01,
                 maximum_package_power_w: float = 2000.0):
        if not 0.001 <= interval_seconds <= 0.25:
            raise ValueError("invalid RAPL sample interval")
        if not 100 <= maximum_package_power_w <= 10000:
            raise ValueError("invalid conservative package-power bound")
        domains = [d for d in discover_rapl(root) if d["contributes"] and d["readable"]]
        if not domains:
            raise RuntimeError("rapl_package_energy_unavailable")
        if any(d["max_energy_range_uj"] is None for d in domains):
            raise RuntimeError("rapl_max_energy_range_unavailable")
        self.domains = domains
        self.interval = interval_seconds
        self.max_power = maximum_package_power_w
        self._stop = threading.Event()
        self._thread = None
        self._error: Optional[BaseException] = None
        self._total_uj = 0
        self._samples = 0

    def _values(self) -> Tuple[float, Dict[str, int]]:
        return time.monotonic(), {str(d["path"]): _read_counter(pathlib.Path(str(d["path"]))) for d in self.domains}

    def start(self) -> None:
        t0, previous = self._values()
        self._samples = 1

        def run():
            nonlocal t0, previous
            try:
                while not self._stop.wait(self.interval):
                    now, current = self._values()
                    dt = now - t0
                    if dt <= 0:
                        raise RuntimeError("non_monotonic_rapl_sampling")
                    for d in self.domains:
                        key = str(d["path"])
                        old, new = previous[key], current[key]
                        rng = int(d["max_energy_range_uj"])
                        delta = new - old if new >= old else rng - old + new
                        if delta < 0 or delta / 1e6 > dt * self.max_power:
                            raise RuntimeError("rapl_delta_exceeds_conservative_power_bound")
                        self._total_uj += delta
                    previous, t0 = current, now
                    self._samples += 1
            except BaseException as exc:
                self._error = exc
                self._stop.set()

        self._thread = threading.Thread(target=run, name="shorthand-rapl-sampler", daemon=True)
        self._thread.start()

    def stop(self) -> Tuple[float, int]:
        if self._thread is None:
            raise RuntimeError("sampler_not_started")
        self._stop.set()
        self._thread.join(timeout=2)
        if self._thread.is_alive():
            raise RuntimeError("rapl_sampler_did_not_stop")
        if self._error:
            raise RuntimeError(str(self._error))
        return self._total_uj / 1e6, self._samples


def measure_command(argv: List[str], completed: int, output: pathlib.Path,
                    powercap_root: Optional[pathlib.Path] = None,
                    sample_interval: float = 0.01) -> int:
    if type(completed) is not int or completed <= 0:
        raise ValueError("completed must be positive")
    if not argv:
        raise ValueError("missing command")
    hardware = cpu_identity()
    try:
        sampler = RaplSampler(powercap_root, interval_seconds=sample_interval)
    except RuntimeError as exc:
        write_json(output, {
            "schema": SCHEMA,
            "kind": "measurement",
            "evidence_class": None,
            "method": "rapl_package",
            "available": False,
            "reason": str(exc),
            "physical_system_energy_measured": False,
            "component_energy_measured": False,
            "joules_per_completed_correct_task": None,
            "completed_correct_tasks": completed,
            "hardware": hardware,
            "claim_authorized": False,
            "recorded_utc": _utc(),
        })
        return 3
    start = time.perf_counter()
    sampler.start()
    try:
        proc = subprocess.run(argv, check=False)
    finally:
        joules, samples = sampler.stop()
    elapsed = time.perf_counter() - start
    available = proc.returncode == 0
    write_json(output, {
        "schema": SCHEMA,
        "kind": "measurement",
        "evidence_class": "E1" if available else None,
        "method": "rapl_package",
        "available": available,
        "reason": "package_energy_not_process_attributed" if available else "command_failed",
        "physical_system_energy_measured": False,
        "component_energy_measured": available,
        "hardware_measured_joules": joules if available else None,
        "calibrated_joules_estimate": None,
        "analytical_joules_estimate": None,
        "joules_per_completed_correct_task": joules / completed if available else None,
        "completed_correct_tasks": completed,
        "elapsed_seconds": elapsed,
        "rapl_sample_count": samples,
        "hardware": hardware,
        "calibration_profile_sha256": None,
        "claim_authorized": False,
        "recorded_utc": _utc(),
    })
    return proc.returncode


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="action", required=True)

    p = sub.add_parser("probe")
    p.add_argument("--output", type=pathlib.Path, required=True)
    p.add_argument("--profiles", type=pathlib.Path)

    e = sub.add_parser("estimate")
    e.add_argument("--counters", type=pathlib.Path, required=True)
    e.add_argument("--profile", type=pathlib.Path, required=True)
    e.add_argument("--output", type=pathlib.Path, required=True)

    c = sub.add_parser("compare")
    c.add_argument("--shorthand", type=pathlib.Path, required=True)
    c.add_argument("--baseline", type=pathlib.Path, required=True)
    c.add_argument("--output", type=pathlib.Path, required=True)

    m = sub.add_parser("measure-command")
    m.add_argument("--completed", type=int, required=True)
    m.add_argument("--output", type=pathlib.Path, required=True)
    m.add_argument("--sample-interval", type=float, default=0.01)
    m.add_argument("command", nargs=argparse.REMAINDER)

    args = parser.parse_args()
    if args.action == "probe":
        write_json(args.output, probe(args.profiles))
        return 0
    if args.action == "estimate":
        counters = _json(args.counters)
        profile = _json(args.profile)
        write_json(args.output, estimate(counters, profile, cpu_identity(), _sha(args.profile)))
        return 0
    if args.action == "compare":
        write_json(args.output, compare(_json(args.shorthand), _json(args.baseline)))
        return 0
    command = list(args.command)
    if command and command[0] == "--":
        command = command[1:]
    return measure_command(command, args.completed, args.output, sample_interval=args.sample_interval)


if __name__ == "__main__":
    sys.exit(main())
