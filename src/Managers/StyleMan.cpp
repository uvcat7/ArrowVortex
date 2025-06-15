#include <Managers/StyleMan.h>

#include <Core/Xmr.h>
#include <Core/Utils.h>
#include <Core/StringUtils.h>

#include <Editor/Common.h>
#include <Managers/NoteskinMan.h>
#include <Managers/ChartMan.h>

#include <algorithm>
#include <vector>

namespace Vortex {
namespace {

static std::vector<vec2i> ReadColumnPairs(const XmrNode* node, const char* attrib, int numCols)
{
	std::vector<vec2i> out;
	XmrAttrib* pairs = node->attrib(attrib);
	if(pairs)
	{
		for(int i = 0; i < pairs->numValues; ++i)
		{
			const char* val = pairs->values[i];
			if(val[0] && val[1])
			{
				vec2i cols = {val[0] - 'A', val[1] - 'A'};
				if(cols.x >= 0 && cols.x < numCols && cols.y >= 0 && cols.y < numCols)
				{
					out.push_back(cols);
				}
			}
		}
	}
	return out;
}

static void ApplyMirror(std::vector<int>& cols, const std::vector<vec2i>& mirrors)
{
	for(auto& mirror : mirrors)
	{
		for(auto& col : cols)
		{
			if(col == mirror.x)
			{
				col = mirror.y;
			}
			else if(col == mirror.y)
			{
				col = mirror.x;
			}
		}
	}
}

static int* CreateMirrorTable(const XmrNode* node, const char* attrib, int numCols)
{
	int* out = nullptr;
	std::vector<vec2i> pairs = ReadColumnPairs(node, attrib, numCols);
	if(pairs.size())
	{
		out = new int[numCols];
		for(int col = 0; col < numCols; ++col)
		{
			out[col] = col;
		}
		for(auto& pair : pairs)
		{
			for(int col = 0; col < numCols; ++col)
			{
				if(out[col] == pair.x)
				{
					out[col] = pair.y;
				}
				else if(out[col] == pair.y)
				{
					out[col] = pair.x;
				}
			}
		}
	}
	return out;
}

static String IdToName(StringRef id)
{
	String out;
	char prev = ' ';
	for(char c : id)
	{
		if(c == '-') c = ' ';
		if(prev == ' ' && c >= 'a' && c <= 'z') c += 'A' - 'a';
		out += c;
		prev = c;
	}
	return out;
}

}; // anonymous namespace.

void DeleteStyle(Style* style)
{
	if(!style) return;

	delete[] style->mirrorTableH;
	delete[] style->mirrorTableV;

	delete[] style->padColPositions;
	delete[] style->padInitialFeetCols;

	delete style;
}

Style* NewStyle()
{
	Style* out = new Style;

	out->index = 0;
	out->numCols = 0;
	out->numPlayers = 0;

	out->mirrorTableH = nullptr;
	out->mirrorTableV = nullptr;

	out->padWidth = 0;
	out->padHeight = 0;

	out->padColPositions = nullptr;
	out->padInitialFeetCols = nullptr;

	return out;
}

Style* CreateStyle(StringRef id, int numCols, int numPlayers)
{
	Style* out = NewStyle();

	out->id = id;
	if(out->id.empty())
	{
		out->id = Str::fmt("kb%1-single").arg(numCols);
	}

	out->name = IdToName(out->id);

	if(numCols < 1 || numCols > SIM_MAX_COLUMNS)
	{
		int old = numCols;
		numCols = clamp<int>(numCols, 1, SIM_MAX_COLUMNS);
		HudError("Style %s has %i columns, ArrowVortex only supports 1-%i, using %i columns.",
			id.str(), old, SIM_MAX_COLUMNS, numCols);
	}

	if(numPlayers < 1 || numPlayers > SIM_MAX_PLAYERS)
	{
		int old = numPlayers;
		numPlayers = clamp<int>(numPlayers, 1, SIM_MAX_PLAYERS);
		HudError("Style %s has %i players, ArrowVortex only supports 1-%i, using %i players.",
			id.str(), old, SIM_MAX_PLAYERS, numPlayers);
	}

	out->numCols = numCols;
	out->numPlayers = numPlayers;

	return out;
}

Style* CreateStyle(const XmrNode* node)
{

	String id = node->get("id", "");
	int numCols = node->get("numCols", 0);
	int numPlayers = node->get("numPlayers", 1);

	// Check if the style is valid.
	if(id.empty())
	{
		HudError("One of the styles is missing an id field, ignoring style.");
		return nullptr;
	}
	else if(numCols < 1 || numCols > SIM_MAX_COLUMNS)
	{
		HudError("Style %s has %i columns, ArrowVortex only supports 1-%i, ignoring style.",
			id.str(), numCols, SIM_MAX_COLUMNS);
		return nullptr;
	}
	else if(numPlayers < 1 || numPlayers > SIM_MAX_PLAYERS)
	{
		HudError("Style %s has %i players, ArrowVortex only supports 1-%i, ignoring style.",
			id.str(), numPlayers, SIM_MAX_PLAYERS);
		return nullptr;
	}

	// At this point returning a style is guaranteed.
	Style* out = NewStyle();

	out->id = id;
	out->numCols = numCols;
	out->numPlayers = numPlayers;
	out->name = node->get("name", "");

	if(out->name.empty()) out->name = IdToName(id);

	// Read pad layout.
	XmrNode* padNode = node->child("pad");
	if(padNode)
	{
		int width = 0;
		int height = 0;
		std::vector<vec2i> pos(numCols, {0, 0});
		ForXmrAttribsNamed(row, padNode, "row")
		{
			const char* buttons = row->values[0];
			for(int x = 0; buttons[x]; ++x)
			{
				int col = buttons[x] - 'A';
				if(col >= 0 && col < numCols)
				{
					pos[col] = {x, height};
				}
				width = max(width, x + 1);
			}
			++height;
		}
		if(width > 0 && height > 0)
		{
			out->padWidth = width;
			out->padHeight = height;
			out->padColPositions = new vec2i[numCols];
			memcpy(out->padColPositions, pos.data(), sizeof(vec2i) * numCols);
		}
	}

	// Read the initial foot positions.
	if(out->padWidth > 0)
	{
		auto cols = ReadColumnPairs(node, "feetPos", numCols);
		cols.resize(numPlayers, {0, 0});
		out->padInitialFeetCols = new vec2i[numPlayers];
		memcpy(out->padInitialFeetCols, cols.data(), sizeof(vec2i) * numPlayers);
	}

	// Read column/row pairs for mirror operations.
	out->mirrorTableH = CreateMirrorTable(node, "mirrorH", numCols);
	out->mirrorTableV = CreateMirrorTable(node, "mirrorV", numCols);

	return out;
}

// ================================================================================================
// StyleManImpl.

#define STYLE_MAN ((StyleManImpl*)gStyle)

struct StyleManImpl : public StyleMan {

std::vector<Style*> myStyles;
int myNumDefaultStyles;
Style* myActiveStyle;

// ================================================================================================
// StyleManImpl :: constructor / destructor.

StyleManImpl()
{
	XmrDoc doc;
	if(doc.loadFile("settings/styles.txt") != XMR_SUCCESS)
	{
		HudError("Could not load styles file.");
	}
	ForXmrNodesNamed(node, &doc, "style")
	{
		Style* style = CreateStyle(node);
		if(style)
		{
			style->index = myStyles.size();
			myStyles.push_back(style);
		}
	}
	myNumDefaultStyles = myStyles.size();
	myActiveStyle = nullptr;
}

~StyleManImpl()
{
	for(auto style : myStyles)
	{
		DeleteStyle(style);
	}
}

// ================================================================================================
// StyleManImpl :: member functions.

static String getFallbackText(int numCols, int numPlayers)
{
	String out;
	if(numPlayers > 1)
	{
		out = Str::fmt("creating a fallback style (%1 columns, %1 players)").arg(numCols).arg(numPlayers);
	}
	else
	{
		out = Str::fmt("creating a fallback style (%1 columns)").arg(numCols);
	}
	return out;
}

const Style* createFallbackStyle(StringRef id, int numCols, int numPlayers)
{
	auto style = CreateStyle(id, numCols, numPlayers);
	style->index = myStyles.size();
	myStyles.push_back(style);
	return style;
}

const Style* findStyle(StringRef styleId)
{
	for(int i = 0; i < myStyles.size(); ++i)
	{
		if(myStyles[i]->id == styleId)
		{
			return myStyles[i];
		}
	}
	return nullptr;
}

const Style* findStyle(StringRef chartName, int numCols, int numPlayers)
{
	// First, check if an existing style matches the requested column and player count.
	for(int i = 0; i < myStyles.size(); ++i)
	{
		auto style = myStyles[i];
		if(style->numCols == numCols && style->numPlayers == numPlayers)
		{
			return style;
		}
	}

	// If that fails, create a fallback style with the given parameters.
	String text = getFallbackText(numCols, numPlayers);
	HudWarning("%s has an unknown style, %s.", chartName.str(), text.str());
	return createFallbackStyle(String(), numCols, numPlayers);
}

const Style* findStyle(StringRef chartName, int numCols, int numPlayers, StringRef id)
{
	/// Use lookup by column and player count if the id string is empty.
	if(id.empty())
	{
		// First, check if an existing style matches the requested column and player count.
		for(int i = 0; i < myStyles.size(); ++i)
		{
			auto style = myStyles[i];
			if(style->numCols == numCols && style->numPlayers == numPlayers)
			{
				HudWarning("%s is missing a style, using %s.", chartName.str(), style->name.str());
				return style;
			}
		}

		// If not, create a fallback style with the requested column and player count.
		String text = getFallbackText(numCols, numPlayers);
		HudWarning("%s is missing a style, %s.", chartName.str(), text.str());
		return createFallbackStyle(String(), numCols, numPlayers);
	}

	/// Try to find a match in the list of loaded styles.
	for(int i = 0; i < myStyles.size(); ++i)
	{
		auto style = myStyles[i];
		if(style->id == id)
		{
			return style;
		}
	}

	// If that fails, create a new style with the given parameters.
	String text = getFallbackText(numCols, numPlayers);
	HudWarning("%s has an unknown style %s, %s.", chartName.str(), id.str(), text.str());
	return createFallbackStyle(id, numCols, numPlayers);
}

void update(Chart* chart)
{
	myActiveStyle = (Style*)(chart ? chart->style : nullptr);
}

int getNumStyles() const
{
	return myNumDefaultStyles;
}

int getNumCols() const
{
	return myActiveStyle ? myActiveStyle->numCols : 0;
}

int getNumPlayers() const
{
	return myActiveStyle ? myActiveStyle->numPlayers : 0;
}

Style* get(int index) const
{
	return myStyles[index];
}

Style* get() const
{
	return myActiveStyle;
}

}; // StyleManImpl.

// ================================================================================================
// StyleMan :: create / destroy.

StyleMan* gStyle = nullptr;

void StyleMan::create()
{
	gStyle = new StyleManImpl;
}

void StyleMan::destroy()
{
	delete STYLE_MAN;
	gStyle = nullptr;
}

}; // namespace Vortex
