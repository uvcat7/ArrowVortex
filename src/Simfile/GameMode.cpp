#include <Precomp.h>

#include <Core/Utils/Util.h>
#include <Core/Utils/String.h>
#include <Core/Utils/vector.h>

#include <Core/System/Log.h>

#include <Simfile/Common.h>
#include <Simfile/GameMode.h>

namespace AV {

using namespace std;
using namespace Util;

namespace GameModeMan
{
	struct State
	{
		vector<string> games;
		vector<GameMode*> standardModes;
		vector<GameMode*> generatedModes;
	};
	static State* state = nullptr;
}
using GameModeMan::state;

static GameMode::Index MakeGameIndex(bool generated, int mode)
{
	return (generated << 31) | mode;
}

static void SplitGameIndex(GameMode::Index index, bool& generated, int& mode)
{
	generated = (index >> 31) & 1;
	mode = index & 0x7FFFFFFF;
}

static string SplitId(string id, string& outGame, string& outMode)
{
	string out;
	bool prefix = true;
	char prev = ' ';
	for (char c : id)
	{
		if (prev == ' ' && c >= 'a' && c <= 'z') c += 'A' - 'a';
		if (prefix)
		{
			if (c == '-')
			{
				prefix = false;
			}
			else
			{
				outGame += c;
			}
		}
		else
		{
			if (c == '-') c = ' ';
			outMode += c;
		}
		prev = c;
	}
	return out;
}

static string GetFallbackText(int numCols, int numPlayers)
{
	string out;
	if (numPlayers > 1)
	{
		out = format("using fallback ({} columns, {} players)", numCols, numPlayers);
	}
	else
	{
		out = format("using fallback ({} columns)", numCols);
	}
	return out;
}

const GameMode* CreateFallbackGameMode(string id, int numCols, int numPlayers)
{
	if (id.empty()) id = format("kb{}-single", numCols);

	GameMode* gameMode = new GameMode(id, numCols, numPlayers);

	SplitId(gameMode->id, gameMode->game, gameMode->mode);

	gameMode->index = MakeGameIndex(true, (int)state->generatedModes.size());

	state->generatedModes.push_back(gameMode);

	return gameMode;
}

GameMode::GameMode(string id, int numCols, int numPlayers)
	: index(0)
	, id(id)
	, numCols(numCols)
	, numPlayers(numPlayers)
	, colorTable(nullptr)
	, mirrorTableH(nullptr)
	, mirrorTableV(nullptr)
	, padWidth(0)
	, padHeight(0)
	, padColPositions(nullptr)
	, padInitialFeetCols(nullptr)
{

	if (numCols < 1 || numCols > SimfileConstants::MaxColumns)
	{
		int old = numCols;
		numCols = clamp(numCols, 1, SimfileConstants::MaxColumns);
		Log::error(format("Game mode {} has {} columns, ArrowVortex only supports 1-{}, using {} columns.",
			id, old, SimfileConstants::MaxColumns, numCols));
	}

	if (numPlayers < 1 || numPlayers > SimfileConstants::MaxPlayers)
	{
		int old = numPlayers;
		numPlayers = clamp(numPlayers, 1, SimfileConstants::MaxPlayers);
		Log::error(format("Game mode {} has {} players, ArrowVortex only supports 1-{}, using {} players.",
			id, old, SimfileConstants::MaxPlayers, numPlayers));
	}
}

GameMode::~GameMode()
{
	delete[] colorTable;

	delete[] mirrorTableH;
	delete[] mirrorTableV;

	delete[] padColPositions;
	delete[] padInitialFeetCols;
}

void GameModeMan::initialize(const XmrDoc& settings)
{
	state = new State();
}

void GameModeMan::deinitialize()
{
	for (auto mode : state->standardModes)
		delete mode;

	for (auto mode : state->generatedModes)
		delete mode;

	Util::reset(state);
}

void GameModeMan::addGame(stringref name)
{
	if (!Vector::contains(state->games, name))
	{
		state->games.push_back(name);
	}
}

void GameModeMan::addMode(GameMode* gameMode)
{
	state->standardModes.push_back(gameMode);
	gameMode->index = MakeGameIndex(false, (int)state->standardModes.size());
}

const GameMode* GameModeMan::find(stringref id)
{
	for (auto mode : state->standardModes)
	{
		if (mode->id == id)
		{
			return mode;
		}
	}
	for (auto mode : state->generatedModes)
	{
		if (mode->id == id)
		{
			return mode;
		}
	}
	
	return nullptr;
}

const GameMode* GameModeMan::find(int numCols, int numPlayers)
{
	// First, check if an existing mode matches the requested column and player count.
	for (auto mode : state->standardModes)
	{
		if (mode->numCols == numCols && mode->numPlayers == numPlayers)
		{
			return mode;
		}
	}
	for (auto mode : state->generatedModes)
	{
		if (mode->numCols == numCols && mode->numPlayers == numPlayers)
		{
			return mode;
		}
	}

	// If that fails, create a fallback mode with the given parameters.
	string text = GetFallbackText(numCols, numPlayers);
	Log::warning(format("Missing game mode, {}.", text));
	return CreateFallbackGameMode(string(), numCols, numPlayers);
}

const GameMode* GameModeMan::find(stringref id, int numCols, int numPlayers)
{
	// Use lookup by column and player count if the id string is empty.
	if (id.empty())
	{
		// First, check if an existing mode matches the requested column and player count.
		for (auto mode : state->standardModes)
		{
			if (mode->numCols == numCols && mode->numPlayers == numPlayers)
			{
				Log::warning(format("Missing game mode, using {}.", mode->mode));
				return mode;
			}
		}
		for (auto mode : state->generatedModes)
		{
			if (mode->numCols == numCols && mode->numPlayers == numPlayers)
			{
				Log::warning(format("Missing game mode, using {}.", mode->mode));
				return mode;
			}
		}

		// If not, create a fallback mode with the requested column and player count.
		string text = GetFallbackText(numCols, numPlayers);
		Log::warning(format("Missing game mode, {}.", text));
		return CreateFallbackGameMode(string(), numCols, numPlayers);
	}

	// Try to find a match in the list of currently loaded modes.
	for (auto mode : state->standardModes)
	{
		if (mode->id == id)
		{
			return mode;
		}
	}
	for (auto mode : state->generatedModes)
	{
		if (mode->id == id)
		{
			return mode;
		}
	}

	// If that fails, create a new mode with the given parameters.
	string text = GetFallbackText(numCols, numPlayers);
	Log::warning(format("Unknown game mode {}, {}", id, text));
	return CreateFallbackGameMode(id, numCols, numPlayers);
}

GameMode* GameModeMan::fromIndex(GameMode::Index index)
{
	int mode;
	bool generated;
	SplitGameIndex(index, generated, mode);

	if (generated)
	{
		if (mode < (int)state->generatedModes.size())
			return state->generatedModes[mode];
	}
	else
	{
		if (mode < (int)state->standardModes.size())
			return state->standardModes[mode];
	}

	return nullptr;
}

vector<string> GameModeMan::getGames()
{
	if (!state) return vector<string>();

	return state->games;
}

vector<GameMode*> GameModeMan::getModes(stringref game)
{
	auto modes = vector<GameMode*>(10);
	modes.reserve(10);

	for (auto& mode : state->standardModes)
	{
		if (mode->game == game)
		{
			modes.push_back(mode);
		}
	}

	return modes;
}

} // namespace AV
