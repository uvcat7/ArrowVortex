#pragma once

#include <Precomp.h>

namespace AV {

// Line edit that allows the user to execute arbitrary commands.
namespace CommandLine
{
	void initialize(const XmrDoc& settings);
	void deinitialize();

	void toggle();
	bool isOpen();
};

} // namespace AV
