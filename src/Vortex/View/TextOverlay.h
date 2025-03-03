#pragma once

#include <Precomp.h>

namespace AV {

namespace TextOverlay
{
	enum class Mode { None, Shortcuts, MessageLog, DebugLog, About };

	void initialize(const XmrDoc& settings);
	void deinitialize();

	void show(Mode mode);

	bool isOpen();
};

} // namespace AV
