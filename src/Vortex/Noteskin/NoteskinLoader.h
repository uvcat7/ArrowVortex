#pragma once

#include <Precomp.h>

#include <Vortex/Noteskin/Noteskin.h>

namespace AV {

// Reads noteskin data from file.
struct NoteskinLoader
{
	// Loads the noteskin file of a specific game mode (e.g. "dance-single.txt").
	static shared_ptr<Noteskin> load(const NoteskinStyle* style, const GameMode* mode);
};

} // namespace AV
