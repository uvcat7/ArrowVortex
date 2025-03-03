#include <Precomp.h>

#include <Core/Utils/Xmr.h>
#include <Core/Utils/Unicode.h>
#include <Core/Common/Event.h>
#include <Vortex/Settings.h>

#include <Core/Utils/Util.h>
#include <Core/Utils/String.h>
#include <Core/Utils/vector.h>
#include <Core/Utils/Flag.h>

#include <Core/Graphics/Text.h>
#include <Core/Graphics/Draw.h>

#include <Core/Interface/UiStyle.h>

#include <Core/System/File.h>
#include <Core/System/Debug.h>

#include <Simfile/Simfile.h>

#include <Vortex/Common.h>

#include <Vortex/Lua/Utils.h>

#include <Vortex/Application.h>
#include <Vortex/Managers/MusicMan.h>
#include <Vortex/Editor.h>
#include <Vortex/View/StatusBar.h>
#include <Vortex/View/Minimap.h>
#include <Vortex/View/View.h>
#include <Vortex/Notefield/Waveform.h>

namespace AV {

using namespace std;
using namespace Util;

// =====================================================================================================================
// Settings initializers

Settings::General::General(Setting::Registry& r)
	: multithread  (r, "general.multithread", true)
	, verticalSync (r, "general.verticalSync", true)
	, fontSize     (r, "general.fontSize", 13, 1, 100)
	, fontPath     (r, "general.fontPath", "assets/bokutachi no gothic 2.otf")
{
}

Settings::Audio::Audio(Setting::Registry& r)
	: musicVolume  (r, "audio.musicVolume",  100, 0, 100)
	, musicSpeed   (r, "audio.musicSpeed", 100, 10, 400, Setting::Flags::None)
	, tickOffsetMs (r, "audio.tickOffsetMs", 0, INT_MIN, INT_MAX)
	, muteMusic    (r, "audio.mute", false)
	, beatTicks    (r, "audio.beatTick", false)
	, noteTicks    (r, "audio.noteTick", false)
{
}

Settings::Editor::Editor(Setting::Registry& r)
	: undoRedoJump      (r, "editor.undoRedoJump", true)
	, timeBasedCopy     (r, "editor.timeBasedCopy", false)
	, defaultSaveFormat (r, "editor.defaultSaveFormat", "sm")
{
}

Settings::View::View(Setting::Registry& r)
	: isTimeBased     (r, "view.isTimeBased", false)
	, useColumnColors (r, "view.columnColors", false)
	, showColumnBg    (r, "view.showColumnBg", true)
	, reverseScroll   (r, "view.reverseScroll", false)
	, showWaveform    (r, "view.showWaveform", true)
	, showBeatLines   (r, "view.showBeatLines", true)
	, showNotes       (r, "view.showNotes", true)
	, showTempoBoxes  (r, "view.showTempoBoxes", true)
	, showTempoHelp   (r, "view.showTempoHelp", true)
	, showPassedNotes (r, "view.showPassedNotes", false)
	, bgMode          (r, "view.backgroundMode", Sprite::Fill::Stretch)
	, bgBrightness    (r, "view.backgroundBrightness", 50, 0, 100)
	, minimapMode     (r, "view.minimapMode", Minimap::Mode::Notes)
{
}

Settings::Waveform::Waveform(Setting::Registry& r)
	: backgroundColor (r, "waveform.backgroundColor", { 0.0f, 0.0f, 0.0f, 1 })
	, waveColor       (r, "waveform.waveColor", { 0.4f, 0.6f, 0.4f, 1 })
	, filterColor     (r, "waveform.filterColor", { 0.8f, 0.8f, 0.5f, 1 })
	, luminance       (r, "waveform.luminance", AV::Waveform::Luminance::Uniform)
	, waveShape       (r, "waveform.waveShape", AV::Waveform::WaveShape::Rectified)
	, antiAliasing    (r, "waveform.antiAliasing", 1, 0, 3)
{
}

Settings::Status::Status(Setting::Registry& r)
	: showChart      (r, "status.showChart", true)
	, showSnap       (r, "status.showSnap", true)
	, showBpm        (r, "status.showBpm", true)
	, showRow        (r, "status.showRow", false)
	, showBeat       (r, "status.showBeat", false)
	, showMeasure    (r, "status.showMeasure", true)
	, showTime       (r, "status.showTime", true)
	, showTimingMode (r, "status.showTimingMode", true)
{
}

// ====================================================================================================================
// Static data

namespace Settings
{
	struct State
	{
		State(Setting::Registry&);

		map<string, Setting*> settings;

		General general;
		Audio audio;
		Editor editor;
		Status status;
		Waveform waveform;
		View view;
	};
	static State* state = nullptr;
}
using Settings::state;

Settings::State::State(Setting::Registry& r)
	: general(r)
	, audio(r)
	, editor(r)
	, status(r)
	, waveform(r)
	, view(r)
{
}

// =====================================================================================================================
// Load/save settings.

struct SettingsRegistry : public Setting::Registry
{
	map<string, Setting*> settings;

	void add(Setting* setting, const char* id) override
	{
		settings.emplace(id, setting);
	}
};

void Settings::initialize(const XmrDoc& doc)
{
	SettingsRegistry registry;
	state = new State(registry);
	state->settings.swap(registry.settings);
	for (auto& kv : state->settings)
	{
		if (!(kv.second->flags() & Setting::Flags::Save))
			continue;

		XmrNode* attrib;
		string groupName, attribName;
		if (String::splitOnce(kv.first, groupName, attribName, "."))
		{
			auto group = doc.child(groupName.data());
			if (!group) return;
			attrib = group->child(attribName.data());
		}
		else
		{
			attrib = doc.child(kv.first.data());
		}
		if (!attrib) return;
		kv.second->read(attrib->value);
	}
}

void Settings::deinitialize()
{
	Util::reset(state);
}

void Settings::save(XmrDoc& doc)
{
	for (auto& kv : state->settings)
	{
		if (!(kv.second->flags() & Setting::Flags::Save))
			continue;

		XmrNode* attrib;
		string groupName, attribName;
		if (String::splitOnce(kv.first, groupName, attribName, "."))
		{
			auto group = doc.getOrAddChild(groupName.data());
			attrib = group->addChild(attribName.data());
		}
		else
		{
			attrib = doc.getOrAddChild(kv.first.data());
		}
		kv.second->write(attrib->value);
	}
}

bool Settings::hasCategory(stringref name)
{
	auto prefix = name + ".";
	for (auto& setting : state->settings)
	{
		if (setting.first.starts_with(prefix))
			return true;
	}
	return false;
}

int Settings::luaSet(lua_State* L, stringref id)
{
	auto it = state->settings.find(id);
	if (it == state->settings.end())
	{
		luaL_error(L, "setting '%s' not found", id);
		return 0;
	}
	it->second->set(L);
	return 0;
}

int Settings::luaGet(lua_State* L, stringref id)
{
	auto it = state->settings.find(id);
	if (it == state->settings.end())
	{
		luaL_error(L, "setting '%s' not found", id);
		return 0;
	}
	it->second->get(L);
	return 1;
}

Settings::General& Settings::general()
{
	return state->general;
}

Settings::Audio& Settings::audio()
{
	return state->audio;
}

Settings::Editor& Settings::editor()
{
	return state->editor;
}

Settings::Status& Settings::status()
{
	return state->status;
}

Settings::Waveform& Settings::waveform()
{
	return state->waveform;
}

Settings::View& Settings::view()
{
	return state->view;
}

} // namespace AV
