#pragma once

#include <Precomp.h>

namespace AV {

class Color
{
public:
	// Shift values and masks.

	static constexpr uint32_t RShift = 0;
	static constexpr uint32_t GShift = 8;
	static constexpr uint32_t BShift = 16;
	static constexpr uint32_t AShift = 24;

	static constexpr uint32_t RMask = 0xFF;
	static constexpr uint32_t GMask = 0xFF00;
	static constexpr uint32_t BMask = 0xFF0000;
	static constexpr uint32_t AMask = 0xFF000000;

	// Static colors.

	static const Color White;
	static const Color Black;
	static const Color Blank;

	// Creates a color with RGBA values of zero.
	constexpr Color()
		: value(0)
	{
	}

	// Creates a copy of another color.
	constexpr Color(const Color& other)
		: value(other.value)
	{
	}

	// Creates a copy of another color and alpha values (0-255).
	constexpr Color(const Color& other, int alpha)
		: value((alpha << AShift) | (other.value & ~AMask))
	{
	}

	// Creates a copy of another color.
	constexpr Color(Color&& other) noexcept
		: value(other.value)
	{
	}

	// Creates an 8-bit color from red, green, blue and alpha values (0-255).
	constexpr Color(int red, int green, int blue, int alpha)
		: value((alpha << AShift) | (blue << BShift) | (green << GShift) | (red << RShift))
	{
	}

	// Creates an 8-bit color from red, green, blue values (0-255). Alpha will be 255.
	constexpr Color(int red, int green, int blue)
		: value(AMask | (blue << BShift) | (green << GShift) | (red << RShift))
	{
	}

	// Creates an 8-bit color from a luminance and alpha value (0-255).
	constexpr Color(int luminance, int alpha)
		: value((alpha << AShift) | (luminance << BShift) | (luminance << GShift) | (luminance << RShift))
	{
	}

	// Returns the value of the red channel.
	inline int red() const { return (value & RMask) >> RShift; }

	// Returns the value of the green channel.
	inline int green() const { return (value & GMask) >> GShift; }

	// Returns the value of the blue channel.
	inline int blue() const { return (value & BMask) >> BShift; }

	// Returns the value of the alpha channel.
	inline int alpha() const { return (value & AMask) >> AShift; }

	// Assigns another color to this color.
	Color& operator = (const Color& other) { value = other.value; return *this; }

	// The raw color value;
	uint32_t value;

	auto operator <=> (const Color&) const = default;
};

} // namespace AV
