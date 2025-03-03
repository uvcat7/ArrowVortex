#include <Precomp.h>

#include <Core/Utils/Xmr.h>

#include <Core/Utils/String.h>
#include <Core/Utils/Util.h>

#include <Core/Graphics/Draw.h>
#include <Core/Graphics/Text.h>

#include <Core/Interface/UiElement.h>
#include <Core/Interface/UiStyle.h>
#include <Core/Interface/UiMan.h>

#include <Core/System/System.h>

#include <Vortex/Editor.h>
#include <Vortex/Settings.h>
#include <Vortex/Common.h>
#include <Vortex/VortexUtils.h>

#include <Vortex/Edit/TempoTweaker.h>

#include <Vortex/View/StatusBar.h>
#include <Vortex/View/View.h>

namespace AV {

using namespace std;

// ====================================================================================================================
// Static data

struct StatusbarUi : public UiElement
{
	void draw(bool enabled) override;
};

// =====================================================================================================================
// StatusBar :: member functions.

void StatusBar::initialize(const XmrDoc& settings)
{
	UiMan::add(make_unique<StatusbarUi>());
}

void StatusBar::deinitialize()
{
}

void StatusbarUi::draw(bool enabled)
{
	if (!Editor::currentSimfile()) return;

	vector<std::string> info;

	auto chart = Editor::currentChart();
	auto tempo = Editor::currentTempo();

	auto& settings = AV::Settings::status();

	if (settings.showChart && chart)
	{
		info.push_back(chart->getFullDifficulty());
	}

	if (settings.showSnap)
	{
		const char* snap = Enum::toString(View::getSnapType());
		info.push_back(format("{} {}", "{tc:888}Snap:{tc} ", snap));
	}

	if (settings.showBpm && tempo)
	{
		Row row = View::getCursorRowI();
		auto bpm = tempo->bpmChanges.get(row).bpm;
		info.push_back(format("{} {:.3f}", "{tc:888}BPM:{tc} ", bpm));
	}

	if (settings.showRow)
	{
		Row row = View::getCursorRowI();
		info.push_back(format("{} {}", "{tc:888}Row:{tc} ", row));
	}

	if (settings.showBeat)
	{
		double beat = View::getCursorRow() * BeatsPerRow;
		info.push_back(format("{} {:.2f}", "{tc:888}Beat:{tc} ", beat));
	}

	if (settings.showMeasure && tempo)
	{
		double measure = tempo->timing.rowToMeasure(View::getCursorRow());
		info.push_back(format("{} {:.2f}", "{tc:888}Measure:{tc} ", measure));
	}

	if (settings.showTime)
	{
		string time = String::formatTime(View::getCursorTime());
		info.push_back(format("{} {}", "{tc:888}Time:{tc} ", time));
	}

	if (settings.showTimingMode && tempo)
	{
		switch(GetTimingMode(chart))
		{
		case TimingMode::Unified:
			break;
		case TimingMode::Song:
			info.push_back("{tc:888}Timing:{tc} song");
			break;
		case TimingMode::Chart:
			info.push_back("{tc:888}Timing:{tc} chart");
			break;
		}
	}

	if (info.size())
	{
		string str = String::join(info, " ");
		Text::setStyle(UiText::regular, TextOptions::Markup);
		Text::format(TextAlign::MC, str.data());

		Rect bar = View::rect()
			.sliceB(24)
			.offset(-32,-8)
			.sliceV(Text::getWidth() + 12);

		Draw::fill(bar, Color(0, 128));
		Text::draw(bar);
	}	
}

} // namespace AV
