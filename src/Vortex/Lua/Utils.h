#pragma once

#include <Precomp.h>

#include <Core/Utils/Enum.h>

#include <Vortex/Lua/Types.h>

namespace AV {
namespace Lua {

// Frees resources associated with the lua state.
struct DestroyState { void operator()(lua_State* L); };

// Creates a lua state and initializes it.
unique_ptr<lua_State, DestroyState> createState();

// Returns the associated extra space of the lua state.
ExtraSpace* getExtraSpace(lua_State* L);

// Opens the library with the given name.
void loadLibrary(lua_State* L, const char* name);

// Loads and runs a lua file.
bool runFile(lua_State* L, stringref path);

// Performs a pcall and handles any errors.
bool safeCall(lua_State* L, int nargs, int nresults);

// Stringifies the value at the top of the stack.
std::string stringify(lua_State* L, int index);

// Prints the entire contents of the stack to the log.
void dumpStack(lua_State* L);

// Adds the top value to the lua registry using the given key. The value remains on the stack.
void addToRegistry(lua_State* L, const char* key);

// Assigns a metatable with the given index function to the top value. The value remains on the stack.
void setMetatable(lua_State* L, lua_CFunction index);

// Assigns the given metatable to the top value. The value remains on the stack.
void setMetatable(lua_State* L, const Metatable& mt);

// Raises a lua error if the stack does not contain the given number of arguments.
void checkArgCount(lua_State* L, int count);

// Creates a pointer userdata with the given metatable.
int pushPtrUserdata(lua_State* L, void* ptr, const Metatable& mt);

// Gets a pointer userdata at the given stack index, checks if it has the given metatable, and returns its value.
void* getPtrUserdata(lua_State* L, int index, const Metatable& mt);

// Gets a value from the stack and returns it.
template <typename T>
T get(lua_State* L, int index);

// Pushes a value onto the stack.
template <typename T>
int push(lua_State* L, const T& value);

// Forward declaration of get<string> so get<enum> can use it.
template <>
std::string get(lua_State* L, int index);

// Forward declaration of push<string> so push<enum> can use it.
template <>
int push(lua_State* L, stringref value);

// Gets an enum value from the stack and returns it.
template <typename T> requires std::is_enum_v<T>
T get(lua_State* L, int index)
{
	auto s = Lua::get<std::string>(L, index);
	auto v = Enum::fromString<T>(s);
	if (v.has_value()) return v.value();
	auto m = std::format("invalid {} '{}'", typeid(T).name(), s);
	luaL_argerror(L, index, m.data());
	return T();
}

// Pushes an enum value onto the stack.
template <typename T> requires std::is_enum_v<T>
int push(lua_State* L, const T& v)
{
	return Lua::push<std::string>(L, Enum::toString(v));
}

// Checks whether the stack contains one argument of the given type, and returns it.
template <typename T>
T getArgs(lua_State* L)
{
	checkArgCount(L, 1);
	return Lua::get<T>(L, 1);
}

// Checks whether the stack contains two arguments of the given types, and returns them.
template <typename T1, typename T2>
std::pair<T1, T2> getArgs(lua_State* L)
{
	checkArgCount(L, 2);
	return { Lua::get<T1>(L, 1), Lua::get<T2>(L, 2) };
}

// Checks whether the stack contains three arguments of the given types, and returns them.
template <typename T1, typename T2, typename T3>
std::tuple<T1, T2, T3> getArgs(lua_State* L)
{
	checkArgCount(L, 3);
	return { Lua::get<T1>(L, 1), Lua::get<T2>(L, 2), Lua::get<T3>(L, 3) };
}

} // namespace Lua
} // namespace AV