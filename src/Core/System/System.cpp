#include <Precomp.h>

#include <time.h>
#include <chrono>

#include <Vortex/Commands.h>
#include <Core/Utils/Unicode.h>
#include <Core/Utils/Enum.h>
#include <Core/System/System.h>
#include <Core/Interface/UiMan.h>

#define UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>

namespace AV {

using namespace std;
using namespace std::chrono;
namespace fs = std::filesystem;

// =====================================================================================================================
// Static data.

static steady_clock::time_point StartTime;

static DirectoryPath RunDir;
static DirectoryPath ExeDir;

// =====================================================================================================================
// Time.

static void InitializeStartTime()
{
	StartTime = steady_clock::now();
}

double System::getElapsedTime()
{
	auto elapsed = duration_cast<duration<double>>(steady_clock::now() - StartTime);
	return elapsed.count();
}

string System::getLocalTime()
{
	time_t t = time(0);
	tm* now = localtime(&t);
	string time = asctime(localtime(&t));
	if (time.length() >= 9 && time[7] == ' ' && time[8] == ' ')
	{
		time.begin()[8] = '0';
	}
	if (time.back() == '\n')
	{
		time.pop_back();
	}
	return time;
}

string System::getBuildDate()
{
	string date(__DATE__);
	if (date.length() >= 5 && date[4] == ' ') date.begin()[4] = '0';
	return date;
}

// =====================================================================================================================
// Working directory.

static void InitializeWorkingDirectory()
{
	wchar_t buffer[MAX_PATH + 1];

	// Store the directory from which the application was ran.
	GetCurrentDirectoryW(MAX_PATH, buffer);
	RunDir = DirectoryPath(Unicode::narrow(buffer));

	// Store the executable directory.
	GetModuleFileNameW(NULL, buffer, MAX_PATH);
	wchar_t* finalSlash = wcsrchr(buffer, L'\\');
	if (finalSlash) finalSlash[1] = 0;
	ExeDir = DirectoryPath(Unicode::narrow(buffer));

	// Set the current directory to the executable directory.
	SetCurrentDirectoryW(buffer);
}

const DirectoryPath& System::getExeDir()
{
	return ExeDir;
}

const DirectoryPath& System::getRunDir()
{
	return RunDir;
}

// =====================================================================================================================
// Clipboard.

bool System::setClipboardText(stringref text)
{
	bool result = false;
	if (OpenClipboard(nullptr))
	{
		EmptyClipboard();
		auto wtext = Unicode::widen(text);
		size_t size = sizeof(wchar_t) * (wtext.length() + 1);
		HGLOBAL bufferHandle = GlobalAlloc(GMEM_DDESHARE, size);
		char* buffer = (char*)GlobalLock(bufferHandle);
		if (buffer)
		{
			memcpy(buffer, wtext.data(), size);
			GlobalUnlock(bufferHandle);
			if (SetClipboardData(CF_UNICODETEXT, bufferHandle))
			{
				result = true;
			}
			else
			{
				GlobalFree(bufferHandle);
			}
		}
		CloseClipboard();
	}
	return result;
}

string System::getClipboardText()
{
	string str;
	if (OpenClipboard(nullptr))
	{
		HANDLE hData = GetClipboardData(CF_UNICODETEXT);
		wchar_t* src = (wchar_t*)GlobalLock(hData);
		if (src)
		{
			str = Unicode::narrow(src, wcslen(src));
			GlobalUnlock(hData);
		}
		CloseClipboard();
	}
	return str;
}

// =====================================================================================================================
// Commands.

bool System::runSystemCommand(stringref cmd)
{
	return runSystemCommand(cmd, nullptr, nullptr);
}

bool System::runSystemCommand(stringref cmd, CommandPipe* pipe, void* buffer)
{
	bool result = false;

	// Copy the command to an array because CreateProcessW requires a modifiable buffer, urgh.
	auto wcommand = Unicode::widen(cmd);
	vector<wchar_t> wbuffer(wcommand.length() + 1, 0);
	memcpy(wbuffer.data(), wcommand.data(), sizeof(wchar_t) * wcommand.length());

	// Create a pipe for the process's stdin.
	STARTUPINFOW startupInfo;
	startupInfo.cb = sizeof(startupInfo);
	ZeroMemory(&startupInfo, sizeof(startupInfo));

	HANDLE readPipe, writePipe;
	if (pipe)
	{
		// Set the bInheritHandle flag so pipe handles are inherited. 
		SECURITY_ATTRIBUTES attr;
		attr.nLength = sizeof(SECURITY_ATTRIBUTES);
		attr.bInheritHandle = TRUE;
		attr.lpSecurityDescriptor = NULL;

		// Create a pipe to send data to stdin of the child process.
		CreatePipe(&readPipe, &writePipe, &attr, 0);
		SetHandleInformation(writePipe, HANDLE_FLAG_INHERIT, 0);

		startupInfo.hStdInput = readPipe;
		startupInfo.dwFlags |= STARTF_USESTDHANDLES;
	}

	// Fire off the process.
	int flags = CREATE_NO_WINDOW;
	PROCESS_INFORMATION processInfo;
	ZeroMemory(&processInfo, sizeof(processInfo));
	if (CreateProcessW(NULL, wbuffer.data(), NULL, NULL,
		TRUE, flags, NULL, NULL, &startupInfo, &processInfo))
	{
		if (pipe)
		{
			DWORD bytesWritten;
			auto bytesToWrite = pipe->write();
			while (bytesToWrite > 0)
			{
				WriteFile(writePipe, buffer, (DWORD)bytesToWrite, &bytesWritten, NULL);
				bytesToWrite = pipe->write();
			}
			CloseHandle(writePipe);
		}
		WaitForSingleObject(processInfo.hProcess, INFINITE);
		CloseHandle(processInfo.hProcess);
		CloseHandle(processInfo.hThread);
		result = true;
	}

	return result;
}

// =====================================================================================================================
// Utilities.

vector<string> System::getCommandLineArgs()
{
	int numArgs = 0;
	LPWSTR* wideArgs = CommandLineToArgvW(GetCommandLineW(), &numArgs);

	vector<string> result;
	for (int i = 0; i < numArgs; ++i)
		result.push_back(Unicode::narrow(wideArgs[i], wcslen(wideArgs[i])));

	LocalFree(wideArgs);
	return result;
}

void System::openWebpage(stringref link)
{
	ShellExecuteW(0, 0, Unicode::widen(link).data(), 0, 0, SW_SHOW);
}

} // namespace AV

// =====================================================================================================================
// Main entry point.

#ifdef UNIT_TEST_BUILD

namespace AV { extern int RunUnitTests(); } // UnitTests.cpp

int main()
{
	AV::InitializeStartTime();
	AV::InitializeWorkingDirectory();
	return AV::RunUnitTests();
}

#else // UNIT_TEST_BUILD

namespace AV { extern int ApplicationMain(); } // Application.cpp

int APIENTRY WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ char* argv, _In_ int argc)
{
	AV::InitializeStartTime();
	AV::InitializeWorkingDirectory();
	return AV::ApplicationMain();
}

#endif // UNIT_TEST_BUILD