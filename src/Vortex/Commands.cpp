#include <Precomp.h>

#include <Core/Utils/Util.h>
#include <Core/Utils/Xmr.h>
#include <Core/Utils/String.h>
#include <Core/Utils/Vector.h>
#include <Core/Utils/Ascii.h>
#include <Core/Utils/Enum.h>

#include <Vortex/Commands.h>

#include <Core/System/System.h>
#include <Core/System/Log.h>
#include <Core/System/Debug.h>

#include <Core/Interface/UiMan.h>

#include <Vortex/Managers/MusicMan.h>
#include <Vortex/Application.h>
#include <Vortex/View/Hud.h>
#include <Vortex/View/View.h>
#include <Vortex/Lua/Utils.h>

namespace AV {

using namespace std;

// =====================================================================================================================
// State

namespace Commands
{
	struct State
	{
		unique_ptr<lua_State, Lua::DestroyState> lua;
		map<string, unique_ptr<Command>> commands;
	};
	static State* state = nullptr;
}

// =====================================================================================================================
// Command

Command::Command(const char* id)
	: id(id)
{
}

Command::~Command()
{
}

struct LuaCommand : Command
{
	LuaCommand(const char* id, lua_State* L);
	void run() const override;
	State getState() const override;
	bool hasCheckedMethod = false;
	bool hasEnabledMethod = false;
};

struct IntrinsicCommand : Command
{
	typedef void (*Function)();
	IntrinsicCommand(const char* id, Function func)
		: Command(id), function(func) {}
	void run() const override { function(); }
	State getState() const override { return { true, false }; }
	const Function function;
};

LuaCommand::LuaCommand(const char* id, lua_State* L)
	: Command(id)
{
	lua_getfield(L, LUA_REGISTRYINDEX, "commands"); // [cmds]
	lua_getfield(L, -1, id);                        // [cmds, cmd]
	hasCheckedMethod = lua_getfield(L, -1, "checked") == LUA_TFUNCTION;
	hasEnabledMethod = lua_getfield(L, -2, "enabled") == LUA_TFUNCTION;
	lua_pop(L, 4);
}

void LuaCommand::run() const
{
	auto L = Commands::state->lua.get();
	lua_getfield(L, LUA_REGISTRYINDEX, "commands");  // [cmds]
	lua_getfield(L, -1, id);                         // [cmds, cmd]
	if (lua_getfield(L, -1, "run") == LUA_TFUNCTION) // [cmds, cmd, run]
	{
		Lua::safeCall(L, 0, 0);                      // [cmds, cmd]
		lua_pop(L, 2);
	}
	else
	{
		Log::error(format("Command '{}' has no run function", id));
		lua_pop(L, 3);
	}
}

LuaCommand::State LuaCommand::getState() const
{
	State result = { true, false };
	auto L = Commands::state->lua.get();
	lua_getfield(L, LUA_REGISTRYINDEX, "commands"); // [cmds]
	lua_getfield(L, -1, id);                        // [cmds, cmd]
	if (hasCheckedMethod)
	{
		lua_getfield(L, -1, "checked"); // [cmds, cmd, func]
		if (Lua::safeCall(L, 0, 1))     // [cmds, cmd, checked]
			result.checked = lua_toboolean(L, -1);
		lua_pop(L, 1);                  // [cmds, cmd]
	}
	if (hasEnabledMethod)
	{
		lua_getfield(L, -1, "enabled"); // [cmds, cmd, func]
		if (Lua::safeCall(L, 0, 1))     // [cmds, cmd, enabled]
			 result.enabled = lua_toboolean(L, -1);
		lua_pop(L, 1);                  // [cmds, cmd]
	}
	lua_pop(L, 2);
	return result;
}

// =====================================================================================================================
// Intrinsic commands

static map<string, unique_ptr<Command>> CreateIntrinsicCommands()
{
	map<string, unique_ptr<Command>> result;

	auto add = [&](const char* id, IntrinsicCommand::Function func)
	{
		result.emplace(id, make_unique<IntrinsicCommand>(id, func));
	};

	// Main

	add("exit-application", Application::exit);

	// Edit

	add("edit-undo", []
	{
	});
	add("edit-redo", []
	{
	});
	add("edit-cut", []
	{
	});
	add("edit-copy", []
	{
	});
	add("edit-paste", []
	{
	});
	add("edit-delete", []
	{
	});
	add("edit-select-all", []
	{
	});

	// Region

	add("region-select-start-or-end", []
	{
	});
	add("region-select-before-cursor", []
	{
	});
	add("region-select-after-cursor", []
	{
	});

	// Cursor

	add("cursor-play-or-pause", []
	{
		if (MusicMan::isPaused())
			MusicMan::play();
		else
			MusicMan::pause();
	});
	add("cursor-next-snap", []
	{
		View::setSnapType(Enum::nextWrapped(View::getSnapType(), SnapType::Count));
	});
	add("cursor-previous-snap", []
	{
		View::setSnapType(Enum::previousWrapped(View::getSnapType(), SnapType::Count));
	});
	add("cursor-up", []
	{
		View::setCursorRow(View::snapRow(View::getCursorRowI(), SnapDir::Up));
	});
	add("cursor-down", []
	{
		View::setCursorRow(View::snapRow(View::getCursorRowI(), SnapDir::Down));
	});
	add("cursor-to-previous-measure", []
	{
		View::setCursorRow(View::snapToNextMeasure(true));
	});
	add("cursor-to-next-measure", []
	{
		View::setCursorRow(View::snapToNextMeasure(false));
	});
	add("cursor-to-stream-start", []
	{
		View::setCursorRow(View::snapToStream(true));
	});
	add("cursor-to-stream-end", []
	{
		View::setCursorRow(View::snapToStream(false));
	});
	add("cursor-to-selection-start", []
	{
		View::setCursorRow(View::snapToSelection(true));
	});
	add("cursor-to-selection-end", []
	{
		View::setCursorRow(View::snapToSelection(false));
	});
	add("cursor-to-chart-start", []
	{
		View::setCursorRow(0);
	});
	add("cursor-to-chart-end", []
	{
		View::setCursorRow(View::getEndRow());
	});

	// View

	add("zoom-in", []
	{
	});
	add("zoom-out", []
	{
	});
	add("increase-note-spacing", []
	{
	});
	add("decrease-note-spacing", []
	{
	});

	// Misc

	add("reload-noteskins", []
	{
	});

	return result;
}

static int RegisterCommand(lua_State* L)
{
	lua_getfield(L, LUA_REGISTRYINDEX, "commands"); // [cmd, cmds]
	lua_getfield(L, 1, "id");                       // [cmd, cmds, id]
	lua_pushvalue(L, 1);                            // [cmd, cmds, id, cmd]
	lua_rawset(L, 2);                               // [cmd, cmds]
	return 0;
}

void Commands::initialize(const XmrDoc& settings)
{
	state = new State();
	state->lua = Lua::createState();
	auto L = state->lua.get();

	// Create table in registry to store commands.
	lua_createtable(L, 100, 0);
	lua_setfield(L, LUA_REGISTRYINDEX, "commands");

	// Read and store commands.
	XmrDoc doc;
	lua_register(L, "Command", RegisterCommand);
	auto top = lua_gettop(L);
	for (auto& filename : FileSystem::listFiles("lua/commands", false, "lua"))
		Lua::runFile(L, "lua/commands/" + filename);
	auto top2 = lua_gettop(L);

	// Create intrinsic command first so they can be overwritten by lua commands.
	state->commands = CreateIntrinsicCommands();

	// Create lua commands for the command definitions in the registry.
	lua_getfield(L, LUA_REGISTRYINDEX, "commands");
	lua_pushnil(L); // first key
	while (lua_next(L, -2) != 0)
	{
		auto id = lua_tostring(L, -2);
		state->commands.emplace(id, make_unique<LuaCommand>(id, L));
		lua_pop(L, 1); // pop value, keep key
	}
}

void Commands::deinitialize()
{
	Util::reset(state);
}

Command* Commands::find(stringref id, bool intrinsicsOnly)
{
	auto it = state->commands.find(id);
	return it == state->commands.end() ? nullptr : it->second.get();
}

} // namespace AV
