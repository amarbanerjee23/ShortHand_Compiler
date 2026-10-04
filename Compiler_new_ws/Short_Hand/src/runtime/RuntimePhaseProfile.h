#ifndef SHORTHAND_RUNTIME_PHASE_PROFILE_H
#define SHORTHAND_RUNTIME_PHASE_PROFILE_H

// Diagnostic-only instrumentation. Normal targets preprocess every hook away;
// there is no production clock, TLS lookup, environment lookup or C ABI change.
#if defined(SHORTHAND_RUNTIME_PHASE_PROFILE) && SHORTHAND_RUNTIME_PHASE_PROFILE
#include <array>
#include <chrono>
#include <cstdint>
#include <stdexcept>

namespace shorthand::runtime_profile {
enum class Phase {
    entry_residual, facade_lock, registration, bridge_validation, descriptor_input,
    policy_refresh, hardware_probe, routing, runtime_validation, snapshot,
    session_prepare, backend_other, ort_run, runtime_telemetry, bridge_output,
    bridge_telemetry_log, oracle, count
};
inline constexpr std::array<const char *, static_cast<unsigned>(Phase::count)> names = {
    "entry_residual", "facade_lock", "registration", "bridge_validation", "descriptor_input",
    "policy_refresh", "hardware_probe", "routing", "runtime_validation", "snapshot",
    "session_prepare", "backend_other", "ort_run", "runtime_telemetry", "bridge_output",
    "bridge_telemetry_log", "oracle"
};
struct Sample {
    std::array<std::uint64_t, names.size()> ns{};
    std::array<unsigned, names.size()> visits{};
    std::uint64_t total_ns=0, clock_reads=1;
};
class Recorder;
inline thread_local Recorder *active=nullptr;

// Exclusive accounting: nested phases suspend their parent. The partition
// includes instrumentation overhead; no clock cost is subtracted or hidden.
class Recorder {
    friend class Scope;
    using Clock=std::chrono::steady_clock;
    Sample sample_;
    Clock::time_point start_, last_;
    Phase phase_=Phase::entry_residual;
    unsigned depth_=0;
    void transition(Phase next) noexcept {
        const auto now=Clock::now();
        sample_.ns[static_cast<unsigned>(phase_)]+=static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(now-last_).count());
        ++sample_.clock_reads;
        last_=now; phase_=next;
    }
public:
    Recorder() {
        if (active) throw std::logic_error("nested_runtime_profile_capture");
        active=this; start_=last_=Clock::now();
    }
    Recorder(const Recorder &)=delete;
    Recorder &operator=(const Recorder &)=delete;
    ~Recorder() { if (active==this) active=nullptr; }
    Sample finish() {
        if (active!=this || depth_) throw std::logic_error("unclosed_runtime_profile_scope");
        transition(Phase::entry_residual);
        sample_.total_ns=static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(last_-start_).count());
        active=nullptr;
        return sample_;
    }
};
class Scope {
    Recorder *recorder_=active;
    Phase previous_=Phase::entry_residual;
public:
    explicit Scope(Phase phase) noexcept {
        if (recorder_) {
            previous_=recorder_->phase_; ++recorder_->depth_; next(phase);
        }
    }
    Scope(const Scope &)=delete;
    Scope &operator=(const Scope &)=delete;
    void next(Phase phase) noexcept {
        if (recorder_) {
            recorder_->transition(phase);
            ++recorder_->sample_.visits[static_cast<unsigned>(phase)];
        }
    }
    ~Scope() {
        if (recorder_) { recorder_->transition(previous_); --recorder_->depth_; }
    }
};
}
#define SHORTHAND_PHASE_SCOPE(phase) \
    shorthand::runtime_profile::Scope shorthand_phase_scope(shorthand::runtime_profile::Phase::phase)
#define SHORTHAND_PHASE_NEXT(phase) \
    shorthand_phase_scope.next(shorthand::runtime_profile::Phase::phase)
#else
#define SHORTHAND_PHASE_SCOPE(phase) ((void)0)
#define SHORTHAND_PHASE_NEXT(phase) ((void)0)
#endif
#endif
