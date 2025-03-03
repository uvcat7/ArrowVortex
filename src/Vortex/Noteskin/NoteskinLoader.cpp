#include <Precomp.h>

#include <Core/Utils/Xmr.h>
#include <Core/Utils/String.h>
#include <Core/Utils/Ascii.h>
#include <Core/Utils/Util.h>
#include <Core/Utils/Flag.h>

#include <Core/System/File.h>
#include <Core/System/Log.h>
#include <Core/Graphics/Texture.h>

#include <Simfile/GameMode.h>

#include <Vortex/Noteskin/NoteskinLoader.h>

namespace AV {

using namespace std;

using namespace Util;

// =====================================================================================================================
// Intermediate noteskin data.

struct IntermediateSprite
{
	Rect src;
	float scale = 1.0f;
	float uvs[8] = {0, 0, 1, 0, 0, 1, 1, 1};
};

struct IntermediateNoteskin
{
	IntermediateNoteskin(const NoteskinStyle* style, const GameMode* mode);

	const NoteskinStyle* style;
	const GameMode* gameMode;

	float notefieldWidth = 0.0;

	vector<Texture> notesTex;             // [columns]
	vector<Texture> receptorsTex;         // [columns]
	vector<Texture> holdsTex;             // [columns]
	vector<Texture> glowTex;              // [columns]

	vector<IntermediateSprite> notes;     // [columns][players][quantizations]
	vector<IntermediateSprite> mines;     // [columns][players]
	vector<IntermediateSprite> holds;     // [columns][2 (body,tail)]
	vector<IntermediateSprite> rolls;     // [columns][2 (body,tail)]
	vector<IntermediateSprite> receptors; // [columns][2 (off,on)]
	vector<IntermediateSprite> glow;      // [columns]

	vector<float> columnX;                // [columns]
};

IntermediateNoteskin::IntermediateNoteskin(const NoteskinStyle* style, const GameMode* gm)
	: style(style)
	, gameMode(gm)
{
	int numCols = gm->numCols;
	int numPlayers = gm->numPlayers;

	notesTex.resize(numCols);
	receptorsTex.resize(numCols);
	holdsTex.resize(numCols);
	glowTex.resize(numCols);

	notes.resize(numCols * numPlayers * (size_t)Quantization::Count);
	mines.resize(numCols * numPlayers);
	holds.resize(numCols * 2);
	rolls.resize(numCols * 2);
	receptors.resize(numCols * 2);
	glow.resize(numCols);

	columnX.resize(numCols);
}

// =====================================================================================================================
// Texture coordinate transforms.

typedef void(*TranformFunction)(float[8]);

static void rotate270(float uvs[8])
{
	float result[] = { uvs[2], uvs[3], uvs[6], uvs[7], uvs[0], uvs[1], uvs[4], uvs[5] };
	memcpy(uvs, result, sizeof(float) * 8);
}

static void rotate90(float uvs[8])
{
	float result[] = { uvs[4], uvs[5], uvs[0], uvs[1], uvs[6], uvs[7], uvs[2], uvs[3] };
	memcpy(uvs, result, sizeof(float) * 8);
}

static void rotate180(float uvs[8])
{
	float result[] = { uvs[6], uvs[7], uvs[4], uvs[5], uvs[2], uvs[3], uvs[0], uvs[1] };
	memcpy(uvs, result, sizeof(float) * 8);
}

static void mirrorHorizontally(float uvs[8])
{
	float result[] = { uvs[2], uvs[3], uvs[0], uvs[1], uvs[6], uvs[7], uvs[4], uvs[5] };
	memcpy(uvs, result, sizeof(float) * 8);
}

static void mirrorVertically(float uvs[8])
{
	float result[] = { uvs[4], uvs[5], uvs[6], uvs[7], uvs[0], uvs[1], uvs[2], uvs[3] };
	memcpy(uvs, result, sizeof(float) * 8);
}

// =====================================================================================================================
// Error reporting.

static bool reportError(stringref message)
{
	Log::error(message);
	return false;
}

static bool reportError(const char* message)
{
	Log::error(message);
	return false;
}

static bool reportInvalidSelector(const XmrNode* node, string selector)
{
	return reportError(format("Node '{}' has invalid selector '{}'.", node->name, selector));
}

static bool reportOutOfRange(const XmrNode* node, string selector)
{
	return reportError(format("Node '{}' has selector '{}' which is out of range.", node->name, selector));
}

static bool reportUnknownAttribute(const XmrNode* attr)
{
	return reportError(format("Unknown attribute '{}'.", attr->name));
}

static bool reportInvalidValues(const XmrNode* attr)
{
	return reportError(format("Attribute '{}' has bad value: {}.", attr->name, attr->value));
}

// =====================================================================================================================
// Selectors parsing.

class Selector
{
public:
	enum class Mask
	{
		Columns      = 1 << 0,
		Players      = 1 << 1,
		Quantization = 1 << 2,
		OffOn        = 1 << 3,
		BodyTail     = 1 << 4,
	};

	Selector(int numColumns, int numPlayers);

	bool set(const XmrNode* node, const vector<string>& tokens, Mask mask);

	deque<int> indices;

private:
	const int myNumPlayers;
	const int myNumColumns;

	vector<int> myColumns;
	vector<int> myPlayers;
	vector<int> myQuantizations;
	vector<int> myOffOn;
	vector<int> myBodyTail;

	bool readTokens(const XmrNode* node, const vector<string>& tokens, Mask mask);
	void expandIndices(const vector<int>& indices, int range);
};

template <>
constexpr bool IsFlags<Selector::Mask> = true;

Selector::Selector(int numColumns, int numPlayers)
	: myNumColumns(numColumns)
	, myNumPlayers(numPlayers)
{
}

static bool isNumberSelector(stringref token, char prefix, int& number)
{
	if (token[0] == prefix && all_of(token.begin() + 1, token.end(), Ascii::isDigit))
		return String::readInt(token.substr(1), number);

	return false;
}

static bool convertQuantizationToIndex(int& q)
{
	switch (q)
	{
	case 4:	q = 0; break;
	case 8: q = 1; break;
	case 12: q = 2; break;
	case 16: q = 3; break;
	case 24: q = 4; break;
	case 32: q = 5; break;
	case 48: q = 6; break;
	case 64: q = 7; break;
	case 192: q = 8; break;
	default: return false;
	}
	return true;
}

bool Selector::set(const XmrNode* node, const vector<string>& tokens, Mask mask)
{
	if (!readTokens(node, tokens, mask))
		return false;

	indices.clear();
	indices.push_back(0);

	if (mask & Mask::Columns)
		expandIndices(myColumns, myNumColumns);

	if (mask & Mask::Players)
		expandIndices(myPlayers, myNumPlayers);

	if (mask & Mask::Quantization)
		expandIndices(myQuantizations, (int)Quantization::Count);

	if (mask & Mask::OffOn)
		expandIndices(myOffOn, 2);

	if (mask & Mask::BodyTail)
		expandIndices(myBodyTail, 2);

	return true;
}

bool Selector::readTokens(const XmrNode* node, const vector<string>& tokens, Mask mask)
{
	myPlayers.clear();
	myColumns.clear();
	myOffOn.clear();
	myBodyTail.clear();
	myQuantizations.clear();

	int number;
	for (size_t i = 1; i < tokens.size(); ++i)
	{
		auto& token = tokens[i];
		if (isNumberSelector(token, 'p', number))
		{
			if (!(mask & Mask::Players) || number < 1 || number > myNumPlayers)
				return reportInvalidSelector(node, token);
			myPlayers.push_back(number - 1); // 1-based to 0-based.
		}
		else if (isNumberSelector(token, 'c', number))
		{
			if (!(mask & Mask::Columns) || number < 1 || number > myNumColumns)
				return reportInvalidSelector(node, token);
			myColumns.push_back(number - 1); // 1-based to 0-based.
		}
		else if (isNumberSelector(token, 'q', number))
		{
			if (!(mask & Mask::Quantization) || !convertQuantizationToIndex(number))
				return reportInvalidSelector(node, token);
			myQuantizations.push_back(number);
		}
		else if (token == "off")
		{
			if (!(mask & Mask::OffOn))
				return reportInvalidSelector(node, token);
			myOffOn.push_back(0);
		}
		else if (token == "on")
		{
			if (!(mask & Mask::OffOn))
				return reportInvalidSelector(node, token);
			myOffOn.push_back(1);
		}
		else if (token == "body")
		{
			if (!(mask & Mask::BodyTail))
				return reportInvalidSelector(node, token);
			myBodyTail.push_back(0);
		}
		else if (token == "tail")
		{
			if (!(mask & Mask::BodyTail))
				return reportInvalidSelector(node, token);
			myBodyTail.push_back(1);
		}
		else
		{
			reportInvalidSelector(node, token);
		}
	}
	return true;
}

void Selector::expandIndices(const vector<int>& selectedIndices, int range)
{
	size_t currentSize = indices.size();
	for (size_t i = 0; i < currentSize; ++i)
	{
		int currentIndex = indices.front();
		indices.pop_front();
		if (selectedIndices.empty())
		{
			for (int j = 0; j < range; ++j)
				indices.push_back(currentIndex * range + j);
		}
		else
		{
			for (int j : selectedIndices)
				indices.push_back(currentIndex * range + j);
		}
	}
}

// =====================================================================================================================
// Attribute parsing.

static bool parseFloat(const XmrNode* attr, float& out)
{
	if (!String::readFloat(attr->value, out))
		return reportInvalidValues(attr);

	return true;
}

static bool parseRect(const XmrNode* attr, Rect& out)
{
	int v[4];
	if (!String::readInts(attr->value, v))
		return reportInvalidValues(attr);

	out = Rect(v[0], v[1], v[0] + v[2], v[1] + v[3]);
	return true;
}

static bool parseTexture(const DirectoryPath& dir, const XmrNode* attr, Texture& out)
{
	auto path = FilePath(dir, attr->value);
	out = Texture(path, TextureFormat::RGBA, Color(0, 0, 0));
	out.setWrapping(true);
	return out.valid();
}

static bool parseTransform(const XmrNode* attr, TranformFunction& out)
{
	auto id = attr->value;
	if (id == "rotate-270")
	{
		out = rotate270;
		return true;
	}
	else if (id == "rotate-90")
	{
		out = rotate90;
		return true;
	}
	else if (id == "rotate-180")
	{
		out = rotate180;
		return true;
	}
	else if (id == "mirror-horizontally")
	{
		out = mirrorHorizontally;
		return true;
	}
	else if (id == "mirror-vertically")
	{
		out = mirrorVertically;
		return true;
	}
	return reportInvalidValues(attr);
}

// =====================================================================================================================
// Node parsing.

static void applyColumnSpacing(IntermediateNoteskin& skin, XmrNode* root)
{
	double spacing;
	if (!root->get("column-spacing", &spacing))
		return;

	int numCols = skin.gameMode->numCols;
	skin.notefieldWidth = float(spacing * numCols);

	double dx = 0.5 - numCols * 0.5;
	for (int i = 0; i < numCols; ++i)
		skin.columnX[i] = float((i + dx) * spacing);
}

static void parseColumn(IntermediateNoteskin& skin, const Selector& selector, const DirectoryPath& dir, XmrNode* node)
{
	float fvalue;
	Texture texture;
	for (auto attr : node->children())
	{
		auto id = attr->name;
		if (id == "texture-notes")
		{
			if (parseTexture(dir, attr, texture))
			{
				for (int i : selector.indices)
					skin.notesTex[i] = std::move(texture);
			}
		}
		else if (id == "texture-receptors")
		{
			if (parseTexture(dir, attr, texture))
			{
				for (int i : selector.indices)
					skin.receptorsTex[i] = std::move(texture);
			}
		}
		else if (id == "texture-holds")
		{
			if (parseTexture(dir, attr, texture))
			{
				for (int i : selector.indices)
					skin.holdsTex[i] = std::move(texture);
			}
		}
		else if (id == "texture-glow")
		{
			if (parseTexture(dir, attr, texture))
			{
				for (int i : selector.indices)
					skin.glowTex[i] = std::move(texture);
			}
		}
		else if (id == "x")
		{
			if (parseFloat(attr, fvalue))
			{
				for (int i : selector.indices)
					skin.columnX[i] = fvalue;
			}
		}
		else
		{
			reportUnknownAttribute(attr);
		}
	}
}

static void parseSprite(vector<IntermediateSprite>& sprites, const Selector& selector, XmrNode* node)
{
	Rect rect;
	float fvalue;
	TranformFunction transform;
	for (auto attr : node->children())
	{
		auto id = attr->name;
		if (id == "src")
		{
			if (parseRect(attr, rect))
			{
				for (int i : selector.indices)
					sprites[i].src = rect;
			}
		}
		else if (id == "scale")
		{
			if (parseFloat(attr, fvalue))
			{
				for (int i : selector.indices)
					sprites[i].scale = fvalue;
			}
		}
		else if (id == "transform")
		{
			if (parseTransform(attr, transform))
			{
				for (int i : selector.indices)
					transform(sprites[i].uvs);
			}
		}
		else
		{
			reportUnknownAttribute(attr);
		}
	}
}

static void parseNotefield(IntermediateNoteskin& out, XmrNode* node)
{
	float fvalue;
	for (auto attr : node->children())
	{
		auto id = attr->name;
		if (id == "width")
		{
			if (parseFloat(attr, fvalue))
				out.notefieldWidth = fvalue;
		}
		else
		{
			reportUnknownAttribute(attr);
		}
	}
}

static void parseNoteskin(IntermediateNoteskin& skin, const DirectoryPath& dir, XmrNode* root)
{
	Selector selector(skin.gameMode->numCols, skin.gameMode->numPlayers);

	applyColumnSpacing(skin, root);
	for (auto node : root->children())
	{
		auto tokens = String::split(node->name);
		auto id = tokens[0];
		if (id == "column")
		{
			selector.set(node, tokens, Selector::Mask::Columns);
			parseColumn(skin, selector, dir, node);
		}
		else if (id == "receptor")
		{
			selector.set(node, tokens, Selector::Mask::Columns | Selector::Mask::OffOn);
			parseSprite(skin.receptors, selector, node);
		}
		else if (id == "glow")
		{
			selector.set(node, tokens, Selector::Mask::Columns);
			parseSprite(skin.glow, selector, node);
		}
		else if (id == "note")
		{
			selector.set(node, tokens, Selector::Mask::Columns | Selector::Mask::Players | Selector::Mask::Quantization);
			parseSprite(skin.notes, selector, node);
		}
		else if (id == "hold")
		{
			selector.set(node, tokens, Selector::Mask::Columns | Selector::Mask::BodyTail);
			parseSprite(skin.holds, selector, node);
		}
		else if (id == "roll")
		{
			selector.set(node, tokens, Selector::Mask::Columns | Selector::Mask::BodyTail);
			parseSprite(skin.rolls, selector, node);
		}
		else if (id == "mine")
		{
			selector.set(node, tokens, Selector::Mask::Columns | Selector::Mask::Players);
			parseSprite(skin.mines, selector, node);
		}
		else if (id == "notefield")
		{
			parseNotefield(skin, node);
		}
	}
}

// =====================================================================================================================
// Noteskin conversion.

static SpriteEx finalizeSprite(const IntermediateSprite& sprite, const Texture& texture)
{
	if (!texture.valid())
		return SpriteEx();

	float texW = (float)texture.width();
	float texH = (float)texture.height();

	SpriteEx result;

	if (sprite.src.l == 0 && sprite.src.r == 0 && sprite.src.t == 0 && sprite.src.b == 0)
	{
		result = SpriteEx(texW * sprite.scale, texH * sprite.scale);
		result.setUvs(sprite.uvs);
	}
	else
	{
		result = SpriteEx(sprite.src.w() * sprite.scale, sprite.src.h() * sprite.scale);
		float ox = sprite.src.l / texW;
		float oy = sprite.src.t / texH;
		float dx = (sprite.src.r - sprite.src.l) / texW;
		float dy = (sprite.src.b - sprite.src.t) / texH;
		float uvs[] =
		{
			ox + sprite.uvs[0] * dx,
			oy + sprite.uvs[1] * dy,
			ox + sprite.uvs[2] * dx,
			oy + sprite.uvs[3] * dy,
			ox + sprite.uvs[4] * dx,
			oy + sprite.uvs[5] * dy,
			ox + sprite.uvs[6] * dx,
			oy + sprite.uvs[7] * dy,
		};
		result.setUvs(uvs);
	}

	return result;
}

static shared_ptr<Noteskin> finalizeNoteskin(IntermediateNoteskin& skin)
{
	const int numCols = skin.gameMode->numCols;
	const int numPlayers = skin.gameMode->numPlayers;
	const int numQuants = (int)Quantization::Count;

	auto result = make_shared<Noteskin>(skin.style, skin.gameMode);

	result->width = skin.notefieldWidth;

	for (int c = 0; c < numCols; ++c)
	{
		auto& column = result->columns[c];
		for (int p = 0; p < numPlayers; ++p)
		{
			auto& player = column.players[p];
			for (int q = 0; q < (size_t)Quantization::Count; ++q)
			{
				player.notes[q] = finalizeSprite(
					skin.notes[(c * numPlayers + p) * numQuants + q],
					skin.notesTex[c]);
			}
			player.mine = finalizeSprite(
				skin.mines[c * numPlayers + p],
				skin.notesTex[c]);
		}

		column.receptorOn = finalizeSprite(
			skin.receptors[c * 2 + 0],
			skin.receptorsTex[c]);
		column.receptorOff = finalizeSprite(
			skin.receptors[c * 2 + 1],
			skin.receptorsTex[c]);
		column.receptorGlow = finalizeSprite(
			skin.glow[c],
			skin.glowTex[c]);

		column.holdBody = finalizeSprite(
			skin.holds[c * 2 + 0],
			skin.holdsTex[c]);
		column.holdTail = finalizeSprite(
			skin.holds[c * 2 + 1],
			skin.holdsTex[c]);

		column.rollBody = finalizeSprite(
			skin.rolls[c * 2 + 0],
			skin.holdsTex[c]);
		column.rollTail = finalizeSprite(
			skin.rolls[c * 2 + 1],
			skin.holdsTex[c]);

		column.notesTex = std::move(skin.notesTex[c]);
		column.glowTex = std::move(skin.glowTex[c]);
		column.receptorsTex = std::move(skin.receptorsTex[c]);
		column.holdsTex = std::move(skin.holdsTex[c]);

		column.x = skin.columnX[c];
	}

	return result;
}

// =====================================================================================================================
// NoteskinLoader API.

static FilePath getNoteskinPath(const NoteskinStyle* style, const GameMode* mode)
{
	auto it = style->modes.find(mode);
	auto filename = it != style->modes.end() ? it->second : "fallback.txt";
	return FilePath(DirectoryPath("noteskins", style->name), filename);
}

shared_ptr<Noteskin> NoteskinLoader::load(const NoteskinStyle* style, const GameMode* mode)
{
	XmrDoc doc;
	auto path = getNoteskinPath(style, mode);
	if (doc.loadFile(path) != XmrResult::Success)
	{
		Log::error(format("Unable to load skin: {}", path.stem()));
		return nullptr;
	}
	IntermediateNoteskin skin(style, mode);
	parseNoteskin(skin, path.directory(), &doc);
	return finalizeNoteskin(skin);
}

} // namespace AV