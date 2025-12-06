#pragma once

#include <Core/Core.h>
#include <assert.h>
#include <chrono>

#ifdef NDEBUG
#define VORTEX_DISABLE_ASSERTS
#define VORTEX_DISABLE_CHECKPOINTS
#endif

namespace Vortex {

namespace Debug {
enum Type { INFO, WARNING, ERROR };

/// Returns a timestamp of the current time.
std::chrono::steady_clock::time_point getElapsedTime();

/// Returns the number of seconds elapsed since the start time.
double getElapsedTime(std::chrono::steady_clock::time_point startTime);

/// Creates a blank log file.
void openLogFile();

/// Opens a debug console to which log messages are written.
void openConsole();

/// Writes a message to the log.
void log(const char* fmt, ...);

/// Writes a blank line before the next log message.
void logBlankLine();

/// Starts an error message block.
void blockBegin(Type type, const char* title);

/// Ends the current error message block.
void blockEnd();
};  // namespace Debug

#ifndef VORTEX_DISABLE_CHECKPOINTS

/// Sets a debug breakpoint.
#define VortexCheckpoint(x) DebugPrivate::check(x)

#else  // VORTEX_DISABLE_CHECKPOINTS

#define VortexCheckpoint(x)

#endif  // VORTEX_DISABLE_CHECKPOINTS

#ifndef VORTEX_DISABLE_ASSERTS

/// Shows an assert dialog if the given expression is false.
#define VortexAssert(exp)                                                     \
    if (!(exp) &&                                                             \
        DebugPrivate::assrt(#exp, __FILE__, __LINE__, __FUNCTION__, nullptr)) \
        assert(false);

/// Shows an assert dialog with a formatted message if the given expression is
/// false.
#define VortexAssertFmt(exp, ...)                                             \
    if (!(exp) && DebugPrivate::assrt(#exp, __FILE__, __LINE__, __FUNCTION__, \
                                      __VA_ARGS__))                           \
        assert(false);

#else  // VORTEX_DISABLE_ASSERTS

#define VortexAssert(exp)
#define VortexAssertFmt(exp, ...)

#endif  // VORTEX_DISABLE_ASSERTS

/// Checks if the last error code of OpenGL is non-zero, and if so, logs it.
#define VortexCheckGlError() \
    DebugPrivate::glerr(__FILE__, __LINE__, __FUNCTION__)

// Private functions, do not use directly.
namespace DebugPrivate {
bool assrt(const char*, const char*, int, const char*, const char*, ...);
bool glerr(const char*, int, const char*);
void check(const char*);
};  // namespace DebugPrivate

};  // namespace Vortex