#include <Precomp.h>

#include <Simfile/GameMode.h>

#include <Vortex/Noteskin/Noteskin.h>

namespace AV {

Noteskin::Noteskin(const NoteskinStyle* style, const GameMode* mode)
	: style(style)
	, gameMode(mode)
	, width(0)
{
	columns.resize(mode->numCols);
	for (int c = 0; c < mode->numCols; ++c)
		columns[c].players.resize(mode->numPlayers);
}

} // namespace AV
