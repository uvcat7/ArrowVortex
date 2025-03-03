#pragma once

#include <Core/Common/Event.h>
#include <Core/Graphics/Draw.h>

#include <Vortex/Common.h>

#include <Vortex/Noteskin/Noteskin.h>

namespace AV {

// Manages the noteskin of the active chart.
namespace NoteskinMan
{
	void initialize(const XmrDoc& settings);
	void deinitialize();

	// Called when the noteskin settings change or noteskins are manually reloaded.
	void reload();

	// Gives a noteskin style the highest priority. Current loaded noteskins are updated to the given style if supported.
	void setPrimaryStyle(int index);

	// List of supported noteskin styles.
	std::span<shared_ptr<NoteskinStyle>> supportedStyles();

	// Returns the noteskin for the given game mode, and loads it if necessary.
	shared_ptr<Noteskin> getNoteskin(const GameMode* mode);

	struct StylesChanged : Event {};
};

} // namespace AV
