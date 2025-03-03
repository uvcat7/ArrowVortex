#include <Precomp.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include <ftglyph.h>

#include <Core/Utils/Map.h>
#include <Core/Utils/Util.h>
#include <Core/System/File.h>
#include <Core/System/Debug.h>
#include <Core/System/Log.h>
#include <Core/Graphics/FontManager.h>

namespace AV {

using namespace std;
using namespace Util;

struct CustomGlyph : Glyph
{
	unique_ptr<Texture> customTexture;
};

static void CreatePlaceholderGlyphs(Glyph placeholderGlyphs[8])
{
	static constexpr int Padding = 2;

	static constexpr int w[8] = { 2, 4,  6,  8, 10, 14, 18, 22 };
	static constexpr int h[8] = { 4, 8, 12, 16, 20, 28, 36, 44 };
	static constexpr int b[8] = { 1, 1,  1,  2,  2,  3,  3,  4 };
	static constexpr int BufferSize = (22 + Padding) * (44 + Padding);

	uchar tmp[BufferSize];
	for (int i = 0; i < 8; ++i)
	{
		Glyph& glyph = placeholderGlyphs[i];

		memset(tmp, 0, BufferSize);
		int boxW = w[i] + Padding;
		int boxH = h[i] + Padding;
		for (int y = 0; y < h[i]; ++y)
		{
			for (int x = 0; x < w[i]; ++x)
			{
				tmp[(y + 1) * boxW + x + 1] = 255;
			}
		}
		for (int y = b[i]; y < b[i] * 4; ++y)
		{
			for (int x = b[i]; x < w[i] - b[i]; ++x)
			{
				tmp[(y + 1) * boxW + x + 1] = 0;
			}
		}

		float rw = 1.0f / (float)boxW;
		float rh = 1.0f / (float)boxH;

		glyph.hasTexture = 1;
		glyph.hasAlphaTex = 1;
		glyph.texture = new Texture(
			tmp, boxW, boxH, boxW, PixelFormat::Alpha, TextureFormat::Alpha);

		float* uvs = glyph.uvs;
		uvs[0] = uvs[4] = rw;
		uvs[1] = uvs[3] = rh;
		uvs[2] = uvs[6] = 1.0f - rw;
		uvs[5] = uvs[7] = 1.0f - rh;

		glyph.slot = Rect(0, 0, boxW, boxH);
		glyph.box = Rect(b[i], -h[i], b[i] + w[i], 0);
		glyph.advance = w[i] + b[i] / 2 + 1;
	}
}

FontManager::Entry::Entry(FontFace* face, FontHint hinting)
	: FontData(face, hinting)
{
}

class PrivateFontEntry : public FontManager::Entry
{
public:
	PrivateFontEntry(FontFace* face, FontHint hinting)
		: Entry(face, hinting)
		, referenceCount(1)
		, isCached(false)
	{
	}
	int referenceCount;
	bool isCached;
};

// =====================================================================================================================
// Font manager state.

namespace FontManager
{
	struct State
	{
		FT_Library ftLib;
		map<string, PrivateFontEntry*> fonts;
		map<string, CustomGlyph> customGlyphs;
		Glyph placeholderGlyphs[8];
	};
	static State* state = nullptr;
}
using FontManager::state;

// =====================================================================================================================
// FontManager

void FontManager::initialize()
{
	state = new State();

	FT_Init_FreeType(&state->ftLib);
	CreatePlaceholderGlyphs(state->placeholderGlyphs);
}

void FontManager::deinitialize()
{
	for (auto glyph : state->placeholderGlyphs)
		delete glyph.texture;

	for (auto font : state->fonts)
	{
		VortexAssertf(font.second->referenceCount == 0,
			"Lifetime of font is longer than lifetime of font manager.");
		if (font.second->referenceCount == 0) delete font.second;
	}
	FT_Done_FreeType(state->ftLib);

	Util::reset(state);
}

FontManager::Entry* FontManager::load(const FilePath& path, FontHint hinting)
{
	if (!state)
	{
		VortexAssertf(false, "Font manager must be created before a font is created.");
		return nullptr;
	}

	// Check if the font is already loaded.
	auto it = state->fonts.find(path.str);
	if (it != state->fonts.end())
	{
		it->second->referenceCount++;
		return it->second;
	}

	// If not, try to load the font.
	FT_Face face;
	int result = FT_New_Face((FT_Library)(state->ftLib), path.str.data(), 0, &face);
	if (result == FT_Err_Ok)
	{
		auto result = new PrivateFontEntry((FontFace*)face, hinting);
		state->fonts.emplace(path.str, result);
		return result;
	}
	else
	{
		Log::blockBegin("Could not load font file.");
		Log::error(format("file: %s", path.str));
		switch (result)
		{
		case FT_Err_Cannot_Open_Resource:
			Log::error("reason: file not found"); break;
		case FT_Err_Unknown_File_Format:
			Log::error("reason: unknown file format"); break;
		case FT_Err_Invalid_File_Format:
			Log::error("reason: invalid file format"); break;
		default:
			Log::error(format("ft-error: {}", result));
		};
		Log::blockEnd();
	}

	return nullptr;
}

void FontManager::cache(Entry* _entry)
{
	auto entry = (PrivateFontEntry*)_entry;
	entry->isCached = true;
}

void FontManager::addReference(Entry* _entry)
{
	auto entry = (PrivateFontEntry*)_entry;
	entry->referenceCount++;
}

void FontManager::release(Entry* _entry)
{
	auto entry = (PrivateFontEntry*)_entry;
	entry->referenceCount--;
	if (entry->referenceCount == 0 && !entry->isCached)
	{
		// Fonts can still exist after shutdown, don't assume state is valid.
		if (state)
		{
			Map::eraseVals(state->fonts, entry);
		}
		else
		{
			VortexAssertf(false, "Lifetime of font is longer than lifetime of font manager.");
		}
		delete entry;
	}
}

bool FontManager::addCustomGlyph(stringref name, unique_ptr<Texture>& texture, int dx, int dy, int advance)
{
	if (texture)
	{
		auto& glyph = state->customGlyphs[name];

		int width = texture->width();
		int height = texture->height();

		glyph.hasAlphaTex = (texture->format() == TextureFormat::Alpha);
		glyph.hasTexture = 1;
		glyph.slot = Rect(0, 0, width, height);
		glyph.box = Rect(dx, dy - height, dx + width, dy);
		glyph.advance = advance ? advance : (width + 1);
		glyph.texture = texture.get();
		texture.swap(glyph.customTexture);

		float* uvs = glyph.uvs;
		uvs[0] = uvs[1] = uvs[3] = uvs[4] = 0;
		uvs[2] = uvs[5] = uvs[6] = uvs[7] = 1;

		return true;
	}
	return false;
}

Glyph* FontManager::getCustomGlyph(stringref name)
{
	return Map::find(state->customGlyphs, name);
}

void FontManager::optimizeGlyphPages()
{
	for (auto font : state->fonts)
	{
		font.second->optimize();
	}
}

const Glyph& FontManager::getPlaceholderGlyph(int size)
{
	int index = clamp(size / 8, 0, 7);
	return state->placeholderGlyphs[index];
}

void FontManager::debugInfo()
{
	Log::blockBegin("Loaded fonts:");
	Log::info(format("amount: {}", state->fonts.size()));
	for (auto& font : state->fonts)
	{
		string path = font.first;
		if (path.empty()) path = "-- no path --";
		Log::lineBreak();
		Log::info(format("path   : {}", path));
		Log::info(format("refs   : {}", font.second->referenceCount));
		Log::info(format("cached : {}", font.second->isCached));
	}
	Log::blockEnd();
}

} // namespace AV
