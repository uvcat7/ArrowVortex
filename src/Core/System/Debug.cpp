#include <Precomp.h>

#include <io.h>
#include <fcntl.h>

#include <Core/Utils/Unicode.h>
#include <Core/System/System.h>
#include <Core/System/Debug.h>
#include <Core/System/Log.h>

#define UNICODE
#define WIN32_LEAN_AND_MEAN
#include "windows.h"

namespace AV {

using namespace std;

// Timing.

static double TimerStartTime = 0.0;

// Ignored errors.

static constexpr int MaxIgnoredErrorLen = MAX_PATH + 16;
static constexpr int MaxIgnoredErrors = 32;

static char IgnoredErrorList[MaxIgnoredErrors][MaxIgnoredErrorLen];
static char NumIgnoredErrors = 0;

// Asserts.

static const char AssertDashLine[] = "-----------------------------------\n";
static const size_t AssertMessageBufferSize = 1024;

static HHOOK AssertHook = nullptr;

void Debug::startTiming()
{
	TimerStartTime = System::getElapsedTime();
}

void Debug::endTiming()
{
	auto now = System::getElapsedTime();
	Log::info(format("Elapsed time: {:.2f} ms", (now - TimerStartTime) * 1000));
}

string Debug::getLocationString(const std::source_location& location)
{
	const char* path = location.file_name();
	auto subPath = strstr(path, "src");
	if (subPath && (subPath[3] == '\\' || subPath[3] == '/'))
		path = subPath + 4;

	return format("{}:{} ({})", path, location.line(), location.function_name());
}

// =====================================================================================================================
// Private functions.

#ifndef VORTEX_DISABLE_ASSERTS

static bool ShouldIgnore(const char* id)
{
	for (int i = 0; i < NumIgnoredErrors; ++i)
	{
		if (!strcmp(IgnoredErrorList[i], id))
			return true;
	}
	return false;
}

static bool AddIgnore(const char* id)
{
	if (NumIgnoredErrors < MaxIgnoredErrors)
	{
		strcpy(IgnoredErrorList[NumIgnoredErrors], id);
		++NumIgnoredErrors;
		return true;
	}
	return false;
}

static LRESULT CALLBACK CBTProc(INT nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HCBT_ACTIVATE)
	{
		HWND hWndChild = (HWND)wParam;
		UINT result;

		if (GetDlgItem(hWndChild, IDYES))
			result = SetDlgItemTextA(hWndChild, IDYES, "Debug");

		if (GetDlgItem(hWndChild, IDYES))
			result = SetDlgItemTextA(hWndChild, IDNO, "Ignore Once");

		if (GetDlgItem(hWndChild, IDYES))
			result = SetDlgItemTextA(hWndChild, IDCANCEL, "Ignore All");

		UnhookWindowsHookEx(AssertHook);
	}
	else
	{
		CallNextHookEx(AssertHook, nCode, wParam, lParam);
	}
	return 0;
}

static int ShowMessageBox(HWND hWnd, LPSTR lpText, LPSTR lpCaption, UINT uType)
{
	AssertHook = SetWindowsHookEx(WH_CBT, &CBTProc, 0, GetCurrentThreadId());
	return MessageBoxA(hWnd, lpText, lpCaption, uType);
}

bool DebugImpl::assrt(const char* exp, const std::source_location& loc, const char* fmt, ...)
{
	// Check if the assert is flagged as ignore.
	char id[MaxIgnoredErrorLen];
	sprintf(id, "%s%i", loc.file_name(), loc.line());
	if (ShouldIgnore(id))
		return false;

	auto locStr = Debug::getLocationString(loc);
	char buffer[AssertMessageBufferSize];
	if (fmt)
	{
		va_list args;
		va_start(args, fmt);
		char message[AssertMessageBufferSize + MaxIgnoredErrorLen];
		vsnprintf(message, AssertMessageBufferSize + MaxIgnoredErrorLen, fmt, args);
		va_end(args);
		sprintf(buffer, "Check: %s\nFile: %s\n%s\n", exp, locStr.c_str(), message);
	}
	else
	{
		sprintf(buffer, "Check: %s\nFile: %s\n", exp, locStr.c_str());
	}

	Log::blockBegin("Assertion failed");
	Log::error(buffer);
	Log::blockEnd();

	int answer = ShowMessageBox(0, buffer, (LPSTR)"Assertion failed", MB_ICONERROR | MB_YESNOCANCEL);
	if (answer == IDYES)
	{
		return true;
	}
	else if (answer == IDCANCEL)
	{
		if (!AddIgnore(id))
			MessageBoxA(0, "Maximum number of ignorable asserts reached.", "ERROR", MB_ICONERROR | MB_OK);
	}
	return false;
}

#endif

} // namespace AV