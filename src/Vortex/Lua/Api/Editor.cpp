#include <Precomp.h>

#include <Core/System/File.h>
#include <Core/System/Log.h>
#include <Core/Utils/Zip.h>
#include <Core/System/Debug.h>

#include <Simfile/Simfile.h>
#include <Simfile/GameMode.h>

#include <Vortex/Editor.h>
#include <Vortex/Edit/Selection.h>

#include <Vortex/View/View.h>
#include <Vortex/View/Hud.h>

#include <Vortex/Lua/Api.h>
#include <Vortex/Lua/Utils.h>
#include <Vortex/Managers/MusicMan.h>
#include <Vortex/Managers/DialogMan.h>

#include <Vortex/Application.h>

using namespace std;

namespace AV {
namespace Lua {

// =====================================================================================================================
// Settings category

static string GetSettingId(lua_State* L)
{
	// The first part of the id, the category name, is stored in the user value.
	lua_getiuservalue(L, 1, 1);
	auto result = string(lua_tostring(L, -1));
	lua_pop(L, 1);

	// The second part of the id, the setting name, is the index key.
	result.push_back('.');
	result.append(Lua::get<string>(L, 2));
	return result;
}

static const Lua::Metatable SettingsCategoryMetatable {
	.name = "SettingsCategory",
	.index = [](lua_State* L) { return AV::Settings::luaGet(L, GetSettingId(L)); },
	.newIndex = [](lua_State* L) { return AV::Settings::luaSet(L, GetSettingId(L)); },
};

static void PushSettingsCategory(lua_State* L, stringref category)
{
	lua_newuserdatauv(L, 0, 1);
	Lua::push(L, category);
	lua_setiuservalue(L, -2, 1);
	Lua::setMetatable(L, SettingsCategoryMetatable);
}

// =====================================================================================================================
// Settings

static int IndexSettings(lua_State* L)
{
	auto id = Lua::get<string>(L, 2);
	if (!AV::Settings::hasCategory(id))
		luaL_error(L, "settings category '%s' does not exist", id.data());
	PushSettingsCategory(L, id);
	return 1;
}

static void PushSettings(lua_State* L)
{
	static const char* cacheKey = "settings";
	if (lua_getfield(L, LUA_REGISTRYINDEX, cacheKey) != LUA_TUSERDATA)
	{
		lua_newuserdatauv(L, 0, 0);
		Lua::setMetatable(L, IndexSettings);
		Lua::addToRegistry(L, cacheKey);
	}
}

// =====================================================================================================================
// API

#define FUN(name) index.addFunc(#name) = [](lua_State* L)
#define GET(name) index.addProp(#name) = [](lua_State* L)

void Lua::initializeEditorApi(TypeApi& index)
{
	// Cursor

	FUN(getCursorRow) {
		lua_pushnumber(L, View::getCursorRow());
		return 1;
	};

	FUN(setCursorRow) {
		View::setCursorRow(Lua::getArgs<double>(L));
		return 0;
	};

	FUN(getCursorTime) {
		lua_pushnumber(L, View::getCursorTime());
		return 1;
	};

	FUN(setCursorTime) {
		View::setCursorTime(Lua::getArgs<double>(L));
		return 0;
	};

	// Selection

	FUN(getSelectedRows) {
		auto region = Selection::getRegion();
		lua_pushinteger(L, region.beginRow);
		lua_pushinteger(L, region.endRow);
		return 2;
	};

	FUN(setSelectedRows) {
		auto [beginRow, endRow] = Lua::getArgs<int, int>(L);
		Selection::selectRegion(beginRow, endRow);
		return 0;
	};

	// Simfile

	FUN(openSimfile) {
		switch (lua_gettop(L))
		{
		case 0:
			Editor::openSimfile();
			break;
		case 1:
			Editor::openSimfile(Lua::getArgs<string>(L));
			break;
		default:
			luaL_error(L, "Expected 1-2 arguments.");
		}
		return 0;
	};

	FUN(openNextSimfile) {
		Editor::openNextSimfile(true);
		return 0;
	};

	FUN(openPreviousSimfile) {
		Editor::openNextSimfile(false);
		return 0;
	};

	FUN(saveSimfile) {
		Editor::saveSimfile(false);
		return 0;
	};

	FUN(saveSimfileAs) {
		Editor::saveSimfile(true);
		return 0;
	};

	FUN(closeSimfile) {
		Editor::closeSimfile();
		return 0;
	};

	GET(activeSimfile) {
		// TODO: implement.
		return 0;
	};

	// Chart

	FUN(openNextChart) {
		Editor::nextChart();
		return 0;
	};

	FUN(openPreviousChart) {
		Editor::previousChart();
		return 0;
	};

	FUN(closeChart) {
		Editor::openChart(nullptr);
		return 0;
	};

	GET(activeChart) {
		// TODO: implement.
		return 0;
	};

	// Misc

	FUN(convertMusicToOgg) {
		MusicMan::startOggConversion();
		return 1;
	};

	FUN(openDialog) {
		// TODO: resolve id.
		auto id = Lua::getArgs<string>(L);
		DialogMan::requestOpen(DialogId::AdjustSync /* <- needs to be replaced */);
		return 0;
	};

	FUN(exitApplication) {
		Application::exit();
		return 0;
	};

	GET(settings) {
		Lua::PushSettings(L);
		return 1;
	};
}

} // namespace Lua
} // namespace AV