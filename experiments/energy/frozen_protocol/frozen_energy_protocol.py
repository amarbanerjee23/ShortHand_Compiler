#!/usr/bin/env python3
"""Frozen-matrix energy protocol for ShortHand versus C, C++, Rust and Go.

This tool only measures and judges evidence; it never changes the compiler or
runtime. It deliberately refuses to produce an energy claim unless every gate
holds:

* host qualification: a physical energy source (Linux powercap/RAPL package
  counters and/or a cumulative external AC meter command) is readable, with
  no virtualisation, a quiet system and recorded CPU frequency policy;
* functional equivalence: every variant prints the same bytes as the reference;
* statistics: enough balanced, randomised rounds, idle power subtracted, and a
  bootstrap confidence interval for the paired energy ratio;
* replication: at least two distinct hosts when claims are aggregated.

Elapsed time, TDP and CPU utilisation are never substituted for energy. When no
physical counter exists, joules are null and the claim is unauthorised.

Subcommands: qualify, run, aggregate.
"""
from __future__ import annotations

import argparse
import datetime
import hashlib
import json
import math
import os
import pathlib
import platform
import random
import shutil
import statistics
import subprocess
import sys
import tempfile
import time
from typing import Any, Callable

HERE = pathlib.Path(__file__).resolve().parent
ROOT = HERE.parents[2]
SCHEMA = "shorthand.frozen_energy_protocol.v1"
POWERCAP_ROOT = pathlib.Path("/sys/class/powercap")


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def sha256_file(path: pathlib.Path) -> str:
    return sha256_bytes(path.read_bytes())


def load_protocol(path: pathlib.Path) -> dict[str, Any]:
    protocol = json.loads(path.read_text())
    required = ("name", "workloads", "variants", "rounds", "warmup_rounds", "idle_windows",
                "confidence", "bootstrap_resamples", "seed", "ac_meter_agreement_tolerance",
                "minimum_rounds_for_claim", "minimum_hosts_for_claim", "reference_variant",
                "candidate_variant")
    missing = [key for key in required if key not in protocol]
    if missing:
        raise ValueError(f"protocol is missing keys: {missing}")
    if protocol["reference_variant"] not in protocol["variants"] or \
       protocol["candidate_variant"] not in protocol["variants"]:
        raise ValueError("reference/candidate variant must be declared")
    if not 0.5 < protocol["confidence"] < 1.0:
        raise ValueError("confidence must be in (0.5, 1)")
    return protocol


# --------------------------------------------------------------------------- hardware


class RaplSource:
    """Cumulative package energy from top-level powercap domains (microjoules)."""

    name = "rapl"

    def __init__(self, root: pathlib.Path = POWERCAP_ROOT):
        self.root = root
        self.domains: list[tuple[pathlib.Path, int]] = []
        self.problems: list[str] = []
        if not root.is_dir():
            self.problems.append("powercap directory is absent")
            return
        for entry in sorted(root.iterdir()):
            # Top-level package domains only: sub-domains (core/dram) are
            # already included in the package counter and must not be summed.
            if ":" not in entry.name or entry.name.count(":") != 1:
                continue
            energy = entry / "energy_uj"
            if not energy.exists():
                continue
            label = (entry / "name").read_text().strip() if (entry / "name").exists() else ""
            if label and not label.startswith("package"):
                continue
            try:
                int(energy.read_text())
                max_range = int((entry / "max_energy_range_uj").read_text())
            except (OSError, ValueError) as error:
                self.problems.append(f"{entry.name}: unreadable counter ({type(error).__name__})")
                continue
            if max_range <= 0:
                self.problems.append(f"{entry.name}: invalid max_energy_range_uj")
                continue
            self.domains.append((entry, max_range))
        if not self.domains and not self.problems:
            self.problems.append("no readable package energy domains")

    @property
    def available(self) -> bool:
        return bool(self.domains) and not self.problems

    def read(self) -> list[int]:
        return [int((entry / "energy_uj").read_text()) for entry, _ in self.domains]

    def delta_joules(self, begin: list[int], end: list[int], seconds: float) -> float:
        total = 0.0
        for (entry, max_range), first, last in zip(self.domains, begin, end):
            diff = last - first
            if diff < 0:
                diff += max_range
                # A second wrap would be undetectable. Reject windows that
                # could contain one assuming an implausible 1 kW package.
                if seconds * 1_000_000_000 >= max_range:
                    raise RuntimeError(f"{entry.name}: window may contain more than one counter wrap")
            total += diff / 1_000_000.0
        return total


class AcMeterSource:
    """Cumulative joules printed (one float) by an external meter command."""

    name = "ac_meter"

    def __init__(self, command: str | None):
        self.command = command
        self.problems: list[str] = []
        if command:
            try:
                self.read()
            except Exception as error:  # noqa: BLE001 - reported as qualification problem
                self.problems.append(f"meter command failed: {error}")

    @property
    def available(self) -> bool:
        return bool(self.command) and not self.problems

    def read(self) -> float:
        if not self.command:
            raise RuntimeError("no meter command configured")
        out = subprocess.run(self.command, shell=True, check=True, capture_output=True,
                             text=True, timeout=10).stdout.strip()
        value = float(out)
        if not math.isfinite(value) or value < 0:
            raise RuntimeError("meter returned a non-finite or negative reading")
        return value

    def delta_joules(self, begin: float, end: float, _seconds: float) -> float:
        if end < begin:
            raise RuntimeError("AC meter cumulative counter moved backwards")
        return end - begin


def _read_text(path: str) -> str | None:
    try:
        return pathlib.Path(path).read_text().strip()
    except OSError:
        return None


def host_facts() -> dict[str, Any]:
    cpuinfo = _read_text("/proc/cpuinfo") or ""
    model = next((line.split(":", 1)[1].strip() for line in cpuinfo.splitlines()
                  if line.startswith("model name")), platform.processor())
    flags = next((line for line in cpuinfo.splitlines() if line.startswith("flags")), "")
    governors = sorted({(_read_text(f"/sys/devices/system/cpu/cpu{i}/cpufreq/scaling_governor") or "unavailable")
                        for i in range(os.cpu_count() or 1)})
    return {
        "cpu_model": model,
        "logical_cpus": os.cpu_count(),
        "machine": platform.machine(),
        "kernel": platform.release(),
        "virtualized": "hypervisor" in flags.split(),
        "scaling_governors": governors,
        "intel_no_turbo": _read_text("/sys/devices/system/cpu/intel_pstate/no_turbo"),
        "smt_active": _read_text("/sys/devices/system/cpu/smt/active"),
        "load_average_1m": os.getloadavg()[0],
    }


def host_id(facts: dict[str, Any]) -> str:
    stable = json.dumps({k: facts[k] for k in ("cpu_model", "logical_cpus", "machine", "kernel")},
                        sort_keys=True)
    return sha256_bytes((stable + platform.node()).encode())[:16]


def qualify_host(protocol: dict[str, Any], rapl: RaplSource, meter: AcMeterSource) -> dict[str, Any]:
    facts = host_facts()
    reasons: list[str] = []
    if not (rapl.available or meter.available):
        reasons.append("no physical energy source: RAPL package counters and AC meter are both unavailable")
        reasons.extend(f"rapl: {p}" for p in rapl.problems)
        reasons.extend(f"ac_meter: {p}" for p in meter.problems)
    if facts["virtualized"]:
        reasons.append("host is virtualised (hypervisor flag); counters are not a trustworthy isolated measurement")
    if facts["load_average_1m"] > protocol.get("max_idle_load_average", 0.5) * (facts["logical_cpus"] or 1):
        reasons.append(f"host is not quiet: 1m load average {facts['load_average_1m']:.2f}")
    if protocol.get("require_performance_governor", True) and \
            facts["scaling_governors"] not in (["performance"],):
        reasons.append(f"CPU governor is not fixed to 'performance': {facts['scaling_governors']}")
    return {
        "schema": SCHEMA,
        "kind": "host_qualification",
        "host_id": host_id(facts),
        "host": facts,
        "sources": {"rapl": rapl.available, "ac_meter": meter.available},
        "qualified": not reasons,
        "disqualifying_reasons": reasons,
    }


# --------------------------------------------------------------------------- statistics


def percentile(values: list[float], q: float) -> float:
    ordered = sorted(values)
    position = q * (len(ordered) - 1)
    lo, hi = math.floor(position), math.ceil(position)
    return ordered[lo] + (ordered[hi] - ordered[lo]) * (position - lo)


def bootstrap_ratio_ci(ratios: list[float], confidence: float, resamples: int, seed: int) -> dict[str, float]:
    """Percentile bootstrap CI of the geometric-mean paired ratio (deterministic)."""
    if not ratios or any(r <= 0 or not math.isfinite(r) for r in ratios):
        raise ValueError("ratios must be positive and finite")
    logs = [math.log(r) for r in ratios]
    rng = random.Random(seed)
    means = []
    for _ in range(resamples):
        sample = [logs[rng.randrange(len(logs))] for _ in logs]
        means.append(sum(sample) / len(sample))
    alpha = (1.0 - confidence) / 2.0
    return {
        "geomean": math.exp(sum(logs) / len(logs)),
        "lower": math.exp(percentile(means, alpha)),
        "upper": math.exp(percentile(means, 1.0 - alpha)),
        "n": len(ratios),
    }


def idle_power_watts(samples: list[tuple[float, float]]) -> float:
    """Median of joules/seconds idle windows."""
    return statistics.median(j / s for j, s in samples)


def decide_claim(protocol: dict[str, Any], ci: dict[str, float], equivalent: bool) -> str:
    """Return verdict for 'candidate uses less energy than reference' on one host."""
    if not equivalent:
        return "invalid_outputs_differ"
    if ci["n"] < protocol["minimum_rounds_for_claim"]:
        return "inconclusive_insufficient_rounds"
    if ci["upper"] < 1.0:
        return "candidate_lower_energy"
    if ci["lower"] > 1.0:
        return "candidate_higher_energy"
    return "no_significant_difference"


# --------------------------------------------------------------------------- build / run


def which_or_none(name: str) -> str | None:
    return shutil.which(name)


def build_variant(variant: str, spec: dict[str, Any], workload: str, work: pathlib.Path,
                  shorthand: pathlib.Path) -> tuple[list[str], dict[str, Any]]:
    """Build one variant; returns the run command and its toolchain record."""
    source = HERE / "workloads" / f"{workload}.{spec['extension']}"
    if not source.exists():
        raise FileNotFoundError(source)
    out_dir = work / variant
    out_dir.mkdir(parents=True, exist_ok=True)
    local = out_dir / source.name
    shutil.copyfile(source, local)
    binary = out_dir / "run"
    command = [part.format(src=str(local), out=str(binary), shorthand=str(shorthand))
               for part in spec["build"]]
    tool = command[0]
    if not (os.path.isabs(tool) and os.path.exists(tool)) and which_or_none(tool) is None:
        raise FileNotFoundError(f"toolchain not found for {variant}: {tool}")
    env = dict(os.environ)
    if spec.get("needs_runtime_library_path"):
        env["LD_LIBRARY_PATH"] = os.pathsep.join(filter(None, [str(shorthand.parent), env.get("LD_LIBRARY_PATH", "")]))
    subprocess.run(command, cwd=out_dir, check=True, capture_output=True, env=env, timeout=600)
    produced = out_dir / spec.get("produced", "run")
    if not produced.exists():
        raise RuntimeError(f"{variant}: build produced no executable at {produced}")
    version = subprocess.run(spec["version"], capture_output=True, text=True, check=False).stdout.strip().splitlines()
    return [str(produced)], {
        "variant": variant,
        "build_command": [c.replace(str(work), "<work>") for c in command],
        "toolchain": version[0] if version else "unknown",
        "executable_sha256": sha256_file(produced),
    }


class Meter:
    """Combine the available physical sources behind begin()/end()."""

    def __init__(self, rapl: RaplSource, ac: AcMeterSource):
        self.rapl, self.ac = rapl, ac

    def begin(self) -> dict[str, Any]:
        return {
            "rapl": self.rapl.read() if self.rapl.available else None,
            "ac": self.ac.read() if self.ac.available else None,
            "t": time.perf_counter(),
        }

    def end(self, start: dict[str, Any]) -> dict[str, Any]:
        stop = {
            "rapl": self.rapl.read() if self.rapl.available else None,
            "ac": self.ac.read() if self.ac.available else None,
            "t": time.perf_counter(),
        }
        seconds = stop["t"] - start["t"]
        joules: dict[str, float | None] = {"rapl": None, "ac_meter": None}
        if start["rapl"] is not None:
            joules["rapl"] = self.rapl.delta_joules(start["rapl"], stop["rapl"], seconds)
        if start["ac"] is not None:
            joules["ac_meter"] = self.ac.delta_joules(start["ac"], stop["ac"], seconds)
        return {"seconds": seconds, "joules": joules}


def run_once(command: list[str], stdin_text: str, meter: Meter | None, cpu: int | None) -> dict[str, Any]:
    def pin() -> None:
        if cpu is not None and hasattr(os, "sched_setaffinity"):
            os.sched_setaffinity(0, {cpu})

    start = meter.begin() if meter else {"t": time.perf_counter()}
    proc = subprocess.run(command, input=stdin_text.encode(), capture_output=True, preexec_fn=pin, check=False,
                          timeout=900)
    if meter:
        window = meter.end(start)
    else:
        window = {"seconds": time.perf_counter() - start["t"], "joules": {"rapl": None, "ac_meter": None}}
    return {"returncode": proc.returncode, "stdout_sha256": sha256_bytes(proc.stdout), "stdout": proc.stdout,
            **window}


def idle_windows(meter: Meter, count: int, seconds: float) -> list[dict[str, Any]]:
    windows = []
    for _ in range(count):
        start = meter.begin()
        time.sleep(seconds)
        windows.append(meter.end(start))
    return windows


def net_energy(sources: list[str], run: dict[str, Any], idle_w: dict[str, float]) -> dict[str, float | None]:
    out: dict[str, float | None] = {}
    for source in sources:
        gross = run["joules"][source]
        out[source] = None if gross is None else max(gross - idle_w[source] * run["seconds"], 0.0)
    return out


def summarise(protocol: dict[str, Any], per_variant: dict[str, list[dict[str, Any]]], sources: list[str],
              equivalent: bool) -> dict[str, Any]:
    ref, cand = protocol["reference_variant"], protocol["candidate_variant"]
    result: dict[str, Any] = {"energy": {}, "time": {}}
    for variant, rows in per_variant.items():
        result["time"][variant] = {
            "median_seconds": statistics.median(r["seconds"] for r in rows),
            "p95_seconds": percentile([r["seconds"] for r in rows], 0.95),
        }
        result["energy"][variant] = {}
        for source in sources:
            values = [r["net_joules"][source] for r in rows if r["net_joules"][source] is not None]
            result["energy"][variant][source] = {
                "median_net_joules": statistics.median(values) if values else None,
                "p95_net_joules": percentile(values, 0.95) if values else None,
            }
    comparisons = {}
    for baseline in protocol["variants"]:
        if baseline == cand:
            continue
        per_source = {}
        for source in sources:
            ratios = []
            for a, b in zip(per_variant[cand], per_variant[baseline]):
                x, y = a["net_joules"][source], b["net_joules"][source]
                if x and y:
                    ratios.append(x / y)
            if not ratios:
                per_source[source] = {"verdict": "no_energy_data"}
                continue
            ci = bootstrap_ratio_ci(ratios, protocol["confidence"], protocol["bootstrap_resamples"], protocol["seed"])
            per_source[source] = {**ci, "verdict": decide_claim(protocol, ci, equivalent)}
        comparisons[baseline] = per_source
    result["candidate_vs_baseline_energy_ratio"] = comparisons
    return result


def command_qualify(args: argparse.Namespace) -> int:
    protocol = load_protocol(args.protocol)
    report = qualify_host(protocol, RaplSource(pathlib.Path(args.powercap_root)), AcMeterSource(args.ac_meter_cmd))
    text = json.dumps(report, indent=2, sort_keys=True)
    if args.output:
        pathlib.Path(args.output).write_text(text + "\n")
    print(text)
    return 0 if report["qualified"] else 3


def command_run(args: argparse.Namespace) -> int:
    protocol = load_protocol(args.protocol)
    rapl = RaplSource(pathlib.Path(args.powercap_root))
    ac = AcMeterSource(args.ac_meter_cmd)
    qualification = qualify_host(protocol, rapl, ac)
    out = pathlib.Path(args.output_dir)
    out.mkdir(parents=True, exist_ok=True)
    (out / "qualification.json").write_text(json.dumps(qualification, indent=2, sort_keys=True) + "\n")
    physical = qualification["qualified"]
    if not physical and not args.allow_unqualified:
        print("host not qualified; no energy run performed (use --allow-unqualified for a functional/time dry run)",
              file=sys.stderr)
        for reason in qualification["disqualifying_reasons"]:
            print(f"  - {reason}", file=sys.stderr)
        return 3
    sources = [name for name, ok in (("rapl", rapl.available), ("ac_meter", ac.available)) if ok and physical]
    meter = Meter(rapl, ac) if sources else None
    scale = args.scale
    rng = random.Random(protocol["seed"])
    shorthand = pathlib.Path(args.shorthand).resolve()
    reports = []
    with tempfile.TemporaryDirectory(prefix="shorthand-frozen-") as tmp:
        work = pathlib.Path(tmp)
        for workload_name, workload in protocol["workloads"].items():
            stdin_text = f"{max(int(workload['n'] * scale), 1)}\n"
            commands, toolchains, skipped = {}, [], {}
            for variant, spec in protocol["variants"].items():
                try:
                    commands[variant], record = build_variant(variant, spec, workload_name, work, shorthand)
                    toolchains.append(record)
                except (FileNotFoundError, subprocess.CalledProcessError, RuntimeError) as error:
                    skipped[variant] = f"{type(error).__name__}: {error}"
            if protocol["candidate_variant"] not in commands or protocol["reference_variant"] not in commands:
                raise SystemExit(f"candidate/reference variant could not be built: {skipped}")
            reference_out = run_once(commands[protocol["reference_variant"]], stdin_text, None, args.cpu)["stdout"]
            equivalent = True
            for variant, command in commands.items():
                if run_once(command, stdin_text, None, args.cpu)["stdout"] != reference_out:
                    equivalent = False
                    skipped[variant] = "stdout differs from reference"
            idle = idle_windows(meter, protocol["idle_windows"], args.idle_seconds) if meter else []
            idle_w = {s: idle_power_watts([(w["joules"][s], w["seconds"]) for w in idle]) for s in sources}
            per_variant: dict[str, list[dict[str, Any]]] = {v: [] for v in commands}
            for round_index in range(protocol["warmup_rounds"] + protocol["rounds"]):
                order = list(commands)
                rng.shuffle(order)
                for variant in order:
                    run = run_once(commands[variant], stdin_text, meter, args.cpu)
                    if run["returncode"] != 0:
                        raise SystemExit(f"{variant} failed with {run['returncode']}")
                    if round_index >= protocol["warmup_rounds"]:
                        per_variant[variant].append({
                            "round": round_index - protocol["warmup_rounds"],
                            "seconds": run["seconds"],
                            "gross_joules": run["joules"],
                            "net_joules": net_energy(sources, run, idle_w) if sources
                            else {"rapl": None, "ac_meter": None},
                        })
            summary = summarise(protocol, per_variant, sources, equivalent)
            agreement = None
            if len(sources) == 2:
                tol = protocol["ac_meter_agreement_tolerance"]
                a = statistics.median(r["net_joules"]["rapl"] for r in per_variant[protocol["candidate_variant"]])
                b = statistics.median(r["net_joules"]["ac_meter"] for r in per_variant[protocol["candidate_variant"]])
                agreement = {"rapl_vs_ac_ratio": a / b if b else None,
                             "within_tolerance": bool(b) and abs(a / b - 1.0) <= tol}
            reports.append({"workload": workload_name, "stdin_n": stdin_text.strip(), "scale": scale,
                            "outputs_equivalent": equivalent, "skipped_variants": skipped,
                            "toolchains": toolchains,
                            "idle_power_watts": idle_w, "source_agreement": agreement,
                            "summary": summary, "observations": per_variant})
    quick = bool(args.allow_unqualified and not physical) or scale != 1.0
    result = {
        "schema": SCHEMA,
        "kind": "host_run",
        "created": datetime.datetime.now(datetime.timezone.utc).isoformat(),
        "protocol_sha256": sha256_file(args.protocol),
        "host_id": qualification["host_id"],
        "host_qualified": physical,
        "energy_sources": sources,
        "evidence_class": "physical_counter" if (physical and not quick) else None,
        "joules": None if not sources else "see observations",
        "energy_claim_authorized": False,  # per-host runs never authorise; aggregate does
        "functional_time_dry_run": not physical,
        "scale": scale,
        "workloads": reports,
    }
    (out / "run.json").write_text(json.dumps(result, indent=2, sort_keys=True, default=str) + "\n")
    print(f"wrote {out/'run.json'} (qualified={physical}, sources={sources})")
    return 0


def command_aggregate(args: argparse.Namespace) -> int:
    protocol = load_protocol(args.protocol)
    runs = [json.loads(pathlib.Path(p).read_text()) for p in args.runs]
    reasons: list[str] = []
    eligible = []
    for run in runs:
        if run.get("schema") != SCHEMA or run.get("kind") != "host_run":
            reasons.append("an input is not a frozen-protocol host_run")
        elif not run["host_qualified"] or run["evidence_class"] != "physical_counter":
            reasons.append(f"host {run.get('host_id')} not qualified or not a full-scale physical run")
        elif run["protocol_sha256"] != sha256_file(args.protocol):
            reasons.append(f"host {run['host_id']} used a different protocol (not frozen)")
        else:
            eligible.append(run)
    hosts = {run["host_id"] for run in eligible}
    if len(hosts) < protocol["minimum_hosts_for_claim"]:
        reasons.append(f"need at least {protocol['minimum_hosts_for_claim']} distinct qualified hosts, have {len(hosts)}")
    verdicts: dict[str, dict[str, list[str]]] = {}
    for run in eligible:
        for workload in run["workloads"]:
            if workload["source_agreement"] and not workload["source_agreement"]["within_tolerance"]:
                reasons.append(f"{run['host_id']}/{workload['workload']}: RAPL and AC meter disagree")
            for baseline, sources in workload["summary"]["candidate_vs_baseline_energy_ratio"].items():
                for source, record in sources.items():
                    verdicts.setdefault(workload["workload"], {}).setdefault(baseline, []).append(record["verdict"])
    claims = {}
    for workload, baselines in verdicts.items():
        for baseline, values in baselines.items():
            ok = len(values) >= protocol["minimum_hosts_for_claim"] and all(v == "candidate_lower_energy" for v in values)
            claims[f"{workload}:{protocol['candidate_variant']}_vs_{baseline}"] = "supported" if ok else "not_supported"
    authorized = not reasons and bool(claims) and all(v == "supported" for v in claims.values())
    result = {
        "schema": SCHEMA, "kind": "aggregate", "hosts": sorted(hosts), "claims": claims,
        "blocking_reasons": reasons, "energy_claim_authorized": authorized,
        "statement": ("Candidate used less measured energy than every listed baseline on all qualified hosts for the "
                      "frozen workloads; this does not extend to other workloads or software-development workflows.")
        if authorized else "No energy claim is authorised.",
    }
    text = json.dumps(result, indent=2, sort_keys=True)
    if args.output:
        pathlib.Path(args.output).write_text(text + "\n")
    print(text)
    return 0


def parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = p.add_subparsers(dest="command", required=True)

    def common(sp: argparse.ArgumentParser) -> None:
        sp.add_argument("--protocol", type=pathlib.Path, default=HERE / "protocol.json")
        sp.add_argument("--powercap-root", default=str(POWERCAP_ROOT))
        sp.add_argument("--ac-meter-cmd", default=None,
                        help="shell command printing cumulative AC-meter joules as one number")

    q = sub.add_parser("qualify")
    common(q)
    q.add_argument("--output")
    q.set_defaults(func=command_qualify)
    r = sub.add_parser("run")
    common(r)
    r.add_argument("--output-dir", required=True)
    r.add_argument("--shorthand", default=str(ROOT / "build/short_hand"))
    r.add_argument("--scale", type=float, default=1.0, help="scale n (<1 for smoke tests; never claimable)")
    r.add_argument("--cpu", type=int, default=None, help="pin workload to this CPU")
    r.add_argument("--idle-seconds", type=float, default=5.0)
    r.add_argument("--allow-unqualified", action="store_true",
                   help="functional and wall-time dry run only; joules stay null")
    r.set_defaults(func=command_run)
    a = sub.add_parser("aggregate")
    a.add_argument("--protocol", type=pathlib.Path, default=HERE / "protocol.json")
    a.add_argument("--runs", nargs="+", required=True)
    a.add_argument("--output")
    a.set_defaults(func=command_aggregate)
    return p


def main(argv: list[str] | None = None) -> int:
    args = parser().parse_args(argv)
    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())
