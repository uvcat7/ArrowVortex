#include <Precomp.h>

#include <Core/Common/Input.h>

#include <Core/Utils/Flag.h>

namespace AV {

using namespace std;

static unordered_map<KeyCode, const char*> KeyToNotation = {
	{ KeyCode::Accent, "`" },
	{ KeyCode::Dash, "-" },
	{ KeyCode::Equal, "=" },
	{ KeyCode::BracketL, "[" },
	{ KeyCode::BracketR, "]" },
	{ KeyCode::Semicolon, ";" },
	{ KeyCode::Quote, "'" },
	{ KeyCode::Backslash, "\\" },
	{ KeyCode::Comma, "," },
	{ KeyCode::Period, "." },
	{ KeyCode::Slash, "/" },
	{ KeyCode::Space, " " },
	{ KeyCode::CtrlL, "Ctrl" },
	{ KeyCode::CtrlR, "Ctrl" },
	{ KeyCode::AltL, "Alt" },
	{ KeyCode::AltR, "Alt" },
	{ KeyCode::ShiftL, "Shift" },
	{ KeyCode::ShiftR, "Shift" },
	{ KeyCode::SystemL, "System" },
	{ KeyCode::SystemR, "System" },
	{ KeyCode::Tab, "Tab" },
	{ KeyCode::Caps, "Caps" },
	{ KeyCode::Return, "Enter" },
	{ KeyCode::Backspace, "Bkspce" },
	{ KeyCode::PageUp, "PgUp" },
	{ KeyCode::PageDown, "PgDn" },
	{ KeyCode::Home, "Home" },
	{ KeyCode::End, "End" },
	{ KeyCode::Insert, "Ins" },
	{ KeyCode::Delete, "Del" },
	{ KeyCode::PrintScreen, "PrtScn" },
	{ KeyCode::ScrollLock, "ScrollLock" },
	{ KeyCode::Pause, "Pause" },
	{ KeyCode::Left, "Left" },
	{ KeyCode::Right, "Right" },
	{ KeyCode::Up, "Up" },
	{ KeyCode::Down, "Down" },
	{ KeyCode::NumLock, "NumLock" },
	{ KeyCode::NumpadDivide, "Num /" },
	{ KeyCode::NumpadMultiply, "Num *" },
	{ KeyCode::NumpadSubtract, "Num -" },
	{ KeyCode::NumpadAdd, "Num +" },
	{ KeyCode::NumpadSeperator, "Num ." },
};

static char ToChar(KeyCode value, KeyCode subtract, char add)
{
	return char((int)value - (int)subtract) + add;
}

string KeyCombination::notation() const
{
	string result;

	if (modifiers & ModifierKeys::Ctrl)
		result += "Ctrl+";

	if (modifiers & ModifierKeys::Shift)
		result += "Shift+";

	if (modifiers & ModifierKeys::Alt)
		result += "Alt+";

	if (code >= KeyCode::A && code <= KeyCode::Z)
		return result.append(1, ToChar(code, KeyCode::A, 'A'));

	if (code >= KeyCode::Digit0 && code <= KeyCode::Digit9)
		return result.append(1, ToChar(code, KeyCode::Digit0, '0'));

	if (code >= KeyCode::Numpad0 && code <= KeyCode::Numpad9)
		return result.append("Num ").append(1, ToChar(code, KeyCode::Numpad0, '0'));

	if (code >= KeyCode::F1 && code <= KeyCode::F15)
		return result.append(1, 'F').append(1, ToChar(code, KeyCode::F1, '1'));

	auto it = KeyToNotation.find(code);
	if (it != KeyToNotation.end())
		return result + it->second;

	return result.append("Unknown");
}

} // namespace AV