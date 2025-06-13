#pragma once

#include <Editor/Common.h>

namespace Vortex {

struct NotefieldPreview
{
	enum DrawMode { CMOD, XMOD, XMOD_ALL };

	static void create(XmrNode& settings);
	static void destroy();

	virtual void saveSettings(XmrNode& settings) = 0;

	virtual void draw() = 0;

	virtual int getY() = 0;

	virtual void setMode(DrawMode mode) = 0;
	virtual DrawMode getMode() = 0;

	virtual void toggleEnabled() = 0;
	virtual void toggleShowBeatLines() = 0;
	virtual void toggleReverseScroll() = 0;

	virtual bool hasEnabled() = 0;
	virtual bool hasShowBeatLines() = 0;
	virtual bool hasReverseScroll() = 0;
};

extern NotefieldPreview* gNotefieldPreview;

}; // namespace Vortex