#include <Editor/Statusbar.h>

#include <Core/StringUtils.h>
#include <Core/Utils.h>
#include <Core/Xmr.h>
#include <Core/Gui.h>
#include <Core/Draw.h>
#include <Core/Text.h>

#include <System/System.h>

#include <Editor/Menubar.h>
#include <Editor/Action.h>
#include <Editor/Editor.h>
#include <Editor/Common.h>
#include <Editor/View.h>

#include <Managers/MetadataMan.h>
#include <Managers/SimfileMan.h>
#include <Managers/ChartMan.h>
#include <Managers/TempoMan.h>

namespace Vortex {

// ================================================================================================
// StatusbarImpl :: member data.

struct StatusbarImpl : public Statusbar {

bool myShowChart;
bool myShowSnap;
bool myShowBpm;
bool myShowRow;
bool myShowBeat;
bool myShowMeasure;
bool myShowTime;
bool myShowTimingMode;

// ================================================================================================
// StatusbarImpl :: constructor / destructor.

~StatusbarImpl()
{
}

StatusbarImpl()
{
	myShowChart = true;
	myShowSnap = true;
	myShowBpm = true;
	myShowRow = false;
	myShowBeat = false;
	myShowMeasure = true;
	myShowTime = true;
	myShowTimingMode = true;
}

// ================================================================================================
// StatusbarImpl :: load / save settings.

void loadSettings(XmrNode& settings)
{
	XmrNode* statusbar = settings.child("statusbar");
	if(statusbar)
	{
		statusbar->get("showChart", &myShowChart);
		statusbar->get("showSnap", &myShowSnap);
		statusbar->get("showBpm", &myShowBpm);
		statusbar->get("showRow", &myShowRow);
		statusbar->get("showBeat", &myShowBeat);
		statusbar->get("showMeasure", &myShowMeasure);
		statusbar->get("showTime", &myShowTime);
		statusbar->get("showTimingMode", &myShowTimingMode);
	}
}

void saveSettings(XmrNode& settings)
{
	XmrNode* statusbar = settings.addChild("statusbar");

	statusbar->addAttrib("showChart", myShowChart);
	statusbar->addAttrib("showSnap", myShowSnap);
	statusbar->addAttrib("showBpm", myShowBpm);
	statusbar->addAttrib("showRow", myShowRow);
	statusbar->addAttrib("showBeat", myShowBeat);
	statusbar->addAttrib("showMeasure", myShowMeasure);
	statusbar->addAttrib("showTime", myShowTime);
	statusbar->addAttrib("showTimingMode", myShowTimingMode);
}

// ================================================================================================
// StatusbarImpl :: member functions.

void draw()
{
	Vector<String> info;

	TextStyle textStyle;
	textStyle.textFlags = Text::MARKUP;

	if(myShowChart)
	{
		info.push_back(gChart->getDescription());
	}

	if(myShowSnap)
	{
		const char* snap = ToString(gView->getSnapType());
		info.push_back(Str::fmt("{tc:888}Snap:{tc} %1").arg(snap));
	}

	if(myShowBpm && gSimfile->isOpen())
	{
		double bpm = gTempo->getBpm(gView->getCursorRow());
		info.push_back(Str::fmt("{tc:888}BPM:{tc} %1").arg(bpm, 3, 3));
	}

	if(myShowRow)
	{
		int row = gView->getCursorRow();
		info.push_back(Str::fmt("{tc:888}Row:{tc} %1").arg(row));
	}

	if(myShowBeat)
	{
		double beat = gView->getCursorBeat();
		info.push_back(Str::fmt("{tc:888}Beat:{tc} %1").arg(beat, 3, 3));
	}

	if(myShowMeasure)
	{
		double measure = gTempo->beatToMeasure(gView->getCursorBeat());
		info.push_back(Str::fmt("{tc:888}Measure:{tc} %1").arg(measure, 2, 2));
	}

	if(myShowTime)
	{
		String time = Str::formatTime(gView->getCursorTime());
		info.push_back(Str::fmt("{tc:888}Time:{tc} %1").arg(time));
	}
	if(myShowTimingMode)
	{
		switch(gTempo->getTimingMode())
		{
		case TempoMan::TIMING_UNIFIED:
			break;
		case TempoMan::TIMING_SONG:
			info.push_back("{tc:888}Timing:{tc} song");
			break;
		case TempoMan::TIMING_STEPS:
			info.push_back("{tc:888}Timing:{tc} steps");
			break;
		}
	}

	if(info.size())
	{
		String str = Str::join(info, " ");
		Text::arrange(Text::MC, textStyle, str.str());

		recti view = gView->getRect();
		view = {view.x + 128, view.y + view.h - 32, view.w - 256 - 32, 24};

		int w = Text::getSize().x + 12;
		int x = view.x + view.w / 2 - w / 2;
		Draw::fill({x, view.y, w, view.h}, Color32(0, 128));

		Text::draw(view);
	}	
}

// ================================================================================================
// StatusbarImpl :: toggle/check functions.

void toggleChart()
{
	myShowChart = !myShowChart;
	gMenubar->update(Menubar::STATUSBAR_CHART);
}

void toggleSnap()
{
	myShowSnap = !myShowSnap;
	gMenubar->update(Menubar::STATUSBAR_SNAP);
}

void toggleBpm()
{
	myShowBpm = !myShowBpm;
	gMenubar->update(Menubar::STATUSBAR_BPM);
}

void toggleRow()
{
	myShowRow = !myShowRow;
	gMenubar->update(Menubar::STATUSBAR_ROW);
}

void toggleBeat()
{
	myShowBeat = !myShowBeat;
	gMenubar->update(Menubar::STATUSBAR_BEAT);
}

void toggleMeasure()
{
	myShowMeasure = !myShowMeasure;
	gMenubar->update(Menubar::STATUSBAR_MEASURE);
}

void toggleTime()
{
	myShowTime = !myShowTime;
	gMenubar->update(Menubar::STATUSBAR_TIME);
}

void toggleTimingMode()
{
	myShowTimingMode = !myShowTimingMode;
	gMenubar->update(Menubar::STATUSBAR_TIMING_MODE);
}


bool hasChart()
{
	return myShowChart;
}

bool hasSnap()
{
	return myShowSnap;
}

bool hasBpm()
{
	return myShowBpm;
}

bool hasRow()
{
	return myShowRow;
}

bool hasBeat()
{
	return myShowBeat;
}

bool hasMeasure()
{
	return myShowMeasure;
}

bool hasTime()
{
	return myShowTime;
}

bool hasTimingMode()
{
	return myShowTimingMode;
}

}; // StatusbarImpl

// ================================================================================================
// Statusbar :: create / destroy.

Statusbar* gStatusbar = nullptr;

void Statusbar::create(XmrNode& settings)
{
	gStatusbar = new StatusbarImpl();
	((StatusbarImpl*)gStatusbar)->loadSettings(settings);
}

void Statusbar::destroy()
{
	delete (StatusbarImpl*)gStatusbar;
	gStatusbar = nullptr;
}

}; // namespace Vortex
