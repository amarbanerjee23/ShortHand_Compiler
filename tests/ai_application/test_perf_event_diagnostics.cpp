#include "PerfEventDiagnostics.h"
#include <iostream>
#include <string>
#include <utility>
using namespace shorthand::event_profile;
namespace {
void require(bool value) { if(!value) throw std::runtime_error("PMU diagnostic contract failed"); }
struct State {
    int next=0,closed=0,fail=-1;
    std::string failure;
    std::uint64_t tid=7,enabled=100,running=50,raw=12;
    std::array<bool,3> live{};
};
struct Fake {
    State *state;
    std::uint64_t thread() const { return state->tid; }
    int open(std::uint64_t,int &error) const {
        const int i=state->next++;
        if(i==state->fail && state->failure=="open") { error=EACCES; return -1; }
        state->live[i]=true; error=0; return i;
    }
    bool control(int fd,Control op,std::uint64_t *id,int &error) const {
        const std::string name=op==Control::id?"id":op==Control::enable?"enable":"disable";
        if(fd==state->fail && state->failure==name) { error=EPERM; return false; }
        if(id) *id=100+fd;
        error=0; return true;
    }
    long read(int fd,std::array<std::uint64_t,4> &data,int &error) const {
        error=0;
        if(fd==state->fail && state->failure=="read") { error=EIO; return -1; }
        data={state->raw,state->enabled,state->running,static_cast<std::uint64_t>(100+fd)};
        if(fd==state->fail && state->failure=="short") return sizeof(data)-1;
        if(fd==state->fail && state->failure=="changed") ++data[3];
        return sizeof(data);
    }
    void close(int fd) const { require(state->live[fd]); state->live[fd]=false; ++state->closed; }
};
}
int main() {
    for(const auto *failure:{"open","id","enable","disable","read","short","changed"}) {
        for(int i=0;i<3;++i) {
            State state; state.failure=failure; state.fail=i;
            { Capture<Fake> capture{Fake{&state}}; const auto sample=capture.finish();
              for(int j=0;j<3;++j) require(sample.readings[j].available==(i!=j));
              require(!sample.readings[i].raw);
              bool rejected=false; try { capture.finish(); } catch(const std::logic_error &) { rejected=true; }
              require(rejected);
            }
            require(state.closed==(state.failure=="open"?2:3));
        }
    }
    for(const auto &pair:{std::pair<std::uint64_t,std::uint64_t>{0,0},{100,0},{100,101},{100,100},{100,50}}) {
        State state; state.enabled=pair.first; state.running=pair.second; state.raw=0;
        Capture<Fake> capture{Fake{&state}}; const auto sample=capture.finish();
        for(const auto &r:sample.readings) {
            require(r.available==(pair.second>0 && pair.second<=pair.first));
            require(r.enabled==pair.first && r.running==pair.second);
            if(r.available) require(r.raw==0);
        }
        require(state.closed==3);
    }
    { State state; { Capture<Fake> capture{Fake{&state}}; } require(state.closed==3); }
    { State state; try { Capture<Fake> capture{Fake{&state}}; throw 1; } catch(int) {} require(state.closed==3); }
    { State state; Capture<Fake> capture{Fake{&state}}; state.tid=8; const auto result=capture.finish();
      for(const auto &r:result.readings) require(!r.available && std::string(r.reason)=="wrong_thread");
      require(state.closed==3); }
    // A real capability observation may be unavailable. Never elevate privileges.
    Capture<> capture;
    volatile std::uint64_t total=0;
    for(std::uint64_t i=0;i<100000;++i) total+=i;
    require(total==4999950000ULL);
    json(std::cout,capture.finish()); std::cout<<'\n';
}
