"""Strict replay of diagnostic Linux PMU events, never joules or timing controls."""
import json

EVENTS = (("cpu_cycles", 0), ("instructions", 1), ("cache_misses", 3))
REASONS = {"open_failed", "id_failed", "enable_failed", "disable_failed", "read_failed",
           "short_read", "id_changed", "invalid_scheduling", "wrong_thread"}
SOURCES = ("PerfEventDiagnostics.h", "generated_infer_probe.cpp",
           "event_diagnostics.py", "generated_infer_evidence.py")


def uint(value):
    return type(value) is int and 0 <= value <= (1 << 64) - 1


def validate_boundary(row):
    fixed = dict(scope="calling_thread_user_space", worker_threads_included=False,
                 inherit=False, exclude_kernel=True, exclude_hv=True, cpu=-1,
                 pid_argument=0, counter_windows="independent_enable_disable")
    if (not isinstance(row, dict) or set(row) != set(fixed) | {"thread_id", "observer_elapsed_ns", "events"}
            or any(type(row.get(k)) is not type(v) or row[k] != v for k, v in fixed.items())
            or not uint(row.get("thread_id")) or not uint(row.get("observer_elapsed_ns"))
            or row["observer_elapsed_ns"] <= 0
            or not isinstance(row.get("events"), list) or len(row["events"]) != len(EVENTS)):
        raise ValueError("invalid event scope or observer boundary")
    ids = []
    for record, (name, config) in zip(row["events"], EVENTS):
        fields = {"name", "type", "config", "available", "reason", "errno", "event_id", "raw_count",
                  "time_enabled_ns", "time_running_ns", "multiplexed", "scaled_count_estimate"}
        if (not isinstance(record, dict) or set(record) != fields
                or record.get("name") != name or type(record.get("type")) is not int or record["type"] != 0
                or type(record.get("config")) is not int or record["config"] != config
                or type(record.get("available")) is not bool or not uint(record.get("errno"))
                or type(record.get("reason")) is not str or record["scaled_count_estimate"] is not None
                or any(record[k] is not None and not uint(record[k]) for k in
                       ("event_id", "raw_count", "time_enabled_ns", "time_running_ns"))):
            raise ValueError("invalid event identity or raw record")
        if record["event_id"] is not None:
            ids.append(record["event_id"])
        enabled, running = record["time_enabled_ns"], record["time_running_ns"]
        scheduled = enabled is not None and running is not None and 0 < running <= enabled
        if record["available"]:
            if (record["reason"] != "available" or record["errno"] != 0 or row["thread_id"] <= 0
                    or record["event_id"] is None or record["raw_count"] is None or not scheduled
                    or type(record["multiplexed"]) is not bool or record["multiplexed"] != (running < enabled)):
                raise ValueError("unqualified event scheduling")
        else:
            reason = record["reason"]
            if (reason not in REASONS or record["raw_count"] is not None or record["multiplexed"] is not None
                    or (record["errno"] > 0) != reason.endswith("_failed")
                    or (record["event_id"] is None) != (reason in {"open_failed", "id_failed"})):
                raise ValueError("unavailable event must not contain a count or conflicting status")
            if reason == "invalid_scheduling":
                if enabled is None or running is None or scheduled:
                    raise ValueError("missing invalid scheduling observation")
            elif enabled is not None or running is not None:
                raise ValueError("failed event contains unverified scheduling")
    if len(set(ids)) != len(ids):
        raise ValueError("duplicate event IDs")
    return row


def validate_sample(sample, batch, iterations=32, blocks=2):
    fixed = dict(schema="shorthand.generated_infer.events.v1", success=True, diagnostic_only=True,
                 instrumented=True, energy_claim_authorized=False, latency_claim_authorized=False,
                 hardware_measured_joules=None, batch=batch, iterations=iterations,
                 completed_calls=iterations * blocks, threads=1, warmups=8, cold_calls=1, empty_calls=0)
    if (not isinstance(sample, dict) or set(sample) != set(fixed) | {"cold_boundary", "empty_boundary", "warm_blocks"}
            or any(type(sample.get(k)) is not type(v) or sample[k] != v for k, v in fixed.items())
            or not isinstance(sample["warm_blocks"], list) or len(sample["warm_blocks"]) != blocks):
        raise ValueError("invalid diagnostic event sample or execution count")
    rows = [sample["empty_boundary"], sample["cold_boundary"], *sample["warm_blocks"]]
    for row in rows:
        validate_boundary(row)
    if len({row["thread_id"] for row in rows}) != 1:
        raise ValueError("capture thread changed")
    return sample


def replay(out, manifest, digest):
    """Called only after the enclosing generated-infer bundle verifies all hashes."""
    records = manifest.get("event_profiles")
    hashes = manifest["hashes"]
    if (not isinstance(records, list) or len(records) != 3
            or any(not isinstance(r, dict) or set(r) != {"cell", "path"}
                   or type(r["cell"]) is not str or type(r["path"]) is not str
                   or r["path"] not in hashes for r in records)
            or {r["cell"] for r in records} != {"b1-t1", "b16-t1", "b32-t1"}
            or len({r["path"] for r in records}) != 3):
        raise ValueError("missing or duplicate event captures")
    other = {r["path"] for name in ("observations", "profiles", "observer_observations") for r in manifest.get(name, [])}
    if other & {r["path"] for r in records}:
        raise ValueError("event capture used as a latency control")
    for name in SOURCES:
        path = "event-sources/" + name
        if path not in hashes or not (out / path).read_text().strip():
            raise ValueError("missing event source snapshot")
    rows = []
    for record in records:
        batch = int(record["cell"][1:-3])
        sample = validate_sample(json.loads((out / record["path"]).read_text()), batch)
        rows.append(dict(cell=record["cell"], sample=sample))
    return dict(schema="shorthand.generated_infer.event_report.v1", success=True,
                manifest_sha256=digest, rows=rows, diagnostic_only=True,
                latency_claim_authorized=False, energy_claim_authorized=False,
                hardware_measured_joules=None,
                boundary="calling thread user-space events only; other threads and kernel excluded; "
                         "independent counter windows include observer work; cold and warm boundaries separate; "
                         "empty observer control retained without subtraction; cache_misses is not an LLC metric")


def validate_report(report):
    fixed = dict(schema="shorthand.generated_infer.event_report.v1", success=True, diagnostic_only=True,
                 latency_claim_authorized=False, energy_claim_authorized=False, hardware_measured_joules=None)
    if (not isinstance(report, dict) or set(report) != set(fixed) | {"rows", "manifest_sha256", "boundary"}
            or any(type(report.get(k)) is not type(v) or report[k] != v for k, v in fixed.items())
            or type(report["manifest_sha256"]) is not str or len(report["manifest_sha256"]) != 64
            or any(c not in "0123456789abcdef" for c in report["manifest_sha256"])
            or type(report["boundary"]) is not str or not report["boundary"]
            or not isinstance(report["rows"], list) or len(report["rows"]) != 3):
        raise ValueError("invalid diagnostic event report")
    cells = []
    for row in report["rows"]:
        if not isinstance(row, dict) or set(row) != {"cell", "sample"} or row["cell"] not in ("b1-t1", "b16-t1", "b32-t1"):
            raise ValueError("invalid event report cell")
        cells.append(row["cell"])
        validate_sample(row["sample"], int(row["cell"][1:-3]))
    if len(set(cells)) != 3:
        raise ValueError("duplicate event report cells")
    return report
