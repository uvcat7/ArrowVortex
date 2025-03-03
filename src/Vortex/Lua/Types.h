#pragma once

#include <Core/Common/NonCopyable.h>
#include <Core/System/File.h>
#include <Core/System/Debug.h>
#include <Core/Utils/Vector.h>

#include <Simfile/Chart.h>
#include <Simfile/Tempo.h>
#include <Simfile/Formats.h>

#include <lua/src/lua.hpp>

namespace AV {
namespace Lua {

// Definition that can be used to construct a metatable.
// The table will be cached in the registry under the given name.
struct Metatable
{
	const char* name;
	const lua_CFunction index = nullptr;
	const lua_CFunction newIndex = nullptr;
	const lua_CFunction gc = nullptr;
	const lua_CFunction close = nullptr;
};

// Stores all operations that can be performed on a type when its __index or __newindex metamethod is invoked.
struct TypeApi
{
	// In order: function, property, integer constant, string constant.
	typedef std::variant<lua_CFunction, lua_CFunction, int, const char*> Field;

	// Helper to make the syntax for adding functions and properties nicer.
	// Allows writing add(name) = []{ ... }; instead of add(name, []{ ... });
	struct LambdaHelper
	{
		TypeApi* api;
		const char* name;
		const bool isFunc;
		void operator=(lua_CFunction);
	};

	// Adds a function. When indexed, the assigned function is pushed onto the stack.
	LambdaHelper addFunc(const char* name);

	// Adds a property. When indexed, the assigned function is invoked to either push the property value onto the stack
	// (if the API implements __index) or set the property to the value on the stack (if the API implements __newindex).
	LambdaHelper addProp(const char* name);

	// Adds an integer constant. When indexed, the integer is pushed onto the stack.
	void addInt(const char* name, int value);

	// Adds a string constant. When indexed, the string is pushed onto the stack.
	void addStr(const char* name, const char* value);

	// Performs the logic associated with indexing the field corresponding to the given key.
	// The key is assumed to be at stack position 2, which works for both __index and __newindex.
	int invoke(lua_State* L);

	// The map of fields.
	std::map<std::string, Field> fields;
};

// Base for garbage collectable objects implemented as userdata.
class GcObject
{
public:
	virtual ~GcObject() {}

	// Allocates a GC object of the given type as userdata and pushes it onto the stack.
	// The __gc metamethod of the object must invoke GcObject::collect to collect the object.
	template <typename T>
	static T& push(lua_State* L)
	{
		auto p = static_cast<T*>(push(L, sizeof(T)));
		new (p) T();
		return *p;
	}

	// Collects the GC object at stack index 1, invoking its destructor.
	static int collect(lua_State* L);

	// Gets the GC object at the given stack index and verifies it has a metatable with the given name.
	static GcObject* get(lua_State* L, int index, const char* name);

private:
	static GcObject* push(lua_State*, size_t);
};

// Base for lua iterators with state.
class Iterator : public GcObject
{
public:
	// Pushes a default constructed iterator of the given type onto the stack.
	template <typename T>
	static T& push(lua_State* L)
	{
		auto& v = GcObject::push<T>(L);
		wrap(L);
		return v;
	}

	// Pushes the next value onto the stack, or no value if the end is reached.
	virtual int next(lua_State* L) = 0;

private:
	static void wrap(lua_State*);
};

// Stores a map of reference counted objects with a specific metatable.
template <typename T>
struct RefStore
{
	// If present, increments the reference count of the given object.
	// If missing, the object is added to the map with a count of one.
	bool add(const shared_ptr<T>& obj);

	// Decrements the reference count of the object on top of the stack.
	// If count reaches zero, the object is removed from the map.
	bool remove(T* ptr);

	std::map<void*, std::pair<shared_ptr<T>, int>> refs;
};

// The extra space that is allocated for each lua state.
struct ExtraSpace
{
	RefStore<Simfile> simfiles;
	RefStore<Chart> charts;
	RefStore<Tempo> tempos;
};

// =====================================================================================================================
// Template implementations

template <typename T>
bool RefStore<T>::add(const shared_ptr<T>& value)
{
	auto ptr = value.get();
	if (!ptr) return false;
	auto it = refs.find(ptr);
	if (it == refs.end())
		refs.emplace((void*)ptr, make_pair(value, 1));
	else
		it->second.second++;
	return true;
}

template <typename T>
bool RefStore<T>::remove(T* ptr)
{
	auto it = refs.find(ptr);
	if (it == refs.end()) return false;
	VortexAssert(it->second.second > 0);
	--it->second.second;
	if (it->second.second == 0) refs.erase(it);
	return true;
}

} // namespace Lua
} // namespace AV