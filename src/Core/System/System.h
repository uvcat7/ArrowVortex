#pragma once

#include <Core/Common/Input.h>
#include <Core/Common/Event.h>
#include <Core/System/File.h>

namespace AV {

// System functions.
namespace System
{
	// Helper struct for running system commands.
	struct CommandPipe { virtual size_t write() = 0; };
	
	// Returns the command line arguments as a list of strings.
	vector<std::string> getCommandLineArgs();
	
	// Sends the given text to the clipboard.
	bool setClipboardText(stringref text);
	
	// Returns the current clipboard text.
	std::string getClipboardText();
	
	// Runs a system command.
	bool runSystemCommand(stringref cmd);
	
	// Runs a system command and pipes data to it.
	bool runSystemCommand(stringref cmd, CommandPipe* pipe, void* buffer);
	
	// Opens a link to a webpage in the default browser.
	void openWebpage(stringref link);
	
	// Returns the directory of the executable.
	const DirectoryPath& getExeDir();
	
	// Returns the directory from which the program was run.
	const DirectoryPath& getRunDir();
	
	// Returns the number of seconds elapsed since the application was started.
	double getElapsedTime();
	
	// Returns the current local startTime as a formatted date/startTime string.
	std::string getLocalTime();
	
	// Returns the build date of the application as a formatted date string.
	std::string getBuildDate();

// Events:
	struct FileDropped : Event { vector<std::string> paths; };
	struct CloseRequested : Event {};

	// extern EventT<FileDrop> fileDropped;
	// extern Event closeRequested;
};

} // namespace AV
