#pragma once

#include <Core/Core.h>

namespace Vortex {

namespace NonCopyable_ {
class NonCopyable {
   public:
    NonCopyable() = default;
    ~NonCopyable() = default;
    NonCopyable(const NonCopyable&) = delete;
    void operator=(const NonCopyable&) = delete;
};
}  // namespace NonCopyable_
typedef NonCopyable_::NonCopyable NonCopyable;

};  // namespace Vortex