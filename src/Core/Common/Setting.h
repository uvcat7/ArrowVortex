#pragma once

#include <Core/Common/Event.h>
#include <Core/Graphics/Colorf.h>
#include <Core/Utils/Enum.h>

struct lua_State;

namespace AV {

// Base class for application settings.
class Setting
{
public:
	enum class Flags
	{
		None = 0,
		Save = 1 << 0, // Write to settings.txt at application shutdown.
	};

	// Helper for registering settings.
	struct Registry { virtual void add(Setting* setting, const char* id) = 0; };

	// Setting changed event.
	struct Changed : Event { const Setting* setting; };

	Setting(Registry&, const char* id, Flags flags = Flags::Save);
	~Setting();

	virtual bool read(stringref s) = 0;
	virtual void write(std::string& s) const = 0;

	virtual void get(lua_State* L) const = 0;
	virtual void set(lua_State* L) = 0;

	Flags flags() const;

protected:
	void publishChanged() const;
	Flags myFlags;
};

// Enable flag operations.
template <>
constexpr bool IsFlags<Setting::Flags> = true;

// Checks whether the changed event is for the given setting.
template <typename T>
inline bool operator == (const Setting::Changed& l, const T& r)
{
	return l.setting == &r;
}

// Stores a boolean.
class BoolSetting : public Setting
{
public:
	BoolSetting(Registry&, const char* id, bool value, Flags flags = Flags::Save);

	bool read(stringref s) override;
	void write(std::string& s) const override;

	void get(lua_State* L) const override;
	void set(lua_State* L) override;

	void set(bool value);
	void toggle();

	operator bool() const { return myValue; }
	inline bool value() const { return myValue; }

private:
	bool myValue;
};

// Stores an integer.
class IntSetting : public Setting
{
public:
	IntSetting(Registry&, const char* id, int value, int min, int max, Flags flags = Flags::Save);

	bool read(stringref s) override;
	void write(std::string& s) const override;

	void get(lua_State* L) const override;
	void set(lua_State* L) override;

	void set(int value);

	operator int() const { return myValue; }
	inline int value() const { return myValue; }

private:
	int myValue, myMin, myMax;
};

// Stores a string.
class StringSetting : public Setting
{
public:
	StringSetting(Registry&, const char* id, std::string_view value, Flags flags = Flags::Save);

	bool read(stringref s) override;
	void write(std::string& s) const override;

	void get(lua_State* L) const override;
	void set(lua_State* L) override;

	void set(std::string_view value);

	operator stringref() const { return myValue; }
	inline stringref value() const { return myValue; }

private:
	std::string myValue;
};

// Stores an RGBA color.
class ColorSetting : public Setting
{
public:
	ColorSetting(Registry&, const char* id, const Colorf& value, Flags flags = Flags::Save);

	bool read(stringref s) override;
	void write(std::string& s) const override;

	void get(lua_State* L) const override;
	void set(lua_State* L) override;

	void set(const Colorf& value);

	operator const Colorf&() const { return myValue; }

	inline const Colorf& value() const { return myValue; }

private:
	Colorf myValue;
};

// Base class for enum settings.
class EnumSettingBase : public Setting
{
public:
	EnumSettingBase(Registry&, const char* id, Flags flags);

	void get(lua_State* L) const override;
	void set(lua_State* L) override;
};

// Stores an enum value.
template <typename T>
class EnumSetting : public EnumSettingBase
{
public:
	EnumSetting(Registry& r, const char* id, T value, Flags flags = Flags::Save)
		: EnumSettingBase(r, id, flags), myValue(value)
	{
	}

	bool read(stringref s) override
	{
		auto v = Enum::fromString<T>(s);
		if (v.has_value())
		{
			myValue = *v;
			return true;
		}
		return false;
	}

	void write(std::string& s) const override
	{
		s = Enum::toString<T>(myValue);
	}

	void set(T v)
	{
		if (myValue != v)
		{
			myValue = v;
			publishChanged();
		}
	}

	operator T() const { return myValue; }
	inline T value() const { return myValue; }

private:
	T myValue;
};

} // namespace AV
