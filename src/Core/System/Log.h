#pragma once

#include <Precomp.h>

namespace AV {

// Logging functions.
namespace Log
{
	// Creates a blank log file to which log messages are written.
	void openFile();
	
	// Opens a console window to which log messages are written.
	void openConsole();

	// Writes a message with severity info.
	void info(const char* message);
	void info(stringref message);

	// Writes a message with severity warning.
	void warning(const char* message);
	void warning(stringref message);
	
	// Writes a message with severity error.
	void error(const char* message);
	void error(stringref message);
		
	// Queues a blank line, to be inserted before the next message.
	void lineBreak();
	
	// Begins a block of related messages. The first message logged during the block will determine its severity.
	void blockBegin(const char* title);
	
	// Ends the current message block.
	void blockEnd();
};

} // namespace AV
