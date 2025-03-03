#pragma once

#include <Core/Graphics/Color.h>

namespace AV {

class Colorf
{
public:
	// Structure representing a hue, saturation, value and alpha tuple.
	struct HSVA { float hue, saturation, value, alpha; };

	// Creates a color with RGBA values of zero.
	constexpr Colorf()
		: red(0)
		, green(0)
		, blue(0)
		, alpha(0)
	{
	}

	// Creates a color from red, green, blue and alpha values (0-1).
	constexpr Colorf(float red, float green, float blue, float alpha)
		: red(red)
		, green(green)
		, blue(blue)
		, alpha(alpha)
	{
	}

	// Creates a color from luminance and alpha values (0-1).
	constexpr Colorf(float luminance, float alpha)
		: red(luminance)
		, green(luminance)
		, blue(luminance)
		, alpha(alpha)
	{
	}

	// Creates a color from an 8-bit RGBA color.
	constexpr Colorf(const Color& color)
		: red(u8tof(color.red()))
		, green(u8tof(color.green()))
		, blue(u8tof(color.blue()))
		, alpha(u8tof(color.alpha()))
	{
	}

	// Creates a color from hue, saturation, value and alpha components.
	Colorf(HSVA hsva);

	// Creates a color from hue, saturation and value components with a separate alpha value.
	Colorf(HSVA hsv, float alpha);

	// The value of the red channel.
	float red;

	// The value of the green channel.
	float green;

	// The value of the blue channel.
	float blue;

	// The value of the alpha channel.
	float alpha;

	// Converts the color to 8-bit RGBA.
	Color toU8() const { return Color(ftou8(red), ftou8(green), ftou8(blue), ftou8(alpha)); }

	// Converts the color from RGB to HSV.
	HSVA toHSVA() const;

	// Converts the given float (0-1) to an 8-bit unsigned int (0-255).
	static constexpr int ftou8(float v)
	{
		int i = int(v * 255.0f);
		return (i > 0) ? ((i < 256) ? i : 255) : 0;
	}

	// Converts the given unsigned int (0-255) to float (0-1).
	static constexpr float u8tof(int v)
	{
		return v * (1 / 255.0f);
	}

	auto operator <=> (const Colorf&) const = default;
};

} // namespace AV
