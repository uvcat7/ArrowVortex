#pragma once

#include <Simfile/GameMode.h>

namespace AV {

// Manages the mode of the active simfile.
namespace GameMan
{
	void initialize(const XmrDoc& settings);
	void deinitialize();
};

} // namespace AV
