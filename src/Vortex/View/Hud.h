#pragma once

#include <Core/Types/Rect.h>

namespace AV {

struct InfoBox
{
	virtual ~InfoBox();
	virtual void draw(Rect r) = 0;
	virtual int height() = 0;
};

struct InfoBoxWithProgress : public InfoBox
{
	void draw(Rect r);
	int height();
	void setProgress(double rate);
	void setTime(double seconds);
	std::string left, right;
};

namespace Hud
{
	void initialize(const XmrDoc& settings);
	void deinitialize();

	void show(const shared_ptr<InfoBox>& infoBox);

	void note(const char* fmt, ...);
	void info(const char* fmt, ...);
	void warning(const char* fmt, ...);
	void error(const char* fmt, ...);
};

} // namespace AV
