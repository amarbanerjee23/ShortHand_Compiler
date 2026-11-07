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
energy or carbon reduction claim. Integration into separate cold/warm workload
captures and the hashed evidence bundle is the following checkpoint in PR126.

The mandatory native test injects failures at every descriptor position, checks
partial cleanup, normal/zero/multiplexed scheduling and cross-thread rejection,
and records live availability without requiring hardware access. An ordinary
build is checked for the absence of diagnostic symbols and syscalls. CI runs
these contracts with ASan/LSan/UBSan as well.

References: [Linux perf_event_open interface](https://man7.org/linux/man-pages/man2/perf_event_open.2.html),
[Linux performance-event access policy](https://docs.kernel.org/admin-guide/perf-security.html).
