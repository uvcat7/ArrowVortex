#include <Precomp.h>

#include <io.h>
#include <fcntl.h>

#include <Core/System/System.h>
#include <Core/System/Log.h>
#include <Core/Utils/Unicode.h>

#define UNICODE
#define WIN32_LEAN_AND_MEAN
#include "windows.h"

namespace AV {

using namespace std;

enum class LogSeverity { Info, Warning, Error };

static const wchar_t LogFilePath[] = L"ArrowVortex.log";

static bool InsertLineBreak = false;
static bool HasLogFile = false;
static bool HasConsole = false;

struct LogBlockData
{
	string title;
	bool headerWritten = false;
};

static unique_ptr<LogBlockData> ActiveBlock;

void Log::openFile()
{
	if (HasLogFile) return;

	FILE* fp = nullptr;
	if (_wfopen_s(&fp, LogFilePath, L"w") == 0 && fp)
	{
		fwrite("\xEF\xBB\xBF", 1, 3, fp); // UTF-8 BOM.
		fclose(fp);
	}

	HasLogFile = true;
}

void Log::openConsole()
{
	if (HasConsole) return;

	AllocConsole();

	int hOut = _open_osfhandle(intptr_t(GetStdHandle(STD_OUTPUT_HANDLE)), _O_WTEXT);
	int hIn = _open_osfhandle(intptr_t(GetStdHandle(STD_INPUT_HANDLE)), _O_WTEXT);
	int hErr = _open_osfhandle(intptr_t(GetStdHandle(STD_ERROR_HANDLE)), _O_WTEXT);

	*stdout = *_fdopen(hOut, "w");
	*stdin = *_fdopen(hIn, "r");
	*stderr = *_fdopen(hErr, "w");

	setvbuf(stdout, nullptr, _IONBF, 0);
	setvbuf(stdin, nullptr, _IONBF, 0);
	setvbuf(stderr, nullptr, _IONBF, 0);

	HasConsole = true;
}

template <size_t N>
void Write(FILE* fp, const char(&message)[N])
{
	fwrite(message, 1, N - 1, fp);
}

template <size_t N>
void Write(HANDLE handle, const wchar_t (&message)[N])
{
	WriteConsoleW(handle, message, N - 1, nullptr, 0);
}

static void SetConsoleTextColor(HANDLE handle, LogSeverity severity)
{
	switch (severity)
	{
	case LogSeverity::Warning:
		SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN);
		break;
	case LogSeverity::Error:
		SetConsoleTextAttribute(handle, FOREGROUND_RED);
		break;
	};
}

static void ResetConsoleTextColor(HANDLE handle)
{
	SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
}

static void WriteSeverity(FILE* fp, LogSeverity severity, bool isHeader)
{
	switch (severity)
	{
	case LogSeverity::Info:
		if (isHeader)
			Write(fp, "Info: ");
		break;
	case LogSeverity::Warning:
		Write(fp, "Warning: ");
		break;
	case LogSeverity::Error:
		Write(fp, "Error: ");
		break;
	};
}

static void WriteSeverity(HANDLE handle, LogSeverity severity, bool isHeader)
{
	switch (severity)
	{
	case LogSeverity::Info:
		if (isHeader)
			Write(handle, L"Info: ");
		break;
	case LogSeverity::Warning:
		Write(handle, L"Warning: ");
		break;
	case LogSeverity::Error:
		Write(handle, L"Error: ");
		break;
	};
}

static void WriteMessage(LogSeverity severity, const char* message, size_t length)
{
	if (HasLogFile)
	{
		FILE* fp = nullptr;
		if (_wfopen_s(&fp, LogFilePath, L"a") == 0 && fp)
		{
			if (InsertLineBreak)
				Write(fp, "\n");

			if (ActiveBlock)
			{
				if (!ActiveBlock->headerWritten)
				{
					WriteSeverity(fp, severity, true);
					auto& title = ActiveBlock->title;
					fwrite(title.data(), 1, title.length(), fp);
					Write(fp, "\n| ");
				}
				else
				{
					Write(fp, "| ");
				}
			}
			else
			{
				WriteSeverity(fp, severity, false);
			}
			fwrite(message, 1, length, fp);
			Write(fp, "\n");
			fclose(fp);
		}
	}

	if (HasConsole)
	{
		auto handle = GetStdHandle(STD_OUTPUT_HANDLE);
		auto wmsg = Unicode::widen(message, length);

		if (InsertLineBreak)
			Write(handle, L"\n");

		SetConsoleTextColor(handle, severity);
		if (ActiveBlock)
		{
			if (!ActiveBlock->headerWritten)
			{
				WriteSeverity(handle, severity, true);
				auto wtitle = Unicode::widen(ActiveBlock->title);
				WriteConsoleW(handle, wtitle.data(), (DWORD)wtitle.length(), nullptr, 0);
				Write(handle, L"\n| ");
			}
			else
			{
				Write(handle, L"| ");
			}
			ResetConsoleTextColor(handle);
			WriteConsoleW(handle, wmsg.data(), (DWORD)wmsg.length(), nullptr, 0);
			Write(handle, L"\n");
		}
		else
		{
			WriteSeverity(handle, severity, false);
			WriteConsoleW(handle, wmsg.data(), (DWORD)wmsg.length(), nullptr, 0);
			ResetConsoleTextColor(handle);
			Write(handle, L"\n");
		}
	}

	InsertLineBreak = false;

	if (ActiveBlock)
		ActiveBlock->headerWritten = true;
}

void Log::info(const char* message)
{
	WriteMessage(LogSeverity::Info, message, strlen(message));
}

void Log::info(stringref message)
{
	WriteMessage(LogSeverity::Info, message.data(), message.length());
}

void Log::warning(const char* message)
{
	WriteMessage(LogSeverity::Warning, message, strlen(message));
}

void Log::warning(stringref message)
{
	WriteMessage(LogSeverity::Warning, message.data(), message.length());
}

void Log::error(const char* message)
{
	WriteMessage(LogSeverity::Error, message, strlen(message));
}

void Log::error(stringref message)
{
	WriteMessage(LogSeverity::Error, message.data(), message.length());
}

void Log::lineBreak()
{
	InsertLineBreak = true;
}

void Log::blockBegin(const char* title)
{
	ActiveBlock = make_unique<LogBlockData>();
	ActiveBlock->title = title;
}

void Log::blockEnd()
{
	if (ActiveBlock)
		ActiveBlock.reset();
}

} // namespace AV