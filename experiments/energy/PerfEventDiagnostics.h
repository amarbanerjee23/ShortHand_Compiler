#pragma once
// Experiment executable only: no PMU code or syscalls in ordinary builds.
#if defined(SHORTHAND_RUNTIME_EVENT_PROFILE) && SHORTHAND_RUNTIME_EVENT_PROFILE
#include <array>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <optional>
#include <ostream>
#include <stdexcept>
#if defined(__linux__)
#include <linux/perf_event.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <unistd.h>
#endif

namespace shorthand::event_profile {
enum class Control { id, enable, disable };
struct Event { const char *name; std::uint64_t config; };
inline constexpr std::array<Event,3> events{{{"cpu_cycles",0},{"instructions",1},{"cache_misses",3}}};
// Independent events retain their own scheduling times. No ratios or scaled
// estimates are calculated. Generic cache_misses is not an LLC measurement.
struct LinuxPerf {
    std::uint64_t thread() const noexcept {
#if defined(__linux__)
        return static_cast<std::uint64_t>(::syscall(SYS_gettid));
#else
        return 0;
#endif
    }
    int open(std::uint64_t config,int &error) const noexcept {
#if defined(__linux__)
        static_assert(PERF_COUNT_HW_CPU_CYCLES==0 && PERF_COUNT_HW_INSTRUCTIONS==1 && PERF_COUNT_HW_CACHE_MISSES==3);
        perf_event_attr attr{};
        attr.size=sizeof(attr); attr.type=PERF_TYPE_HARDWARE; attr.config=config;
        attr.disabled=1; attr.inherit=0; attr.exclude_kernel=1; attr.exclude_hv=1;
        attr.read_format=PERF_FORMAT_TOTAL_TIME_ENABLED|PERF_FORMAT_TOTAL_TIME_RUNNING|PERF_FORMAT_ID;
        const int fd=static_cast<int>(::syscall(SYS_perf_event_open,&attr,0,-1,-1,PERF_FLAG_FD_CLOEXEC));
        error=fd<0?errno:0; return fd;
#else
        (void)config; error=ENOSYS; return -1;
#endif
    }
    bool control(int fd,Control op,std::uint64_t *id,int &error) const noexcept {
#if defined(__linux__)
        const int result=op==Control::id ? ::ioctl(fd,PERF_EVENT_IOC_ID,id) :
            ::ioctl(fd,op==Control::enable?PERF_EVENT_IOC_ENABLE:PERF_EVENT_IOC_DISABLE,0);
        error=result<0?errno:0; return result==0;
#else
        (void)fd; (void)op; (void)id; error=ENOSYS; return false;
#endif
    }
    long read(int fd,std::array<std::uint64_t,4> &data,int &error) const noexcept {
#if defined(__linux__)
        const auto size=::read(fd,data.data(),sizeof(data)); error=size<0?errno:0; return size;
#else
        (void)fd; (void)data; error=ENOSYS; return -1;
#endif
    }
    void close(int fd) const noexcept {
#if defined(__linux__)
        ::close(fd); // Do not retry EINTR: a descriptor could already be reused.
#else
        (void)fd;
#endif
    }
};
struct Reading {
    bool available=false;
    const char *reason="not_started";
    int error=0;
    std::optional<std::uint64_t> id,raw,enabled,running;
};
struct Sample {
    std::uint64_t thread_id=0,observer_elapsed_ns=0;
    std::array<Reading,events.size()> readings{};
};
template<class Backend=LinuxPerf>
class Capture {
    using Clock=std::chrono::steady_clock;
    Backend backend_;
    Sample result_;
    std::array<int,events.size()> descriptors_{{-1,-1,-1}};
    Clock::time_point start_=Clock::now();
    bool finished_=false;
    void close(std::size_t i) noexcept {
        if(descriptors_[i]>=0) { backend_.close(descriptors_[i]); descriptors_[i]=-1; }
    }
    void fail(std::size_t i,const char *reason,int error=0) noexcept {
        auto &r=result_.readings[i]; r.available=false; r.reason=reason; r.error=error; r.raw.reset();
    }
public:
    explicit Capture(Backend backend={}):backend_(backend) {
        result_.thread_id=backend_.thread();
        for(std::size_t i=0;i<events.size();++i) {
            int error=0; auto &r=result_.readings[i];
            const int fd=descriptors_[i]=backend_.open(events[i].config,error);
            if(fd<0) { fail(i,"open_failed",error); continue; }
            std::uint64_t id=0;
            if(!backend_.control(fd,Control::id,&id,error)) { fail(i,"id_failed",error); close(i); continue; }
            r.id=id;
        }
        for(std::size_t i=0;i<events.size();++i) if(descriptors_[i]>=0) {
            int error=0;
            if(!backend_.control(descriptors_[i],Control::enable,nullptr,error)) {
                fail(i,"enable_failed",error); close(i);
            }
        }
    }
    Capture(const Capture &)=delete;
    Capture &operator=(const Capture &)=delete;
    ~Capture() { for(std::size_t i=0;i<events.size();++i) close(i); }
    Sample finish() {
        if(finished_) throw std::logic_error("event_capture_already_finished");
        finished_=true;
        const bool sameThread=result_.thread_id==backend_.thread();
        for(std::size_t i=0;i<events.size();++i) if(descriptors_[i]>=0) {
            int error=0;
            if(!sameThread) { fail(i,"wrong_thread"); close(i); }
            else if(!backend_.control(descriptors_[i],Control::disable,nullptr,error)) {
                fail(i,"disable_failed",error); close(i);
            }
        }
        for(std::size_t i=0;i<events.size();++i) if(descriptors_[i]>=0) {
            int error=0; std::array<std::uint64_t,4> data{};
            const auto count=backend_.read(descriptors_[i],data,error);
            auto &r=result_.readings[i];
            if(count<0) fail(i,"read_failed",error);
            else if(count!=static_cast<long>(sizeof(data))) fail(i,"short_read");
            else if(data[3]!=*r.id) fail(i,"id_changed");
            else {
                r.enabled=data[1]; r.running=data[2];
                if(!data[1] || !data[2] || data[2]>data[1]) fail(i,"invalid_scheduling");
                else { r.raw=data[0]; r.available=true; r.reason="available"; }
            }
            close(i);
        }
        result_.observer_elapsed_ns=static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now()-start_).count());
        return result_;
    }
};
inline void json(std::ostream &out,const Sample &sample) {
    auto value=[&](const std::optional<std::uint64_t> &v) { if(v) out<<*v; else out<<"null"; };
    out<<"{\"scope\":\"calling_thread_user_space\",\"worker_threads_included\":false,\"inherit\":false"
       <<",\"exclude_kernel\":true,\"exclude_hv\":true,\"cpu\":-1,\"pid_argument\":0"
       <<",\"thread_id\":"<<sample.thread_id<<",\"observer_elapsed_ns\":"<<sample.observer_elapsed_ns
       <<",\"counter_windows\":\"independent_enable_disable\",\"events\":[";
    for(std::size_t i=0;i<events.size();++i) {
        const auto &r=sample.readings[i];
        out<<(i?",":"")<<"{\"name\":\""<<events[i].name<<"\",\"type\":0,\"config\":"<<events[i].config
           <<",\"available\":"<<(r.available?"true":"false")<<",\"reason\":\""<<r.reason<<"\",\"errno\":"<<r.error;
        out<<",\"event_id\":"; value(r.id); out<<",\"raw_count\":"; value(r.raw);
        out<<",\"time_enabled_ns\":"; value(r.enabled); out<<",\"time_running_ns\":"; value(r.running);
        out<<",\"multiplexed\":";
        if(r.available) out<<(*r.running<*r.enabled?"true":"false"); else out<<"null";
        out<<",\"scaled_count_estimate\":null}";
    }
    out<<"]}";
}
}
#endif
