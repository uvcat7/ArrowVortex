#pragma once

#include <Core/Graphics/Draw.h>

#include <Vortex/Common.h>

namespace AV {

// Holds metadata for a supported noteskin style.
struct NoteskinStyle
{
	std::string name;
	bool hasFallback = false;
	std::map<const GameMode*, std::string> modes;
};

// Holds noteskin data for a single column.
struct NoteskinColumn
{
	float x;

	struct PlayerSprites
	{
		SpriteEx notes[(size_t)Quantization::Count];
		SpriteEx mine;
	};

	Texture notesTex;
	Texture receptorsTex;
	Texture holdsTex;
	Texture glowTex;

	vector<PlayerSprites> players;

	SpriteEx holdBody;
	SpriteEx holdTail;

	SpriteEx rollBody;
	SpriteEx rollTail;

	SpriteEx receptorOn;
	SpriteEx receptorOff;
	SpriteEx receptorGlow;
};

// Holds data that determines the appearance of the notefield for a specific game mode.
struct Noteskin
{
	Noteskin(const NoteskinStyle* style, const GameMode* mode);

	const NoteskinStyle* style;
	const GameMode* gameMode;
	float width;

	vector<NoteskinColumn> columns;
};

} // namespace AV
