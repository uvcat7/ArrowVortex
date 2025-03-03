#pragma once

#include <Precomp.h>

namespace AV {
namespace SimfileConstants {

constexpr int MaxColumns = 32;
constexpr int MaxPlayers = 16;
constexpr int MaxKeysounds = 4095;

} // Simfile constants

// Supported note types.
enum class NoteType
{
	None,

	Step,
	Mine,
	Hold,
	Roll,
	Lift,
	Fake,
	AutomaticKeysound,

	Count
};

// Represents a row/column index.
struct RowCol
{
	Row row;
	int col;
};

// Represents a single note.
struct Note
{
	Note()
		: row(0)
		, endRow(0)
		, type(0)
		, player(0)
		, keysoundId(0)
		, isWarped(0)
		, unused(0)
	{
	}

	Note(Row row, Row endRow, NoteType type, uint player, uint keysoundId)
		: row(row)
		, endRow(endRow)
		, type((uint)type)
		, player(player)
		, keysoundId(keysoundId)
		, isWarped(0)
		, unused(0)
	{
	}

	Row row;
	Row endRow;

	uint type : 4;
	uint player : 4;
	uint keysoundId : 12;
	uint isWarped : 1;
	uint unused : 3;
};

} // namespace AV
