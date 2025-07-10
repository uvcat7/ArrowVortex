#include <Core/FontData.h>

#include <Core/FontManager.h>
#include <Core/Texture.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

#include <System/OpenGL.h>

#include <algorithm>
#include <vector>

namespace Vortex {

static const int PADDING = 1;
static const int CHANNELS = 1;
static const Texture::Format FORMAT = Texture::ALPHA;

// Creates a padded grayscale copy of a glyph bitmap.
static uchar* CopyGlyphBitmap(int boxW, int boxH, FT_Bitmap bitmap)
{
	int srcW = bitmap.width, srcH = bitmap.rows, srcP = abs(bitmap.pitch);
	uchar* out = (uchar*)calloc(boxW * boxH, 1);
	for(int y = 0; y < srcH; ++y)
	{
		const uchar* src = bitmap.buffer + y * srcP;
		uchar* dst = out + ((y + PADDING) * boxW + PADDING) * CHANNELS;
		switch(bitmap.pixel_mode)
		{
		case FT_PIXEL_MODE_BGRA:
			for(int x = 0; x < srcW; ++x, ++dst, src += 4) dst[0] = src[3];
			break;
		case FT_PIXEL_MODE_GRAY:
			memcpy(dst, src, srcW);
			break;
		case FT_PIXEL_MODE_MONO:
			for(int x = 0; x < srcW; ++x, ++dst)
				dst[0] = (src[x / 8] & (128 >> (x % 8))) ? 255 : 0;
			break;
		default:
			break;
		};
	};
	return out;
}

// ================================================================================================
// Glyph cache functions

enum GlyphTraitBits { GTB_WHITESPACE = 1, GTB_NEWLINE = 2 };

// Glyph traits of charcode values up to 32, values above 32 always have zero.
static const uchar glyphTraits[33] =
{
	0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1 | 2, 1, 1, 1, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1
};

static GlyphCache* CreateCache(FT_Face face, int size)
{
	int texW = 128, texH = 128;
	while(texW < 1024 && texW < size * 8 + 64) texW *= 2;
	std::vector<uchar> pixels(texW * texH);

	auto* cache = new GlyphCache;
	cache->tex = TextureManager::load(texW, texH, FORMAT, false, pixels.data());
	cache->shelfX = 0;
	cache->shelfY = 0;
	cache->shelfH = 0;
	cache->shelfStart = nullptr;
	cache->shelfEnd = nullptr;

	return cache;
}

static void ReleaseCache(GlyphCache* cache)
{
	if(cache)
	{
		TextureManager::release(cache->tex);

		for(auto& g : cache->glyphs) free(g.second);
		cache->glyphs.clear();

		for(auto& g : cache->unusedGlyphs) free(g);
		cache->unusedGlyphs.clear();
	}
	delete cache;
}

static void UpdateCache(GlyphCache* cache, float dt)
{
	static const float maxUnusedCacheTime = 2.0f;
	for(auto it = cache->glyphs.begin(); it != cache->glyphs.end();)
	{
		auto* glyph = it->second;
		glyph->timeSinceLastUse += dt;
		if(glyph->timeSinceLastUse > maxUnusedCacheTime)
		{
			if(glyph->box.w * glyph->box.h > 0)
			{
				cache->unusedGlyphs.insert(glyph);
			}
			else
			{
				free(glyph);
			}
			it = cache->glyphs.erase(it);
		}
		else
		{
			++it;
		}
	}
}

static void IncreaseTextureHeight(GlyphCache* cache, int h)
{
	int oldHeight = cache->tex->h, newHeight = oldHeight;
	while(newHeight < h) newHeight *= 2;

	cache->tex->increaseHeight(newHeight);
	float ratio = (float)((double)oldHeight / (double)newHeight);
	for(auto& g : cache->glyphs)
	{
		g.second->uvs.t *= ratio;
		g.second->uvs.b *= ratio;
	}
}

static recti ReserveEmptySpot(GlyphCache* cache, int bitmapW, int bitmapH)
{
	// If the glyph does not fit on the current shelf, we close the shelf and open a new shelf.
	if(cache->shelfX + bitmapW > cache->tex->w)
	{
		// Once the shelf is closed, we can expand the box height of all glyphs on the
		// shelf to the shelf height to make optimal use of the texture space later on.
		for(Glyph* g = cache->shelfStart; g; g = g->next) g->box.h = cache->shelfH;

		// We need to re-sort the unused glyphs because their area might have changed.
		std::multiset<Glyph*, GlyphAreaCompare> tmp;
		tmp.swap(cache->unusedGlyphs);
		for(auto& g : tmp) cache->unusedGlyphs.insert(g);

		// Move to the start position of the new shelf.
		cache->shelfY += cache->shelfH;
		cache->shelfX = cache->shelfH = 0;
		cache->shelfStart = cache->shelfEnd = nullptr;
	}

	// If the current shelf exceeds the texture height, we increase the texture height.
	int targetHeight = cache->shelfY + bitmapH;
	if(targetHeight > cache->tex->h)
	{
		if(targetHeight <= 1024)
		{
			IncreaseTextureHeight(cache, targetHeight);
		}
		else // Prevent cache textures larger than 1024 pixels. 
		{
			return{ 0, 0, 0, 0 };
		}
	}

	// Return the current shelf position.
	cache->shelfX += bitmapW;
	return{ cache->shelfX - bitmapW, cache->shelfY, bitmapW, bitmapH };
}

static Glyph* PutGlyphInCache(GlyphCache* cache, FT_GlyphSlot slot)
{
	Glyph* glyph = nullptr;

	// If the glyph bitmap has pixels, we need to find a place for it on the cache texture.
	const FT_Bitmap& bitmap = slot->bitmap;
	int glyphW = (int)bitmap.width;
	int glyphH = (int)bitmap.rows;
	if(glyphW > 0 && glyphH > 0)
	{
		int bitmapW = (int)bitmap.width + PADDING * 2;
		int bitmapH = (int)bitmap.rows + PADDING * 2;

		// First, check if we can reuse the spot of a previous glyph that is no longer in use.
		Glyph lb;
		lb.box = { 0, 0, bitmapW, bitmapH };
		auto it = cache->unusedGlyphs.lower_bound(&lb);
		while(it != cache->unusedGlyphs.end() && ((*it)->box.w < bitmapW || (*it)->box.h < bitmapH)) ++it;
		if(it != cache->unusedGlyphs.end())
		{
			// We found an unused glyph spot that is large enough to reuse.
			glyph = *it;
			glyph->tex = cache->tex;
			glyph->timeSinceLastUse = 0;
			cache->unusedGlyphs.erase(it);
		}
		else // We didn't find a good spot to reuse, so we add the glyph to the shelf.
		{
			// Allocate a new glyph.
			glyph = (Glyph*)calloc(1, sizeof(Glyph));
			glyph->hasAlphaTex = 1;

			// Reserve a spot on the glyph texture.
			glyph->tex = cache->tex;
			glyph->box = ReserveEmptySpot(cache, bitmapW, bitmapH);

			// Insert the glyph into the current shelf's list of glyphs.
			if(cache->shelfEnd)
			{
				cache->shelfEnd->next = glyph;
				cache->shelfEnd = glyph;
			}
			else
			{
				cache->shelfStart = cache->shelfEnd = glyph;
			}
		}

		// Copy the glyph pixels to the cache texture.
		uchar* pixels = CopyGlyphBitmap(bitmapW, bitmapH, bitmap);
		cache->tex->modify(glyph->box.x, glyph->box.y, bitmapW, bitmapH, pixels);
		cache->shelfH = std::max(cache->shelfH, bitmapH);
		free(pixels);

		// Set the glyph uvs.
		float rTexW = 1.f / (float)cache->tex->w;
		float rTexH = 1.f / (float)cache->tex->h;
		glyph->uvs =
		{
			(float)(glyph->box.x + PADDING) * rTexW,
			(float)(glyph->box.y + PADDING) * rTexH,
			(float)(glyph->box.x + PADDING + glyphW) * rTexW,
			(float)(glyph->box.y + PADDING + glyphH) * rTexH,
		};
		glyph->hasPixels = 1;
	}
	else // The bitmap does not have pixels, so we can just allocate a glyph and return it.
	{
		glyph = (Glyph*)calloc(1, sizeof(Glyph));
	}

	glyph->ofs.l = slot->bitmap_left;
	glyph->ofs.t = -slot->bitmap_top;
	glyph->ofs.r = glyph->ofs.l + glyphW;
	glyph->ofs.b = glyph->ofs.t + glyphH;

	return glyph;
}

Glyph* RenderGlyph(FontData* font, GlyphCache* cache, int size, int charcode)
{
	Glyph* glyph = nullptr;
	FT_Face face = (FT_Face)font->ftface;

	// Sanity check for invalid codepoint values.
	if(charcode < 0) return nullptr;

	// Render the glyph as a bitmap using FreeType.
	FT_Error error = 0;
	uint index = FT_Get_Char_Index(face, charcode);
	if(index) error = FT_Load_Glyph(face, index, font->loadflags);

	// Check if the glyph is supported by the font face and was rendered correctly.
	if(face->glyph->format == FT_GLYPH_FORMAT_BITMAP && index && !error)
	{
		glyph = PutGlyphInCache(cache, face->glyph);
		glyph->advance = face->glyph->advance.x >> 6;
	}
	// Some fonts do not have whitespace glyphs, but we can approximate them if necessary.
	else if(charcode < 33 && (glyphTraits[charcode] & GTB_WHITESPACE))
	{
		glyph = (Glyph*)calloc(1, sizeof(Glyph));
		glyph->advance = size / 2;
	}
	else // If its a non-whitespace glyph that is not supported, we are out of luck.
	{
		return nullptr;
	}

	// Glyph properties.
	if(charcode < 33)
	{
		if(glyphTraits[charcode] & GTB_WHITESPACE)
			glyph->isWhitespace = 1;
		if(glyphTraits[charcode] & GTB_NEWLINE)
			glyph->isNewline = 1;
	}
	glyph->index = index;
	glyph->font = font;
	glyph->charcode = charcode;

	// Insert the glyph in the glyph map.
	cache->glyphs.insert(std::make_pair(charcode, glyph));

	// Returns a pointer to the rendered glyph.
	return glyph;
}

// ================================================================================================
// FontData functions

FontData::FontData(void* inFtface, const char* inPath, Text::Hinting inHinting)
{
	FT_Select_Charmap((FT_Face)inFtface, FT_ENCODING_UNICODE);

	currentCache = nullptr;
	currentSize = 0;
	ftface = inFtface;
	path = inPath;
	next = nullptr;
	refs = 1;
	cached = false;

	switch(inHinting)
	{
	case Text::HINT_NORMAL:
		loadflags = FT_LOAD_TARGET_NORMAL;
		break;
	case Text::HINT_AUTO:
		loadflags = FT_LOAD_TARGET_NORMAL | FT_LOAD_FORCE_AUTOHINT;
		break;
	case Text::HINT_NONE:
		loadflags = FT_LOAD_TARGET_NORMAL | FT_LOAD_NO_HINTING;
		break;
	case Text::HINT_LIGHT:
		loadflags = FT_LOAD_TARGET_LIGHT;
		break;
	case Text::HINT_MONO:
		loadflags = FT_LOAD_TARGET_MONO;
		break;
	};
	loadflags = loadflags | FT_LOAD_RENDER;
}

FontData::~FontData()
{
	clear();
}

void FontData::clear()
{
	if(ftface)
	{
		FT_Done_Face((FT_Face)ftface);
	}

	for(auto& cache : caches)
	{
		ReleaseCache(cache.second);
	}

	caches.clear();
	currentCache = nullptr;
	currentSize = 0;
	ftface = nullptr;
	path.clear();
	next = nullptr;
	loadflags = 0;
	cached = false;
}

void FontData::update(float dt)
{
	for(auto it = caches.begin(); it != caches.end();)
	{
		GlyphCache* cache = it->second;
		UpdateCache(cache, dt);
		if(cache->glyphs.empty())
		{
			ReleaseCache(cache);
			if(currentCache == cache)
			{
				currentCache = nullptr;
				currentSize = 0;
			}
			it = caches.erase(it);
		}
		else ++it;
	}
}

void FontData::setSize(FontSize size)
{
	if(currentSize != size)
	{
		auto it = caches.find(size);
		if(it != caches.end())
		{
			currentCache = it->second;
		}
		else
		{
			GlyphCache* cache = CreateCache((FT_Face)ftface, size);
			caches[size] = currentCache = cache;
		}
		FT_Set_Pixel_Sizes((FT_Face)ftface, 0, size);
		currentSize = size;
	}
}

const Glyph* FontData::getGlyph(FontSize size, Codepoint charcode)
{
	setSize(size);

	// Try to find the glyph in the font cache.
	GlyphCache* cache = currentCache;
	auto it = cache->glyphs.find(charcode);
	if(it != cache->glyphs.end())
	{
		it->second->timeSinceLastUse = 0;
		return it->second;
	}	

	// If the glyph was not found, render it and add it to the font cache.
	return RenderGlyph(this, cache, size, charcode);
}

bool FontData::hasKerning() const
{
	return (FT_HAS_KERNING(((FT_Face)ftface)) != 0);
}

int FontData::getKerning(const Glyph* left, const Glyph* right) const
{
	auto face = (FT_Face)ftface;
	FT_Vector delta;
	FT_Get_Kerning(face, left->index, right->index, FT_KERNING_DEFAULT, &delta);
	return delta.x >> 6;
}

TextureHandle FontData::getActiveTexture(int size, vec2i& outTexSize)
{
	auto it = caches.find(size);
	if(it != caches.end())
	{
		auto tex = it->second->tex;
		outTexSize.x = tex->w;
		outTexSize.y = tex->h;
		return tex->handle;
	}
	return 0;
}

// ================================================================================================
// Public Font class.

typedef FontManager FontMan;

#define FONTDATA ((FontData*)data_)

Font::~Font()
{
	if(data_) FontMan::release(FONTDATA);
}

Font::Font()
	: data_(nullptr)
{
}

Font::Font(const char* path, Text::Hinting hint)
	: data_(nullptr)
{
	data_ = FontMan::load(path, hint);
}

Font::Font(Font&& font)
	: data_(font.data_)
{
	font.data_ = nullptr;
}

Font::Font(const Font& font)
	: data_(font.data_)
{
	if(data_) FONTDATA->refs++;
}

Font& Font::operator = (Font&& font)
{
	if(data_) FontMan::release(FONTDATA);
	data_ = font.data_;
	font.data_ = nullptr;
	return *this;
}

Font& Font::operator = (const Font& font)
{
	if(data_) FontMan::release(FONTDATA);
	data_ = font.data_;
	if(data_) FONTDATA->refs++;
	return *this;
}

TextureHandle Font::texture(int size, vec2i& outTexSize)
{
	return data_ ? FONTDATA->getActiveTexture(size, outTexSize) : 0;
}

}; // namespace Vortex
