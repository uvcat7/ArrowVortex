#pragma once

#include <Core/Core.h>

namespace Vortex {

struct Statusbar
{
	static void create(XmrNode& settings);
	static void destroy();

	virtual void saveSettings(XmrNode& settings) = 0;

	virtual void toggleChart() = 0;
	virtual void toggleSnap() = 0;
	virtual void toggleBpm() = 0;
	virtual void toggleRow() = 0;
	virtual void toggleBeat() = 0;
	virtual void toggleMeasure() = 0;
	virtual void toggleTime() = 0;
	virtual void toggleTimingMode() = 0;
	virtual void toggleScroll() = 0;
	virtual void toggleSpeed() = 0;

	virtual bool hasChart() = 0;
	virtual bool hasSnap() = 0;
	virtual bool hasBpm() = 0;
	virtual bool hasRow() = 0;
	virtual bool hasBeat() = 0;
	virtual bool hasMeasure() = 0;
	virtual bool hasTime() = 0;
	virtual bool hasTimingMode() = 0;
	virtual bool hasScroll() = 0;
	virtual bool hasSpeed() = 0;

	virtual void draw() = 0;
};

extern Statusbar* gStatusbar;

}; // namespace Vortex
