#include <Precomp.h>

#include <Core/Utils/Xmr.h>
#include <Core/Common/Event.h>

#include <Core/Utils/String.h>
#include <Core/Utils/Util.h>
#include <Core/Utils/vector.h>

#include <Core/System/File.h>
#include <Core/System/Debug.h>
#include <Core/System/Log.h>

#include <Core/Graphics/Texture.h>

#include <Vortex/Common.h>
#include <Vortex/Editor.h>
#include <Vortex/Settings.h>

#include <Vortex/View/View.h>

#include <Vortex/Managers/GameMan.h>
#include <Vortex/Noteskin/NoteskinMan.h>
#include <Vortex/Noteskin/NoteskinLoader.h>
#include <Vortex/NoteField/Notefield.h>

namespace AV {

using namespace std;

using namespace Util;

static constexpr int NOTESKIN_WITHOUT_TYPE = -1;

// =====================================================================================================================
// Settings.

/*
struct NoteskinPrefs
{
	void readFrom(const string& s) override
	{
		prefs = String::split(s, ",");
		Vector::truncate(prefs, 3);
	}

	void writeTo(string& s) const override
	{
		s.assign(String::join(prefs, ", "));
	}

	vector<string> prefs;
};
*/

// =====================================================================================================================
// Static data.

namespace NoteskinMan
{
struct State
{
	NoteskinStyle missing;
	vector<shared_ptr<NoteskinStyle>> supportedStyles;
	map<const GameMode*, shared_ptr<Noteskin>> loadedSkins;
	EventSubscriptions subscriptions;
};
static State* state = nullptr;
}
using NoteskinMan::state;

// =====================================================================================================================
// Helpers.

static void giveStyleTopPriority(size_t index)
{
	if (index >= state->supportedStyles.size())
		return;

	auto& styles = state->supportedStyles;
	auto elem = styles[index];
	styles.erase(styles.begin() + index);
	styles.insert(styles.begin(), elem);
}

static void giveStyleTopPriority(stringref name)
{
	auto& skins = state->supportedStyles;
	for (size_t i = 0; i < skins.size(); ++i)
	{
		if (skins[i]->name == name)
		{
			giveStyleTopPriority(i);
			return;
		}
	}
}

static const NoteskinStyle* getStyleFor(const GameMode* mode)
{
	for (const auto& it : state->supportedStyles)
	{
		if (it->hasFallback || it->modes.find(mode) != it->modes.end())
			return it.get();
	}
	return &state->missing;
}

// =====================================================================================================================
// Load noteskin styles.

static void loadNoteskinStyles()
{
	auto root = DirectoryPath("noteskins");
	for (auto name : FileSystem::listDirectories(root))
	{
		auto dir = DirectoryPath(root, name);

		auto style = make_shared<NoteskinStyle>();
		style->name = name;

		// Check if the style supports arbitrary game modes through a fallback noteskin.
		style->hasFallback = FileSystem::exists(FilePath(dir, "fallback.txt"));

		// Open the metadata document to see which game modes are supported explicitly.
		XmrDoc doc;
		auto metadataPath = FilePath(dir, "metadata.txt");
		if (FileSystem::exists(metadataPath) && doc.loadFile(metadataPath) == XmrResult::Success)
		{
			// Read the list of supported styles.
			auto gameModes = doc.child("game-modes");
			if (gameModes)
			{
				for (auto it : gameModes->children())
				{
					auto gameMode = GameModeMan::find(it->name);
					if (!gameMode)
					{
						Log::error(format("Noteskin '{}' has unknown game mode '{}'.", name, it->name));
					}
					else
					{
						style->modes.insert(pair(gameMode, it->value));
					}
				}
			}
		}

		// Make sure the noteskin supports at least something.
		if (style->modes.size() == 0 && !style->hasFallback)
		{
			Log::error(format("Noteskin '{}' does not support any game modes.", name));
			continue;
		}

		state->supportedStyles.push_back(style);
	}

	// Default noteskin, will be overwritten by stored settings if available.
	giveStyleTopPriority("Classic");
}

static void HandleSettingChanged(const Setting::Changed& arg)
{
	auto& view = Settings::view();
	if (arg == view.reverseScroll || arg == view.useColumnColors || arg == view.showColumnBg)
		NoteskinMan::reload();
}

// =====================================================================================================================
// Initialization.

void NoteskinMan::initialize(const XmrDoc& settings)
{
	state = new State();

	loadNoteskinStyles();

	// TODO: fix subscriptions.
	state->subscriptions.add<Setting::Changed>(HandleSettingChanged);
}

void NoteskinMan::deinitialize()
{
	Util::reset(state);
}

// =====================================================================================================================
// Member functions.

void NoteskinMan::reload()
{
	for (auto& it : state->loadedSkins)
	{
		auto newSkin = NoteskinLoader::load(it.second->style, it.second->gameMode);
		if (newSkin)
			it.second.swap(newSkin);
	}
}

void NoteskinMan::setPrimaryStyle(int index)
{
	giveStyleTopPriority(index);
	for (auto& it : state->loadedSkins)
	{
		auto mode = it.second->gameMode;
		auto newStyle = getStyleFor(mode);
		if (it.second->style != newStyle)
		{
			auto newSkin = NoteskinLoader::load(newStyle, mode);
			if (newSkin)
				it.second.swap(newSkin);
		}
	}
	EventSystem::publish<StylesChanged>();
}

span<shared_ptr<NoteskinStyle>> NoteskinMan::supportedStyles()
{
	return span(state->supportedStyles.begin(), state->supportedStyles.end());
}

shared_ptr<Noteskin> NoteskinMan::getNoteskin(const GameMode* mode)
{
	// Check if the noteskin is already loaded.
	auto it = state->loadedSkins.find(mode);
	if (it != state->loadedSkins.end())
		return it->second;

	// Not yet loaded, load it.
	auto style = getStyleFor(mode);
	auto skin = NoteskinLoader::load(style, mode);
	state->loadedSkins.insert(pair(mode, skin));
	return skin;
}

} // namespace AV
