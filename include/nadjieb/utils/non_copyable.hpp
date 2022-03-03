#pragma once

namespace nadjieb {
namespace utils {
class NonCopyable {
   public:
    NonCopyable(NonCopyable&&) = delete;
    NonCopyable(const NonCopyable&) = delete;
    NonCopyable& operator=(NonCopyable&&) = delete;
    NonCopyable& operator=(const NonCopyable&) = delete;

   protected:
    NonCopyable() = default;
    virtual ~NonCopyable() = default;
};
}  // namespace utils
}  // namespace nadjieb
