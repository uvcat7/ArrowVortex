#pragma once

#include <Core/Utils/Enum.h>

namespace AV {

// Enumeration of mouse buttons.
enum class MouseButton
{
	None = 0,

	LMB = 1,  // Left mouse button.
	MMB = 2,  // Middle mouse button.
	RMB = 3,  // Right mouse button.

	Max // One past the highest valid value.
};

// Enumeration of key codes.
enum class KeyCode
{
	None = 0,

	A = 'a',
	B = 'b',
	C = 'c',
	D = 'd',
	E = 'e',
	F = 'f',
	G = 'g',
	H = 'h',
	I = 'i',
	J = 'j',
	K = 'k',
	L = 'l',
	M = 'm',
	N = 'n',
	O = 'o',
	P = 'p',
	Q = 'q',
	R = 'r',
	S = 's',
	T = 't',
	U = 'u',
	V = 'v',
	W = 'w',
	X = 'x',
	Y = 'y',
	Z = 'z',

	Digit0 = '0',
	Digit1 = '1',
	Digit2 = '2',
	Digit3 = '3',
	Digit4 = '4',
	Digit5 = '5',
	Digit6 = '6',
	Digit7 = '7',
	Digit8 = '8',
	Digit9 = '9',

	Accent = '`',
	Dash = '-',
	Equal = '=',
	BracketL = '[',
	BracketR = ']',
	Semicolon = ';',
	Quote = '\'',
	Backslash = '\\',
	Comma = ',',
	Period = '.',
	Slash = '/',
	Space = ' ',

	// Non-character key codes start at 128 to prevent clashes with ascii characters.
	Escape = 128,

	CtrlL,   // Left control key.
	CtrlR,   // Right control key.
	AltL,    // Left alt key.
	AltR,    // Right alt key.
	ShiftL,  // Left shift key.
	ShiftR,  // Right shift key.
	SystemL, // Left system key.
	SystemR, // Right system key.

	Tab,
	Caps,
	Return,
	Backspace,
	PageUp,
	PageDown,
	Home,
	End,
	Insert,
	Delete,
	PrintScreen,
	ScrollLock,
	Pause,

	Left,  // Left arrow key.
	Right, // Right arrow key.
	Up,    // Up arrow key.
	Down,  // Down arrow key.

	NumLock,

	NumpadDivide,    // Numpad divide key.
	NumpadMultiply,  // Numpad multiply key.
	NumpadSubtract,  // Numpad subtract key.
	NumpadAdd,       // Numpad add key.
	NumpadSeperator, // Numpad decimal seperation key.

	Numpad0,
	Numpad1,
	Numpad2,
	Numpad3,
	Numpad4,
	Numpad5,
	Numpad6,
	Numpad7,
	Numpad8,
	Numpad9,

	F1,
	F2,
	F3,
	F4,
	F5,
	F6,
	F7,
	F8,
	F9,
	F10,
	F11,
	F12,
	F13,
	F14,
	F15,

	Max // One past the highest valid value.
};

// Represents a combination of zero or more modifier keys.
enum class ModifierKeys
{
	None  = 0,
	Ctrl  = 1 << 0,
	Alt   = 1 << 1,
	Shift = 1 << 2,
};

template <>
constexpr bool IsFlags<ModifierKeys> = true;

// Combination of key code and modifiers.
struct KeyCombination
{
	KeyCode code = KeyCode::None;
	ModifierKeys modifiers = ModifierKeys::None;

	std::string notation() const;

	auto operator <=> (const KeyCombination&) const = default;
};

} // namespace AV