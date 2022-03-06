#pragma once

#include <nadjieb/utils/non_copyable.hpp>

#include <condition_variable>
#include <mutex>

// Reference: https://en.wikipedia.org/wiki/Readers%E2%80%93writer_lock#Using_a_condition_variable_and_a_mutex

namespace nadjieb {
namespace utils {
class ReadWriteLock : public NonCopyable {
   public:
    void readLock() {
        std::unique_lock<std::mutex> lk(g_);
        cond_.wait(lk, [&]() { return (num_writers_waiting_ == 0 && !writer_active_); });
        ++num_readers_active_;
    }

    void readUnlock() {
        std::unique_lock<std::mutex> lk(g_);
        --num_readers_active_;
        if (num_readers_active_ == 0) {
            cond_.notify_all();
        }
    }

    void writeLock() {
        std::unique_lock<std::mutex> lk(g_);
        ++num_writers_waiting_;
        cond_.wait(lk, [&]() { return (num_readers_active_ == 0 && !writer_active_); });
        --num_writers_waiting_;
        writer_active_ = true;
    }

    void writeUnlock() {
        std::unique_lock<std::mutex> lk(g_);
        writer_active_ = false;
        cond_.notify_all();
    }

   private:
    std::mutex g_;
    int num_readers_active_ = 0;
    int num_writers_waiting_ = 0;
    bool writer_active_ = false;
    std::condition_variable cond_;
};
}  // namespace utils
}  // namespace nadjieb
