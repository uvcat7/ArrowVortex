#pragma once

#include <Core/Common/Setting.h>
#include <Vortex/Notefield/Waveform.h>
#include <Vortex/View/Minimap.h>

namespace AV {
namespace Settings {

struct General
{
	General(Setting::Registry&);

	BoolSetting multithread;
	BoolSetting verticalSync;
	IntSetting fontSize;
	StringSetting fontPath;
};

struct Status
{
	Status(Setting::Registry&);

	BoolSetting showChart;
	BoolSetting showSnap;
	BoolSetting showBpm;
	BoolSetting showRow;
	BoolSetting showBeat;
	BoolSetting showMeasure;
	BoolSetting showTime;
	BoolSetting showTimingMode;
};

struct Editor
{
	Editor(Setting::Registry&);

	BoolSetting undoRedoJump;
	BoolSetting timeBasedCopy;
	StringSetting defaultSaveFormat;
};

struct View
{
	View(Setting::Registry&);

	BoolSetting isTimeBased;
	BoolSetting useColumnColors;
	BoolSetting showColumnBg;
	BoolSetting reverseScroll;

	BoolSetting showWaveform;
	BoolSetting showBeatLines;
	BoolSetting showNotes;
	BoolSetting showTempoBoxes;
	BoolSetting showTempoHelp;
	BoolSetting showPassedNotes;

	EnumSetting<Sprite::Fill> bgMode;
	IntSetting bgBrightness;

	EnumSetting<Minimap::Mode> minimapMode;
};

struct Audio
{
	Audio(Setting::Registry&);

	IntSetting musicVolume;
	IntSetting musicSpeed;
	IntSetting tickOffsetMs;
	BoolSetting muteMusic;
	BoolSetting beatTicks;
	BoolSetting noteTicks;
};

struct Waveform
{
	Waveform(Setting::Registry&);

	ColorSetting backgroundColor;
	ColorSetting waveColor;
	ColorSetting filterColor;
	EnumSetting<AV::Waveform::Luminance> luminance;
	EnumSetting<AV::Waveform::WaveShape> waveShape;
	IntSetting antiAliasing;
};

// Global settings functions.
void initialize(const XmrDoc& settings);
void deinitialize();

// Settings.
General& general();
Audio& audio();
Editor& editor();
Status& status();
Waveform& waveform();
View& view();

// Saves all registered settings.
void save(XmrDoc& settings);

bool hasCategory(stringref name);

int luaSet(lua_State* L, stringref id);
int luaGet(lua_State* L, stringref id);

} // namespace settings
} // namespace AV
