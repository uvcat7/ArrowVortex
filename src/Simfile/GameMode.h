#pragma once

#include <Precomp.h>

#include <Core/Types/Vec2.h>

namespace AV {

// Holds data that determines the game type of a chart.
struct GameMode
{
	typedef uint Index;

	~GameMode();

	// Creates a game mode with the given identifier, number of columns and number of players.
	GameMode(std::string id, int numCols, int numPlayers);

	// Unique identifier of the game mode (e.g. "dance-single", "pump-double").
	std::string id;

	// Name of the game to which the mode belongs (e.g. "Dance", "Pump").
	std::string game;

	// Name of the mode within the game (e.g. "Single", "Double").
	std::string mode;

	// Index of the game mode in the global game mode list, used for sorting.
	Index index;

	// Number of supported columns.
	int numCols;

	// Number of supported players.
	int numPlayers;

	// Table with the color scheme of each column, has [numCols] elements.
	int* colorTable;

	// The horizontal mirror mapping of each column, has [numCols] elements.
	// The index is the start column, the value is the resulting column.
	int* mirrorTableH;

	// The vertical mirror mapping of each column, has [numCols] elements.
	// The index is the start column, the value is the resulting column.
	int* mirrorTableV;

	// The width of the pad layout, in number of panels.
	int padWidth;

	// The height of the pad layout, in number of panels.
	int padHeight;

	// The (x,y) position of the panel on the pad layout for each column, has [numCols] elements.
	Vec2* padColPositions;

	// The initial column index for the left and right foot of each player, has [numPlayers] elements.
	Vec2* padInitialFeetCols;
};

// Manages all available game modes.
namespace GameModeMan
{
	void initialize(const XmrDoc& settings);
	void deinitialize();

	// Adds a game to the list.
	void addGame(stringref name);

	// Adds a game mode to the list.
	void addMode(GameMode* gameMode);

	// Clears the list and deallocates the associated memory.
	void clear();

	// Returns the first game mode that matches the given id, or null if none was found.
	const GameMode* find(stringref id);

	// Returns the first game mode that matches the given column and player count.
	// If a match is not found, a new game mode is created, added and returned.
	const GameMode* find(int numCols, int numPlayers);

	// Returns the first game mode that matches the given id, column, and player count.
	// If a match is not found, a new mode is created, added and returned.
	const GameMode* find(stringref id, int numCols, int numPlayers);

	// Returns the game mode at the given global index.
	GameMode* fromIndex(GameMode::Index index);

	// Get the list of registered games.
	vector<std::string> getGames();

	// Gets the list of available game modes for the given game.
	vector<GameMode*> getModes(stringref game);
};

} // namespace AV
