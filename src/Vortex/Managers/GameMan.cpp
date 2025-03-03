#include <Precomp.h>

#include <Core/Utils/Xmr.h>
#include <Core/Utils/Util.h>
#include <Core/Utils/String.h>
#include <Core/Utils/vector.h>

#include <Core/System/Log.h>

#include <Vortex/Common.h>

#include <Vortex/Noteskin/NoteskinMan.h>
#include <Vortex/Managers/GameMan.h>

namespace AV {

using namespace std;
using namespace Util;

static vector<Vec2> ReadColumnPairs(const XmrNode* node, const char* attribName, int numCols)
{
	vector<Vec2> out;
	auto pairs = String::split(node->get(attribName, ""));
	for (auto& val : pairs)
	{
		if (val[0] && val[1])
		{
			Vec2 cols = {val[0] - 'A', val[1] - 'A'};
			if (cols.x >= 0 && cols.x < numCols && cols.y >= 0 && cols.y < numCols)
			{
				out.push_back(cols);
			}
		}
	}
	return out;
}

static void ApplyMirror(vector<int>& cols, const vector<Vec2>& mirrors)
{
	for (auto& mirror : mirrors)
	{
		for (auto& col : cols)
		{
			if (col == mirror.x)
			{
				col = mirror.y;
			}
			else if (col == mirror.y)
			{
				col = mirror.x;
			}
		}
	}
}

static int* CreateColorTable(const XmrNode* node, int numCols)
{
	int* out = nullptr;
	auto colorScheme = String::split(node->get("colorScheme", ""));
	if (!colorScheme.empty())
	{
		out = new int[numCols];
		for (int col = 0; col < numCols; ++col)
		{
			out[col] = 0;
		}
		for (int i = 0, n = min((int)colorScheme.size(), numCols); i < n; ++i)
		{
			int val = String::toInt(colorScheme[i], -1);
			if (val >= 0 && val <= numCols)
			{
				out[i] = val;
			}
		}
	}
	return out;
}

static int* CreateMirrorTable(const XmrNode* node, const char* attrib, int numCols)
{
	int* out = nullptr;
	vector<Vec2> pairs = ReadColumnPairs(node, attrib, numCols);
	if (pairs.size())
	{
		out = new int[numCols];
		for (int col = 0; col < numCols; ++col)
		{
			out[col] = col;
		}
		for (auto& pair : pairs)
		{
			for (int col = 0; col < numCols; ++col)
			{
				if (out[col] == pair.x)
				{
					out[col] = pair.y;
				}
				else if (out[col] == pair.y)
				{
					out[col] = pair.x;
				}
			}
		}
	}
	return out;
}

static GameMode* NewGameMode(const XmrNode* node, stringref game)
{
	string id = node->get("id", "");
	int numCols = node->get("numCols", 0);
	int numPlayers = node->get("numPlayers", 1);

	// Check if the mode is valid.
	if (id.empty())
	{
		Log::error("One of the modes is missing an id field, ignoring mode.");
		return nullptr;
	}

	// At this point returning a mode is guaranteed.
	GameMode* out = new GameMode(id, numCols, numPlayers);

	out->game = game;
	out->mode = node->get("name", "");

	// Read pad layout.
	XmrNode* padNode = node->child("pad");
	if (padNode)
	{
		int width = 0;
		int height = 0;
		vector<Vec2> pos(numCols, {0, 0});
		for (auto row : padNode->children("row"))
		{
			const char* buttons = row->value.data();
			for (int x = 0; buttons[x]; ++x)
			{
				int col = buttons[x] - 'A';
				if (col >= 0 && col < numCols)
				{
					pos[col] = {x, height};
				}
				width = max(width, x + 1);
			}
			++height;
		}
		if (width > 0 && height > 0)
		{
			out->padWidth = width;
			out->padHeight = height;
			out->padColPositions = new Vec2[numCols];
			memcpy(out->padColPositions, pos.data(), sizeof(Vec2) * numCols);
		}
	}

	// Read the initial foot positions.
	if (out->padWidth > 0)
	{
		auto cols = ReadColumnPairs(node, "feetPos", numCols);
		Vector::resize(cols, {0, 0}, numPlayers);
		out->padInitialFeetCols = new Vec2[numPlayers];
		memcpy(out->padInitialFeetCols, cols.data(), sizeof(Vec2) * numPlayers);
	}

	// Read coloring table for noteskin colors.
	out->colorTable = CreateColorTable(node, numCols);

	// Read column/row pairs for mirror operations.
	out->mirrorTableH = CreateMirrorTable(node, "mirrorH", numCols);
	out->mirrorTableV = CreateMirrorTable(node, "mirrorV", numCols);

	return out;
}

// =====================================================================================================================
// Initialization.

void GameMan::initialize(const XmrDoc& settings)
{
	XmrDoc doc;
	if (doc.loadFile("settings/game modes.txt") != XmrResult::Success)
	{
		Log::error("Could not load games file.");
	}
	for (auto m : doc.children("game"))
	{
		const char* game = m->get("name");
		if (game && game[0])
		{
			GameModeMan::addGame(game);
			for (auto n : m->children("mode"))
			{
				GameMode* gameMode = NewGameMode(n, game);
				if (gameMode)
				{
					GameModeMan::addMode(gameMode);
				}
				else
				{
					Log::error(format("The game {} has an invalid mode", game));
				}
			}
		}
		else
		{
			Log::error("The game modes file has a game without name.");
		}
	}
}

void GameMan::deinitialize()
{
}

} // namespace AV
