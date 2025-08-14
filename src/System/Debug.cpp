#include <System/Debug.h>

#include <Core/WideString.h>

#include <io.h>
#include <fcntl.h>
#include <stdio.h>
#include <chrono>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <gl/gl.h>
#undef ERROR

namespace Vortex {
namespace Debug {

// ================================================================================================
// Debug :: timing functions.
using namespace std::chrono;

steady_clock::time_point getElapsedTime()
{
	return std::chrono::steady_clock::now();
}

double getElapsedTime(steady_clock::time_point startTime)
{
	auto currentTime = steady_clock::now();
	const duration<double> deltaTime = currentTime - startTime;
	return deltaTime.count();
}

// ================================================================================================
// Debug :: log file and console.

static wchar_t sLogPath[] = L"ArrowVortex.log";

static bool sHasConsole = false;
static bool sHasLogFile = false;

void openLogFile()
{
	if(sHasLogFile) return;

	FILE* fp = nullptr;
	if(_wfopen_s(&fp, sLogPath, L"w") == 0)
	{
		fwrite("\xEF\xBB\xBF", 1, 3, fp); // UTF-8 BOM.
		fclose(fp);
	}

	sHasLogFile = true;
}

void openConsole()
{
	if(sHasConsole) return;

	AllocConsole();

	long hOut = _open_osfhandle((intptr_t)GetStdHandle(STD_OUTPUT_HANDLE), _O_WTEXT);
	long hIn = _open_osfhandle((intptr_t)GetStdHandle(STD_INPUT_HANDLE), _O_WTEXT);
	long hErr = _open_osfhandle((intptr_t)GetStdHandle(STD_ERROR_HANDLE), _O_WTEXT);

	*stdout = *_fdopen(hOut, "w");
	*stdin = *_fdopen(hIn, "r");
	*stderr = *_fdopen(hErr, "w");

	setvbuf(stdout, nullptr, _IONBF, 0);
	setvbuf(stdin, nullptr, _IONBF, 0);
	setvbuf(stderr, nullptr, _IONBF, 0);

	sHasConsole = true;
}

// ================================================================================================
// Debug :: logging functions.

static const int sBufsize = 1024;
static bool sLogBlankLine = false;

static void WriteToLogAndConsole(const char* msg)
{
	FILE* fp = nullptr;
	if(sHasLogFile && _wfopen_s(&fp, sLogPath, L"a") == 0)
	{
		fwrite(msg, 1, strlen(msg), fp);
		fclose(fp);
	}
	if(sHasConsole)
	{
		WideString wmsg = Widen(msg);
		WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), wmsg.str(), wmsg.length(), nullptr, 0);
	}
}

void log(const char* fmt, ...)
{
	if(sLogBlankLine)
	{
		WriteToLogAndConsole("\n");
		sLogBlankLine = false;
	}
	va_list args;
	va_start(args, fmt);
	char buffer[sBufsize];
	int n = vsnprintf(buffer, sBufsize - 1, fmt, args);
	if(n < 0 || n > sBufsize - 1) n = sBufsize - 1;
	buffer[n] = 0;
	WriteToLogAndConsole(buffer);
	va_end(args);
}

void logBlankLine()
{
	sLogBlankLine = true;
}

void blockBegin(Type type, const char* title)
{
	sLogBlankLine = true;
	switch(type)
	{
	case ERROR:
		log("[ERROR] %s\n", title);
		break;
	case WARNING:
		log("[WARNING] %s\n", title);
		break;
	default:
		log("[INFO] %s\n", title);
		break;
	};	
}

void blockEnd()
{
	sLogBlankLine = true;
}

}; // namespace Debug.

// ================================================================================================
// Debug :: ignores.

namespace DebugPrivate {

#define MAX_IGNORE_ID_LEN	(MAX_PATH + 16)
#define MAX_DEBUG_MSG_LEN   (1024)
#define MAX_NUM_IGNORES		(32)

static char sIgnoreList[MAX_NUM_IGNORES][MAX_IGNORE_ID_LEN];
static char sNumIgnored = 0;

static bool ShouldIgnore(const char* id)
{
	for(int i = 0; i < sNumIgnored; ++i)
	{
		if(!strcmp(sIgnoreList[i], id))
		{
			return true;
		}
	}
	return false;
}

static bool AddIgnore(const char* id)
{
	if(sNumIgnored < MAX_NUM_IGNORES)
	{
		strcpy(sIgnoreList[sNumIgnored], id);
		++sNumIgnored;
		return true;
	}
	return false;
}

// ================================================================================================
// Debug :: asserts.

#ifndef VORTEX_DISABLE_ASSERTS

static HHOOK sHook;

static const char* sDashLine = "-----------------------------------";

static LRESULT CALLBACK CBTProc(INT nCode, WPARAM wParam, LPARAM lParam)
{
	if(nCode == HCBT_ACTIVATE)
	{
		HWND hWndChild = (HWND)wParam;

		UINT result;
		if(GetDlgItem(hWndChild, IDYES))
			result = SetDlgItemText(hWndChild, IDYES, "Debug");
		if(GetDlgItem(hWndChild, IDYES))
			result = SetDlgItemText(hWndChild, IDNO, "Ignore Once");
		if(GetDlgItem(hWndChild, IDYES))
			result = SetDlgItemText(hWndChild, IDCANCEL, "Ignore All");

		UnhookWindowsHookEx(sHook);
	}
	else
	{
		CallNextHookEx(sHook, nCode, wParam, lParam);
	}
	return 0;
}

static int ShowMessageBox(HWND hWnd, LPCSTR lpcText, LPCSTR lpcCaption, UINT uType)
{
	sHook = SetWindowsHookEx(WH_CBT, &CBTProc, 0, GetCurrentThreadId());
	return MessageBox(hWnd, lpcText, lpcCaption, uType);
}

bool assrt(const char* exp, const char* file, int line, const char* func, const char* fmt, ...)
{
	char id[MAX_IGNORE_ID_LEN];
	sprintf(id, "%s%i", file, line);

	// Skip leading periods.
	while(file[0] == '.' && file[1] == '.' && file[2] == '\\') file += 3;

	// Check if the assert is flagged as ignore.
	if(!ShouldIgnore(id))
	{
		char buffer[MAX_DEBUG_MSG_LEN * 2];

		if(fmt)
		{
			va_list args;
			va_start(args, fmt);
			char message[MAX_DEBUG_MSG_LEN];
			vsnprintf(message, MAX_DEBUG_MSG_LEN - 1, fmt, args);
			va_end(args);
			sprintf(buffer, "Assert failed: %s\nFile: %s(%i)\nIn: %s\n%s\n", exp, file, line, func, message);
		}
		else
		{
			sprintf(buffer, "Assert failed: %s\nFile: %s(%i)\nIn: %s\n", exp, file, line, func);
		}

		Debug::WriteToLogAndConsole("ASSERT\n");
		Debug::WriteToLogAndConsole(sDashLine);
		Debug::WriteToLogAndConsole(buffer);
		Debug::WriteToLogAndConsole(sDashLine);

		int answer = ShowMessageBox(0, buffer, "ASSERT", MB_ICONERROR | MB_YESNOCANCEL);
		if(answer == IDYES)
		{
			return true;
		}
		else if(answer == IDCANCEL)
		{
			if(!AddIgnore(id))
			{
				MessageBoxA(0, "Maximum number of ignorable asserts reached.", "ERROR", MB_ICONERROR | MB_OK);
			}
		}
	}

	return false;
}

#endif

// ================================================================================================
// Debug :: checkpoints.

#ifndef VORTEX_DISABLE_CHECKPOINTS

void check(const char* exp)
{
	Debug::log("checkpoint: %s\n", exp);
}

#endif

// ================================================================================================
// Debug :: openGL error checking.

bool glerr(const char* file, int line, const char* func)
{
	int code = glGetError();
	if(code)
	{
		char id[MAX_IGNORE_ID_LEN];
		sprintf(id, "%s%i", file, line);
		if(!ShouldIgnore(id))
		{
			Debug::blockBegin(Debug::ERROR, "openGL error");
			Debug::log("location: %s(%i)\n", file, line);
			Debug::log("function: %s\n", func);
			Debug::log("error code: %i\n", code);
			Debug::blockEnd();
			AddIgnore(id);
		}
	}
	return (code != 0);
}

}; // namespace AssertPrivate.

}; // namespace Vortex.