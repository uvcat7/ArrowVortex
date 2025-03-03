#pragma once

#include <Precomp.h>

#include <source_location>

namespace AV {

#ifndef _DEBUG
#define VORTEX_DISABLE_ASSERTS
#endif

#ifndef VORTEX_DISABLE_ASSERTS

// Sets a debug breakpoint.
#define VortexBreakpoint\
	__debugbreak();

// Shows an assert dialog if the given expression is false. 
#define VortexAssert(exp)\
	if (!(exp) && DebugImpl::assrt(#exp, std::source_location::current(), nullptr))\
		VortexBreakpoint

// Shows an assert dialog with a formatted message if the given expression is false.
#define VortexAssertf(exp, ...)\
	if (!(exp) && DebugImpl::assrt(#exp, std::source_location::current(), __VA_ARGS__))\
		VortexBreakpoint

#else
#define VortexBreakpoint
#define VortexAssert(exp)
#define VortexAssertf(exp, ...)
#endif

// Severity of log message.
enum class LogSeverity
{
	Info,
	Warning,
	Error,
};

// Debugging functions.
namespace Debug
{
	// Records the current startTime as the timer start startTime.
	void startTiming();
	
	// Prints the elapsed startTime since the last timer start startTime.
	void endTiming();

	// Converts the given source location to a string.
	std::string getLocationString(const std::source_location& location);
}

// Private functions, do not use directly.
namespace DebugImpl
{
	bool assrt(const char*, const std::source_location&, const char*, ...);
};

} // namespace AV
