#include <Precomp.h>

#include <lua/src/lua.hpp>

#include <Core/Common/Setting.h>
#include <Core/Common/Event.h>

#include <Core/Utils/Xmr.h>
#include <Core/Utils/String.h>

#include <Core/System/Log.h>

namespace AV {

using namespace std;

// =====================================================================================================================
// Setting.

Setting::Setting(Registry& r, const char* id, Flags flags)
	: myFlags(flags)
{
	r.add(this, id);
}

Setting::~Setting()
{
}

void Setting::publishChanged() const
{
	Setting::Changed evt{ .setting = this };
	EventSystem::publish(evt);
}

Setting::Flags Setting::flags() const
{
	return myFlags;
}

// =====================================================================================================================
// Bool setting.

BoolSetting::BoolSetting(Registry& r, const char* id, bool value, Flags flags)
	: Setting(r, id, flags)
	, myValue(value)
{
}

bool BoolSetting::read(const string& s)
{
	return String::readBool(s, myValue);
}

void BoolSetting::write(string& s) const
{
	s = String::fromBool(myValue);
}

void BoolSetting::get(lua_State* L) const
{
	lua_pushboolean(L, myValue);
}

void BoolSetting::set(lua_State* L)
{
	set(lua_toboolean(L, -1));
}

void BoolSetting::set(bool value)
{
	if (myValue != value)
	{
		myValue = value;
		publishChanged();
	}
}

void BoolSetting::toggle()
{
	set(!myValue);
}

// =====================================================================================================================
// Int setting.

IntSetting::IntSetting(Registry& r, const char* id, int value, int min, int max, Flags flags)
	: Setting(r, id, flags)
	, myValue(value)
	, myMin(min)
	, myMax(max)
{
}

bool IntSetting::read(const string& s)
{
	if (String::readInt(s, myValue))
	{
		myValue = clamp(myValue, myMin, myMax);
		return true;
	}
	return false;
}

void IntSetting::write(string& s) const
{
	s = String::fromInt(myValue);
}

void IntSetting::get(lua_State* L) const
{
	lua_pushinteger(L, myValue);
}

void IntSetting::set(lua_State* L)
{
	set((int)lua_tointeger(L, -1));
}

void IntSetting::set(int value)
{
	value = clamp(value, myMin, myMax);
	if (myValue != value)
	{
		myValue = value;
		publishChanged();
	}
}

// =====================================================================================================================
// String setting.

StringSetting::StringSetting(Registry& r, const char* id, string_view value, Flags flags)
	: Setting(r, id, flags)
	, myValue(value)
{
}

bool StringSetting::read(const string& s)
{
	myValue = s;
	return true;
}

void StringSetting::write(string& s) const
{
	s = myValue;
}

void StringSetting::get(lua_State* L) const
{
	lua_pushlstring(L, myValue.data(), myValue.size());
}

void StringSetting::set(lua_State* L)
{
	set(lua_tostring(L, -1));
}

void StringSetting::set(string_view value)
{
	if (myValue != value)
	{
		myValue = value;
		publishChanged();
	}
}

// =====================================================================================================================
// Color setting.

ColorSetting::ColorSetting(Registry& r, const char* id, const Colorf& defaultValue, Flags flags)
	: Setting(r, id, flags)
	, myValue(defaultValue)
{
}

bool ColorSetting::read(const string& s)
{
	float v[4];
	if (String::readFloats(s, v))
	{
		myValue = { v[0], v[1], v[2], v[3] };
		return true;
	}
	return false;
}

void ColorSetting::write(string& s) const
{
	s = format("{:.3f}, {:.3f}, {:.3f}, {:.3f}",
		myValue.red,
		myValue.green,
		myValue.blue,
		myValue.alpha);
}

static void SetChannel(lua_State* L, int index, float channel)
{
	lua_pushnumber(L, channel);
	lua_seti(L, -2, index);
}

void ColorSetting::get(lua_State* L) const
{
	lua_createtable(L, 4, 0);
	SetChannel(L, 1, myValue.red);
	SetChannel(L, 2, myValue.green);
	SetChannel(L, 3, myValue.blue);
	SetChannel(L, 4, myValue.alpha);
}

static float GetF(lua_State* L, int index)
{
	lua_rawgeti(L, -1, index);
	float result = (float)lua_tonumber(L, -1);
	lua_pop(L, 1);
	return result;
}

void ColorSetting::set(lua_State* L)
{
	set({ GetF(L, 1), GetF(L, 2), GetF(L, 3), GetF(L, 4) });
}

void ColorSetting::set(const Colorf& value)
{
	if (myValue != value)
	{
		myValue = value;
		publishChanged();
	}
}

// =====================================================================================================================
// Enum setting.

EnumSettingBase::EnumSettingBase(Registry& r, const char* id, Flags flags)
	: Setting(r, id, flags)
{
}

void EnumSettingBase::get(lua_State* L) const
{
	string str;
	write(str);
	lua_pushlstring(L, str.data(), str.size());
}

void EnumSettingBase::set(lua_State* L)
{
	auto str = lua_tostring(L, -1);
	if (!read(str))
		Log::error(format("invalid setting value '{}'", str));
}

} // namespace AV