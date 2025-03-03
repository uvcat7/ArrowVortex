#pragma once

#include <Common/Input.h>

#include <Vortex/Commands.h>

namespace AV {

// A keyboard shortcut that executes a command.
struct Shortcut
{
	KeyCombination key;
	KeyCombination followUp;
	Command* command;
	std::string name;
};

// A named group of shortcuts.
struct ShortcutGroup
{
	std::string name;
	vector<Shortcut> shortcuts;
};

// Global shortcut functions.
namespace Shortcuts
{
	void initialize(const XmrDoc& settings);
	void deinitialize();

	// Gets all registered shortcuts.
	std::span<const ShortcutGroup> getShortcuts();
};

} // namespace AV
