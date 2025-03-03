#pragma once

#include <Precomp.h>

namespace AV {

// Base class for objects that can not be copied.
class NonCopyable
{
public:
	NonCopyable(const NonCopyable&) = delete;
	void operator=(const NonCopyable&) = delete;
protected:
	NonCopyable() = default;
	~NonCopyable() = default;
};

} // namespace AV