#include <Precomp.h>

#include <Core/System/File.h>
#include <Core/System/Log.h>
#include <Core/System/Debug.h>
#include <Core/System/Window.h>
#include <Core/Utils/Zip.h>

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

static pair<bool, string> ExecuteScript(stringref name, stringref fn)
{
	auto path = format("lua/scripts/{}/main.lua", name);

	// Execute in separate state to keep main state clean.
	auto state = Lua::createState();
	auto L = state.get();

	if (!Lua::runFile(L, path))
		return make_pair(false, format("error occurred in file '{}'", path));

	if (lua_getglobal(L, fn.data()) == LUA_TNIL)
		return make_pair(false, format("function '{}' not found", fn));

	if (!Lua::safeCall(L, 0, 0))
		return make_pair(false, format("error occurred in function '{}'", fn));

	return make_pair(true, string());
}

// =====================================================================================================================
// Iterators

struct StringIter : Iterator
{
	int next(lua_State* L) override
	{
		if (index >= values.size()) return 0;
		Lua::push(L, values[index++]);
		return 1;
	}
	vector<string> values;
	size_t index = 0;
};

struct QuantizationIter : Iterator
{
	int next(lua_State* L) override
	{
		static const int values[] = { 4, 8, 12, 16, 24, 32, 48, 64, 192 };
		if (index >= size(values)) return 0;
		lua_pushinteger(L, values[index++]);
		return 1;
	}
	size_t index = 0;
};

// =====================================================================================================================
// Core API

#define FUN(name) index.addFunc(#name) = [](lua_State* L)
#define GET(name) index.addProp(#name) = [](lua_State* L)

void Lua::initializeCoreApi(TypeApi& index)
{
	FUN(stringify) {
		auto str = Lua::stringify(L, lua_gettop(L));
		Lua::push(L, str);
		return 1;
	};

	FUN(loadLibrary) {
		Lua::loadLibrary(L, Lua::getArgs<string>(L).data());
		return 0;
	};

	FUN(runScript) {
		auto [name, fn] = Lua::getArgs<string, string>(L);
		auto [success, err] = ExecuteScript(name, fn);
		if (!success) luaL_error(L, "%s", err.data());
		return 0;
	};

	FUN(logInfo) {
		Log::info(Lua::getArgs<string>(L));
		return 0;
	};

	FUN(logWarning) {
		Log::warning(Lua::getArgs<string>(L));
		return 0;
	};

	FUN(logError) {
		Log::error(Lua::getArgs<string>(L));
		return 0;
	};

	FUN(isDirectory) {
		auto path = Lua::getArgs<string>(L);
		lua_pushboolean(L, FileSystem::isDirectory(path));
		return 1;
	};

	FUN(getParentDirectory) {
		FilePath path(Lua::getArgs<string>(L));
		Lua::push(L, path.directory().str);
		return 1;
	};

	FUN(getFilenameStem) {
		FilePath path(Lua::getArgs<string>(L));
		Lua::push(L, path.stem());
		return 1;
	};

	FUN(listFiles) {
		string path, ext;
		bool recursive = false;
		switch (lua_gettop(L))
		{
		case 3:
			ext = Lua::get<string>(L, 3);
			[[fallthrough]];
		case 2:
			recursive = Lua::get<bool>(L, 2);
			[[fallthrough]];
		case 1:
			path = Lua::get<string>(L, 1);
			break;
		default:
			luaL_error(L, "Expected 1-3 arguments.");
		}
		auto& iter = Lua::Iterator::push<Lua::StringIter>(L);
		iter.values = FileSystem::listFiles(path, recursive, ext.empty() ? nullptr : ext.data());
		return 1;
	};

	FUN(listDirectories) {
		string path;
		bool recursive = false;
		switch (lua_gettop(L))
		{
		case 2:
			recursive = Lua::get<bool>(L, 2);
			[[fallthrough]];
		case 1:
			path = Lua::get<string>(L, 1);
			break;
		default:
			luaL_error(L, "Expected 1-2 arguments.");
		}
		auto& iter = Lua::Iterator::push<Lua::StringIter>(L);
		iter.values = FileSystem::listDirectories(path, recursive);
		return 1;
	};

	FUN(askOpenFile) {
		auto [title, filters] = Lua::getArgs<string, string>(L);
		int filterIndex = 0;
		auto result = Window::openFileDlg(title, filterIndex, FilePath(), filters);
		if (result.str.empty())
			return 0;
		Lua::push(L, result.str);
		Lua::push(L, filterIndex);
		return 2;
	};

	FUN(askSaveFile) {
		auto [title, filters] = Lua::getArgs<string, string>(L);
		int filterIndex = 0;
		auto result = Window::saveFileDlg(title, filterIndex, FilePath(), filters);
		if (result.str.empty())
			return 0;
		Lua::push(L, result.str);
		Lua::push(L, filterIndex);
		return 2;
	};

	FUN(askOpenDirectory) {
		auto title = Lua::getArgs<string>(L);
		auto result = Window::openDirDlg(title, DirectoryPath());
		if (result.str.empty())
			return 0;
		return Lua::push(L, result.str);
	};

	FUN(unzipFile) {
		FilePath path(Lua::getArgs<string>(L));
		auto result = Zip::unzipFile(path);
		if (result.str.empty())
			luaL_error(L, "Error unzipping '%s'.", path);
		Lua::push(L, result.str);
		return 1;
	};

	FUN(newSimfile) {
		return Lua::push(L, make_shared<Simfile>());
	};

	FUN(loadSimfile) {
		auto path = Lua::getArgs<string>(L);
		auto result = Editor::importSimfile(path);
		if (holds_alternative<string>(result))
		{
			Log::error(format("Invalid simfile: {}", get<string>(result)));
			return 0;
		}
		return Lua::push(L, get<shared_ptr<Simfile>>(result));
	};

	GET(quantizations) {
		Lua::Iterator::push<Lua::QuantizationIter>(L);
		return 1;
	};

	index.addInt("rowsPerBeat", int(RowsPerBeat));
}

} // namespace Lua
} // namespace AV