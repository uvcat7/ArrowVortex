#pragma once

#include <Core/Core.h>

namespace Vortex {

namespace NonCopyable_
{
	class NonCopyable
	{
	protected:
		NonCopyable(){}
		~NonCopyable(){}
	private:
		NonCopyable(const NonCopyable&);
		void operator=(const NonCopyable&);
	};
}
typedef NonCopyable_::NonCopyable NonCopyable;

}; // namespace Vortex