#pragma once

namespace nadjieb {
namespace utils {
enum class State { UNSPECIFIED = 0, NEW, BOOTING, RUNNING, TERMINATING, TERMINATED };
class Runnable {
   public:
    State status() { return state_; }

    bool isRunning() { return (state_ == State::RUNNING); }

   protected:
    State state_ = State::NEW;
};
}  // namespace utils
}  // namespace nadjieb
