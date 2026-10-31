#pragma once
// Private bounded executor. The owning trainer publishes a batch before run(),
// waits for all workers, then reduces gradients in sample order. Tasks must not
// recursively call this executor or destroy it; task state outlives the executor.
#include <condition_variable>
#include <cstdint>
#include <exception>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

namespace shorthand::ai::training_detail {
class PersistentWorkers {
public:
    using Factory=std::function<std::thread(std::function<void()>)>;
    PersistentWorkers(unsigned count,std::function<void(unsigned)> task,Factory factory={})
        : task_(std::move(task)) {
        if(!count || count>128 || !task_) throw std::invalid_argument("invalid_training_workers");
        errors_.resize(count);
        if(!factory) factory=[](std::function<void()> work) { return std::thread(std::move(work)); };
        workers_.reserve(count);
        try {
            for(unsigned i=0;i<count;++i) {
                workers_.push_back(factory([this,i] { worker(i); }));
                if(!workers_.back().joinable()) throw std::runtime_error("training_worker_not_started");
            }
        } catch(...) { stop(); throw; }
    }
    PersistentWorkers(const PersistentWorkers &)=delete;
    PersistentWorkers &operator=(const PersistentWorkers &)=delete;
    ~PersistentWorkers() { stop(); }
    void run() {
        std::lock_guard<std::mutex> client(client_);
        std::unique_lock<std::mutex> lock(mutex_);
        if(stopping_) throw std::runtime_error("training_workers_stopped");
        for(auto &error:errors_) error=nullptr;
        remaining_=workers_.size(); ++generation_;
        ready_.notify_all();
        done_.wait(lock,[this] { return remaining_==0; });
        for(const auto &error:errors_) if(error) std::rethrow_exception(error);
    }
    // Cooperative cancellation at the batch boundary. In-flight bounded tasks
    // finish before joining; no detached worker retains model/workspace pointers.
    void stop() noexcept {
        std::lock_guard<std::mutex> client(client_);
        { std::lock_guard<std::mutex> lock(mutex_); stopping_=true; }
        ready_.notify_all();
        for(auto &thread:workers_) if(thread.joinable()) thread.join();
    }
    std::size_t size() const { return workers_.size(); }
private:
    void worker(unsigned index) {
        std::uint64_t observed=0;
        std::unique_lock<std::mutex> lock(mutex_);
        for(;;) {
            ready_.wait(lock,[&] { return stopping_ || generation_!=observed; });
            if(stopping_) return;
            observed=generation_; lock.unlock();
            std::exception_ptr error;
            try { task_(index); } catch(...) { error=std::current_exception(); }
            lock.lock(); errors_[index]=error;
            if(--remaining_==0) done_.notify_one();
        }
    }
    std::function<void(unsigned)> task_;
    std::vector<std::exception_ptr> errors_;
    std::vector<std::thread> workers_;
    std::mutex mutex_,client_;
    std::condition_variable ready_,done_;
    std::uint64_t generation_=0;
    std::size_t remaining_=0;
    bool stopping_=false;
};
}
