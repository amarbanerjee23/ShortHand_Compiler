# Diagnostic performance events

`experiments/energy/PerfEventDiagnostics.h` is an opt-in experiment collector,
compiled only with `SHORTHAND_RUNTIME_EVENT_PROFILE=1`. It is not part of the
installed language runtime or its frozen C ABI. The ordinary configuration
preprocesses the implementation away.

The Linux collector requests hardware CPU cycles, instructions and generic cache
misses, preserving the event type/configuration and kernel event ID. It measures
only the calling thread's user-space execution on any CPU: `pid=0`, `cpu=-1`,
`inherit=0`, `exclude_kernel=1`, `exclude_hv=1`. Other application/runtime worker
threads are excluded, including workers that already existed when capture began.
The report explicitly records this scope. Generic cache misses are not labelled
LLC misses; their meaning depends on the CPU's event mapping.

Each event opens independently and retains its own raw count, enabled time and
running time. Enable/disable operations are sequential, so the events do not
share an exact common window and are not used to calculate event ratios. A raw
count with reduced running time is marked multiplexed. No scaled count estimate
is produced. Zero counts are possible valid observations; missing counters use
null counts, with an operation-specific reason and errno instead of zeros.

Permission denial, unsupported events, failed enable/disable/read, short reads,
changed IDs, zero running time and invalid scheduling remain unavailable. Other
events can still succeed. The collector does not change machine permissions or
request privilege elevation. Descriptors use close-on-exec and close on normal
completion, partial setup failure or exception unwinding. Reusing a finished
capture is rejected; finishing on a different thread invalidates the capture.

The enclosing observer duration includes setup, controls, reads and cleanup.
It is not kernel execution time. PMU counts are performance diagnostics, not
joules, watts or calibrated energy estimates. They cannot authorize a latency,
energy or carbon reduction claim.

The generated-inference capture builds a distinct diagnostic executable, with
phase profiling disabled, from the retained probe/header snapshots. It runs the
same numerically checked workload for batches 1, 16 and 32. Each cell retains an
empty observer control, one cold call (including reset/registration/preparation),
eight warmups outside the captures, and two independent blocks of 32 verified
warm calls. The empty duration is retained without subtraction. Event capture
includes observer work around the declared boundaries. These executions never
enter uninstrumented latency ratios, phase profiles or component-energy runs.

Bundle schema `shorthand.generated_infer.bundle.v3` requires hashed raw event
captures and collector/probe/replay source snapshots. Earlier v2 bundles remain
replayable without claiming that they contain these diagnostics. The new
`shorthand.generated_infer.events.v1` raw schema and
`shorthand.generated_infer.event_report.v1` report leave existing energy and
signed-evidence schemas unchanged. Replay rejects invalid scope, missing or
duplicate cells, counts with invalid scheduling, fabricated unavailable counts,
claim changes, control contamination and missing source snapshots. CI requires
the resulting `generated-infer/events.json`, deep-validates its records, and
renders availability and errors in `BENCHMARKS.md`. All counters being unavailable
is a valid capability observation; missing or malformed execution evidence fails.

The mandatory native test injects failures at every descriptor position, checks
partial cleanup, normal/zero/multiplexed scheduling and cross-thread rejection,
and records live availability without requiring hardware access. An ordinary
build is checked for the absence of diagnostic symbols and syscalls. CI runs
these contracts with ASan/LSan/UBSan as well.

References: [Linux perf_event_open interface](https://man7.org/linux/man-pages/man2/perf_event_open.2.html),
[Linux performance-event access policy](https://docs.kernel.org/admin-guide/perf-security.html).
