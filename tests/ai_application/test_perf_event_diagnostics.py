"""Fault-injected PMU lifecycle, live availability, and ordinary-build isolation."""
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "experiments/energy"))
from event_diagnostics import validate_boundary


def run(*args):
    result = subprocess.run(list(map(str, args)), text=True, capture_output=True, timeout=90)
    if result.returncode or any(x in result.stderr for x in ("AddressSanitizer", "LeakSanitizer", "runtime error:")):
        raise RuntimeError((args, result.returncode, result.stdout, result.stderr))
    return result.stdout


def main():
    flags = [os.environ.get("CXX", "c++"), "-std=c++17", "-O2", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
             "-I" + str(ROOT / "experiments/energy")]
    with tempfile.TemporaryDirectory(prefix="shorthand-events-") as tmp:
        work = Path(tmp)
        source = ROOT / "tests/ai_application/test_perf_event_diagnostics.cpp"
        configurations = [([], "normal")]
        if os.environ.get("SHORTHAND_EVENT_SANITIZERS") == "ON":
            configurations.append((["-fsanitize=address,undefined", "-fno-omit-frame-pointer"], "sanitized"))
        for sanitizers, label in configurations:
            binary = work / label
            run(*flags, *sanitizers, "-DSHORTHAND_RUNTIME_EVENT_PROFILE=1", source, "-o", binary)
            data = json.loads(run(binary))
            validate_boundary(data)
            if data["scope"] != "calling_thread_user_space" or data["worker_threads_included"] is not False:
                raise ValueError("incorrect event thread scope")
            for event in data["events"]:
                if event["available"]:
                    if not 0 < event["time_running_ns"] <= event["time_enabled_ns"]:
                        raise ValueError("invalid scheduling")
                    if event["name"] != "cache_misses" and event["raw_count"] <= 0:
                        raise ValueError("live arithmetic did not increment event")
                elif event["raw_count"] is not None or not event["reason"]:
                    raise ValueError("unavailable event fabricated a count")
            print("Live PMU capability (" + label + "): " + json.dumps(data, sort_keys=True))
        ordinary = work / "ordinary.cpp"
        ordinary.write_text('#include "PerfEventDiagnostics.h"\nint main() { return 0; }\n')
        for define in ([], ["-DSHORTHAND_RUNTIME_EVENT_PROFILE=0"]):
            binary = work / "ordinary"
            run(*flags, *define, ordinary, "-o", binary)
            symbols = run("nm", "-C", binary)
            if any(x in symbols for x in ("shorthand::event_profile::", "syscall", "ioctl")):
                raise ValueError("event profiling leaked into ordinary build")
            run(binary)
    print("PASS event diagnostics: permission/fault paths, scheduling, descriptor lifecycle, live availability and isolation")


if __name__ == "__main__":
    main()
