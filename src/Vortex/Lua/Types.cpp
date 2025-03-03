#include <Precomp.h>

#include <Core/System/Debug.h>

#include <Vortex/Lua/Types.h>
#include <Vortex/Lua/Utils.h>

using namespace std;

namespace AV {
namespace Lua {

// =====================================================================================================================
// GC object

GcObject* GcObject::push(lua_State* L, size_t size)
{
	auto gcd = (bool*)lua_newuserdatauv(L, sizeof(bool) + size, 0);
	*gcd = false;
	return (GcObject*)(gcd + 1);
}

int GcObject::collect(lua_State* L)
{
	auto gcd = (bool*)lua_touserdata(L, 1);
	if (*gcd) return 0; // already collected
	((GcObject*)(gcd + 1))->~GcObject();
	*gcd = true;
	return 0;
}

GcObject* GcObject::get(lua_State* L, int index, const char* name)
{
	auto gcd = (bool*)lua_touserdata(L, index);
	VortexAssert(*gcd == false);
	if (*gcd) return nullptr; // already collected
	return (GcObject*)(gcd + 1);
}

// =====================================================================================================================
// Iterator

static const Metatable IteratorMetatable{
	.name = "Iterator",
	.gc = GcObject::collect,
	.close = GcObject::collect,
};

static int IteratorNext(lua_State* L)
{
	auto iter = (Iterator*)GcObject::get(L, lua_upvalueindex(1), IteratorMetatable.name);
	return iter->next(L);
}

void Iterator::wrap(lua_State* L)
{
	setMetatable(L, IteratorMetatable);
	lua_pushcclosure(L, IteratorNext, 1);
}

// =====================================================================================================================
// Type API

namespace FieldType
{
	constexpr size_t Property = 0;
	constexpr size_t Function = 1;
	constexpr size_t IntConstant = 2;
	constexpr size_t StringConstant = 3;
};

void TypeApi::LambdaHelper::operator=(lua_CFunction func)
{
	api->fields.emplace(name, isFunc
		? Field(in_place_index<FieldType::Function>, func)
		: Field(in_place_index<FieldType::Property>, func));
}

TypeApi::LambdaHelper TypeApi::addFunc(const char* name)
{
	return { this, name, true };
}

TypeApi::LambdaHelper TypeApi::addProp(const char* name)
{
	return { this, name, false };
}

void TypeApi::addInt(const char* name, int value)
{
	fields.emplace(name, Field(in_place_index<FieldType::IntConstant>, value));
}

void TypeApi::addStr(const char* name, const char* value)
{
	fields.emplace(name, Field(in_place_index<FieldType::StringConstant>, value));
}

int TypeApi::invoke(lua_State* L)
{
	auto name = Lua::get<string>(L, 2);
	auto it = fields.find(name);
	if (it == fields.end())
	{
		luaL_error(L, "field '%s' was not found", name.data());
		return 0;
	}
	auto& entry = it->second;
	switch (entry.index())
	{
	case FieldType::Function:
		lua_pushcfunction(L, get<FieldType::Function>(entry));
		return 1;

	case FieldType::Property:
		return get<FieldType::Property>(entry)(L);

	case FieldType::IntConstant:
		lua_pushinteger(L, get<FieldType::IntConstant>(entry));
		return 1;

	case FieldType::StringConstant:
		lua_pushstring(L, get<FieldType::StringConstant>(entry));
		return 1;
	}
	return 0;
}

} // namespace Lua
} // namespace AV