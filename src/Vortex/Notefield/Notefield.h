#pragma once

#include <Simfile/Common.h>

namespace AV {

namespace Notefield
{
	void initialize(const XmrDoc& settings);
	void deinitialize();

	void drawGhostNote(int col, const Note& n);
};

} // namespace AV
