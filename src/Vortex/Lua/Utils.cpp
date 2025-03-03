#include <Precomp.h>

#include <Core/System/Log.h>
#include <Core/System/Debug.h>

#include <Vortex/Lua/Utils.h>
#include <Vortex/Lua/Global.h>

using namespace std;

namespace AV {
namespace Lua {

// =====================================================================================================================
// Utility functions

static void* LuaAlloc(void* ud, void* ptr, size_t osize, size_t nsize)
{
	if (nsize == 0)
	{
		free(ptr);
		return nullptr;
	}
	return realloc(ptr, nsize);
}

ExtraSpace* Lua::getExtraSpace(lua_State* L)
{
	return *(Lua::ExtraSpace**)lua_getextraspace(L);
}

void Lua::DestroyState::operator()(lua_State* L)
{
	auto es = getExtraSpace(L);
	lua_close(L);
	delete es;
}

unique_ptr<lua_State, Lua::DestroyState> Lua::createState()
{
	auto L = lua_newstate(LuaAlloc, nullptr);
	*(Lua::ExtraSpace**)lua_getextraspace(L) = new Lua::ExtraSpace();

	// Load default libraries.
	luaL_requiref(L, "_G", luaopen_base, 1);           // [_G]
	luaL_requiref(L, LUA_DBLIBNAME, luaopen_debug, 1); // [_G, dbg]
	lua_pop(L, 2);                                     // []

	// Create AV pseudo-table.
	lua_newuserdata(L, 0);               // [ud]
	Lua::setMetatable(L, invokeAvIndex); // [ud]
	lua_setglobal(L, "AV");              // []

	return unique_ptr<lua_State, Lua::DestroyState>(L);
}

void Lua::loadLibrary(lua_State* L, const char* name)
{
	static const luaL_Reg Libraries[] = {
		{LUA_LOADLIBNAME, luaopen_package},
		{LUA_COLIBNAME, luaopen_coroutine},
		{LUA_TABLIBNAME, luaopen_table},
		{LUA_IOLIBNAME, luaopen_io},
		{LUA_OSLIBNAME, luaopen_os},
		{LUA_STRLIBNAME, luaopen_string},
		{LUA_MATHLIBNAME, luaopen_math},
		{LUA_UTF8LIBNAME, luaopen_utf8},
		{LUA_DBLIBNAME, luaopen_debug},
		{nullptr, nullptr}
	};
	for (auto& lib : Libraries)
	{
		if (strcmp(lib.name, name) == 0)
		{
			luaL_requiref(L, lib.name, lib.func, 1);
			lua_pop(L, 1);
			return;
		}
	}
	luaL_error(L, "Unknown library: %s.", name);
	return;
}

bool Lua::runFile(lua_State* L, stringref path)
{
	if (luaL_loadfile(L, path.data()) != LUA_OK)
	{
		Log::error(lua_tostring(L, -1));
		lua_pop(L, 1);
		return false;
	}
	return Lua::safeCall(L, 0, 0);
}

static int Traceback(lua_State* L)
{
	const char* msg = lua_tostring(L, -1);
	luaL_traceback(L, L, msg, 0);
	return 1;
}

bool Lua::safeCall(lua_State* L, int nargs, int nresults)
{
	auto tracebackIndex = -2 - nargs;
	lua_pushcfunction(L, Traceback);
	lua_insert(L, tracebackIndex);
	if (lua_pcall(L, nargs, nresults, tracebackIndex) == LUA_OK)
	{
		lua_remove(L, tracebackIndex);
		return true;
	}
	else
	{
		Log::error(lua_tostring(L, -1));
		lua_pop(L, 2);
		return false;
	}
}

void Lua::addToRegistry(lua_State* L, const char* key)
{
	lua_pushvalue(L, -1);
	lua_setfield(L, LUA_REGISTRYINDEX, key);
}

void Lua::setMetatable(lua_State* L, lua_CFunction index)
{
	lua_createtable(L, 0, 1);
	lua_pushcfunction(L, index);
	lua_setfield(L, -2, "__index");
	lua_setmetatable(L, -2);
}

void Lua::setMetatable(lua_State* L, const Metatable& mt)
{
	if (luaL_newmetatable(L, mt.name) == 1)
	{
		if (mt.index)
		{
			lua_pushcfunction(L, mt.index);
			lua_setfield(L, -2, "__index");
		}
		if (mt.newIndex)
		{
			lua_pushcfunction(L, mt.newIndex);
			lua_setfield(L, -2, "__newindex");
		}
		if (mt.gc)
		{
			lua_pushcfunction(L, mt.gc);
			lua_setfield(L, -2, "__gc");
		}
		if (mt.close)
		{
			lua_pushcfunction(L, mt.close);
			lua_setfield(L, -2, "__close");
		}
	}
	lua_setmetatable(L, -2);
}

void Lua::checkArgCount(lua_State* L, int count)
{
	int top = lua_gettop(L);
	if (top != count)
	{
		if (count == 1)
			luaL_error(L, "expected 1 argument, got %d", top);
		else
			luaL_error(L, "expected %d arguments, got %d", count, top);
	}
}

int Lua::pushPtrUserdata(lua_State* L, void* ptr, const Metatable& mt)
{
	*(void**)lua_newuserdatauv(L, sizeof(void*), 0) = ptr;
	Lua::setMetatable(L, mt);
	return 1;
}

void* Lua::getPtrUserdata(lua_State* L, int index, const Metatable& mt)
{
	return *(void**)luaL_checkudata(L, index, mt.name);
}

// =====================================================================================================================
// Get implementations

template <>
string Lua::get<string>(lua_State* L, int index)
{
	if (lua_type(L, index) != LUA_TSTRING)
	{
		luaL_argerror(L, index, "expected string");
		return string();
	}
	size_t len;
	auto str = lua_tolstring(L, index, &len);
	return string(str, len);
}

template <>
double Lua::get<double>(lua_State* L, int index)
{
	if (lua_type(L, index) != LUA_TNUMBER)
	{
		luaL_argerror(L, index, "expected number");
		return 0;
	}
	return lua_tonumber(L, index);
}

template <>
int Lua::get<int>(lua_State* L, int index)
{
	return (int)lround(Lua::get<double>(L, index));
}

template <>
bool Lua::get<bool>(lua_State* L, int index)
{
	if (lua_type(L, index) != LUA_TBOOLEAN)
	{
		luaL_argerror(L, index, "expected boolean");
		return false;
	}
	return (bool)lua_toboolean(L, index);
}

template <>
nullptr_t Lua::get<nullptr_t>(lua_State* L, int index)
{
	return nullptr;
}

// =====================================================================================================================
// Push implementations

template <>
int Lua::push<string>(lua_State* L, stringref value)
{
	lua_pushlstring(L, value.data(), value.length());
	return 1;
}

template <>
int Lua::push<double>(lua_State* L, const double& value)
{
	lua_pushnumber(L, value);
	return 1;
}

template <>
int Lua::push<int>(lua_State* L, const int& value)
{
	lua_pushinteger(L, value);
	return 1;
}

template <>
int Lua::push<bool>(lua_State* L, const bool& value)
{
	lua_pushboolean(L, value);
	return 1;
}

// =====================================================================================================================
// Stringify

static void Stringify(string& out, lua_State* L, int trackIndex, int& counter, int idx, stringref indent)
{
	int index = lua_absindex(L, idx);
	switch (lua_type(L, index))
	{
	case LUA_TNIL:
		out += "nil";
		break;
	case LUA_TBOOLEAN:
		out += lua_toboolean(L, index) ? "true" : "false";
		break;
	case LUA_TLIGHTUSERDATA:
		out += "ldata (0x";
		out += format("{:X}", (uint64_t)lua_touserdata(L, index));
		out += ')';
		break;
	case LUA_TNUMBER:
		out += format("{:.3f}", lua_tonumber(L, index));
		break;
	case LUA_TSTRING:
		out += '"';
		out += lua_tostring(L, index);
		out += '"';
		break;
	case LUA_TTABLE:
		lua_pushvalue(L, index);
		lua_gettable(L, trackIndex);
		if (lua_type(L, -1) == LUA_TNUMBER) {
			out += "(table "; // table has been visited before, skip
			out += to_string(lua_tointeger(L, -1));
			out += ')';
			lua_pop(L, 1); // pop count
		}
		else {
			int nr = ++counter;
			lua_pushvalue(L, index);
			lua_pushinteger(L, nr);
			lua_settable(L, trackIndex);
			lua_pop(L, 1); // pop nil
			out += "table ";
			out += to_string(nr);
			out += " {\n";
			string newIndent = indent + "  ";
			lua_pushnil(L); // first key
			while (lua_next(L, index) != 0) {
				// uses key (at index -2) and value (at index -1)
				out += newIndent;
				out += '[';
				Stringify(out, L, trackIndex, counter, -2, newIndent);
				out += "] = ";
				Stringify(out, L, trackIndex, counter, -1, newIndent);
				out += "\n";
				lua_pop(L, 1); // pop value; keep key for next iteration
			}
			out += indent;
			out += "}";
		}
		break;
	case LUA_TFUNCTION:
		out += "func (0x";
		out += format("{:X}", (uint64_t)lua_tocfunction(L, index));
		out += ')';
		break;
	case LUA_TUSERDATA:
		out += "udata (0x";
		out += format("{:X}", (uint64_t)lua_touserdata(L, index));
		out += ')';
		break;
	case LUA_TTHREAD:
		out += "thread (0x";
		out += format("{:X}", (uint64_t)lua_tothread(L, index));
		out += ')';
		break;
	}
}

static string Stringify(lua_State* L, int firstStackIndex, int lastStackIndex)
{
	string out;
	lua_createtable(L, 0, 0);
	int trackIndex = lua_gettop(L), counter = 0;
	for (int i = firstStackIndex; i <= lastStackIndex; ++i)
	{
		if (i > firstStackIndex)
			out += "\n";
		if (firstStackIndex != lastStackIndex)
			out += format("{} : ", i);
		Stringify(out, L, trackIndex, counter, i, string());
	}
	lua_pop(L, 1);
	return out;
}

std::string Lua::stringify(lua_State* L, int index)
{
	return Stringify(L, index, index);
}

void Lua::dumpStack(lua_State* L)
{
	int n = lua_gettop(L);
	if (n == 0)
		Log::info("Lua stack is empty.");
	else
		Log::info(Stringify(L, 1, n));
}

} // namespace Lua
} // namespace AV