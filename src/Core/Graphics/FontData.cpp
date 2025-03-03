#include <Precomp.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include <ftglyph.h>

#include <Core/Utils/Flag.h>

#include <Core/System/System.h>
#include <Core/System/Debug.h>

#include <Core/Graphics/Texture.h>
#include <Core/Graphics/FontData.h>

namespace AV {

using namespace std;

// =====================================================================================================================
// Utility functions and constants.

struct GlyphAreaCompare
{
	bool operator()(const Glyph* first, const Glyph* second) const
	{
		return (first->slot.w() * first->slot.h() < second->slot.w() * second->slot.h());
	}
};

enum class GlyphTrait
{
	Whitespace = 1 << 0,
	Newline    = 1 << 1,
};

template <>
constexpr bool IsFlags<GlyphTrait> = true;

static constexpr size_t NumGlyphTraits = 33;

static const uchar GlyphTraits[NumGlyphTraits] =
{
	0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1 | 2, 1, 1, 1, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1
};

static constexpr int GlyphImagePadding = 1;
static constexpr int GlyphTexChannels = 1;

static const PixelFormat GlyphImageFormat = PixelFormat::Alpha;
static const TextureFormat GlyphTexFormat = TextureFormat::Alpha;

static uchar* CopyGlyphBitmap(FT_Bitmap bitmap, int padding)
{
	int srcW = bitmap.width;
	int srcH = bitmap.rows;
	int srcP = abs(bitmap.pitch);
	int dstW = srcW + padding * 2;
	int dstH = srcH + padding * 2;
	uchar* out = (uchar*)calloc(dstW * dstH, 1);
	for (int y = 0; y < srcH; ++y)
	{
		const uchar* src = bitmap.buffer + y * srcP;
		uchar* dst = out + ((y + padding) * dstW + padding) * GlyphTexChannels;
		switch (bitmap.pixel_mode)
		{
		case FT_PIXEL_MODE_BGRA:
			for (int x = 0; x < srcW; ++x, ++dst, src += 4) dst[0] = src[3];
			break;
		case FT_PIXEL_MODE_GRAY:
			memcpy(dst, src, srcW);
			break;
		case FT_PIXEL_MODE_MONO:
			for (int x = 0; x < srcW; ++x, ++dst)
				dst[0] = (src[x / 8] & (128 >> (x % 8))) ? 255 : 0;
			break;
		default:
			break;
		};
	};
	return out;
}

// =====================================================================================================================
// Glyph.

Glyph::Glyph()
	: hasTexture(0)
	, hasAlphaTex(0)
	, isWhitespace(0)
	, isNewline(0)
	, isInUse(0)
	, dummy(0)
	, advance(0)
	, uvs{0,0,1,0,0,1,1,1}
	, charIndex(0)
	, codepoint(0)
	, texture(nullptr)
	, lastUseTime(0)
	, next(nullptr)
{
}

// =====================================================================================================================
// Glyph page.

GlyphPage::~GlyphPage()
{
	for (auto glyph : myGlyphs)
	{
		free(glyph.second);
	}
	for (auto glyph : myEmptySlots)
	{
		free(glyph);
	}
}

static int GetTargetGlyphPageWidth(int fontSize)
{
	int width = 128;
	int targetWidth = min(1024, fontSize * 8 + 64);
	while (width < targetWidth) width *= 2;
	return width;
}

GlyphPage::GlyphPage(FontData* owner, int fontSize)
	: myOwner(owner)
	, myShelfX(0)
	, myShelfY(0)
	, myShelfH(0)
	, myShelfBegin(nullptr)
	, myShelfEnd(nullptr)
	, myFontSize(fontSize)
	, myTexture(GetTargetGlyphPageWidth(fontSize), 128, GlyphTexFormat)
{
}

void GlyphPage::increaseHeight(int minimumHeight)
{
	int oldHeight = myTexture.height(), newHeight = oldHeight;
	while (newHeight < minimumHeight) newHeight *= 2;

	myTexture.adjustHeight(newHeight);
	float ratio = float((double)oldHeight / (double)newHeight);
	for (auto glyph : myGlyphs)
	{
		glyph.second->uvs[1] *= ratio;
		glyph.second->uvs[3] *= ratio;
		glyph.second->uvs[5] *= ratio;
		glyph.second->uvs[7] *= ratio;
	}
}

Rect GlyphPage::getEmptySlot(int width, int height)
{
	// If the glyph does not fit on the current shelf, we close the shelf and open a new shelf.
	if (myShelfX + width > myTexture.width())
	{
		// Once the shelf is closed, we can expand the box height of all glyphs on the
		// shelf to the shelf height to make optimal use of the texture space later on.
		for (Glyph* glyph = myShelfBegin; glyph; glyph = glyph->next)
			glyph->slot.b = glyph->slot.t + myShelfH;

		// Move to the start position of the new shelf.
		myShelfY += myShelfH;
		myShelfX = myShelfH = 0;
		myShelfBegin = myShelfEnd = nullptr;
	}

	// If the current shelf exceeds the texture height, we increase the texture height.
	int targetHeight = myShelfY + height;
	if (targetHeight > myTexture.height())
	{
		if (targetHeight <= 1024)
		{
			increaseHeight(targetHeight);
		}
		else // Prevent cache textures larger than 1024 pixels. 
		{
			VortexAssertf(false, "Glyph page texture limit exceeded");
			return Rect(0, 0, 0, 0);
		}
	}

	// Return the current shelf position.
	myShelfX += width;
	myShelfH = max(myShelfH, height);
	return Rect(myShelfX - width, myShelfY, myShelfX, myShelfY + height);
}

Glyph* GlyphPage::addGlyph(GlyphSlot* slot)
{
	auto ftSlot = (FT_GlyphSlot)slot;

	Glyph* glyph = nullptr;

	// If the glyph bitmap has pixels, we need to find a place for it on the cache texture.
	const FT_Bitmap& bitmap = ftSlot->bitmap;
	int glyphW = (int)bitmap.width;
	int glyphH = (int)bitmap.rows;
	if (glyphW > 0 && glyphH > 0)
	{
		int paddedW = glyphW + GlyphImagePadding * 2;
		int paddedH = glyphH + GlyphImagePadding * 2;

		// First, check if we can reuse the spot of a previous glyph that is no longer in use.
		auto it = myEmptySlots.begin();
		auto end = myEmptySlots.end();
		while (it != end && ((*it)->slot.w() < paddedW || (*it)->slot.h() < paddedH)) ++it;
		if (it != end)
		{
			// We found an unused glyph spot that is large enough to reuse.
			glyph = *it;
			myEmptySlots.erase(it);
		}
		else // We didn't find a good spot to reuse, so we add the glyph to the shelf.
		{
			// Allocate a new glyph.
			glyph = new Glyph();
			glyph->hasAlphaTex = 1;

			// Reserve a spot on the glyph texture.
			glyph->slot = getEmptySlot(paddedW, paddedH);

			// Append the glyph to the current shelf.
			if (myShelfEnd)
			{
				myShelfEnd->next = glyph;
				myShelfEnd = glyph;
			}
			else
			{
				myShelfBegin = myShelfEnd = glyph;
			}
		}

		// Set glyph properties.
		glyph->isInUse = 1;
		glyph->texture = &myTexture;
		glyph->lastUseTime = (float)System::getElapsedTime();

		// Copy the glyph pixels to the cache texture.
		uchar* pixels = CopyGlyphBitmap(bitmap, GlyphImagePadding);
		myTexture.modify(pixels, glyph->slot.l, glyph->slot.t, paddedW, paddedH, paddedW, PixelFormat::Alpha);
		free(pixels);

		// Set the glyph uvs.
		float rTexW = 1.f / (float)myTexture.width();
		float rTexH = 1.f / (float)myTexture.height();

		float* uvs = glyph->uvs;
		uvs[0] = uvs[4] = float(glyph->slot.l + GlyphImagePadding) * rTexW;
		uvs[1] = uvs[3] = float(glyph->slot.t + GlyphImagePadding) * rTexH;
		uvs[2] = uvs[6] = float(glyph->slot.l + paddedW - GlyphImagePadding) * rTexW;
		uvs[5] = uvs[7] = float(glyph->slot.t + paddedH - GlyphImagePadding) * rTexH;

		glyph->hasTexture = 1;
	}
	else // The bitmap does not have pixels, so we can just allocate a glyph and return it.
	{
		glyph = new Glyph();
		glyph->isInUse = 1;
	}

	glyph->box.l = ftSlot->bitmap_left;
	glyph->box.t = -ftSlot->bitmap_top;
	glyph->box.r = glyph->box.l + glyphW;
	glyph->box.b = glyph->box.t + glyphH;

	return glyph;
}

Glyph* GlyphPage::getGlyph(int codepoint)
{
	Glyph* glyph = nullptr;

	// Check if the glyph is already cached.
	auto it = myGlyphs.find(codepoint);
	if (it != myGlyphs.end())
	{
		glyph = it->second;
		glyph->lastUseTime = (float)System::getElapsedTime();
		return glyph;
	}

	// Sanity check for invalid codepoint values.
	if (codepoint < 0) return nullptr;

	// Render the glyph as a bitmap using FreeType.
	FT_Error error = 0;
	FontData* font = myOwner;
	FT_Face face = (FT_Face)font->face();
	FT_Set_Pixel_Sizes(face, 0, myFontSize);
	uint charIndex = FT_Get_Char_Index(face, codepoint);
	if (charIndex) error = FT_Load_Glyph(face, charIndex, font->loadFlags());

	// Check if the glyph is supported by the font face and was rendered correctly.
	if (face->glyph->format == FT_GLYPH_FORMAT_BITMAP && charIndex && !error)
	{
		glyph = addGlyph((GlyphSlot*)face->glyph);
		glyph->advance = face->glyph->advance.x >> 6;
	}
	// Some fonts do not have whitespace glyphs, but we can approximate them if necessary.
	else if (codepoint < 33 && (GlyphTraits[codepoint] & uchar(GlyphTrait::Whitespace)))
	{
		glyph = new Glyph();
		glyph->advance = myFontSize / 2;
	}
	else // If its a non-whitespace glyph that is not supported, we are out of luck.
	{
		return nullptr;
	}

	// Glyph traits.
	if (codepoint < NumGlyphTraits)
	{
		glyph->isWhitespace = ((GlyphTraits[codepoint] & uchar(GlyphTrait::Whitespace)) != 0);
		glyph->isNewline = ((GlyphTraits[codepoint] & uchar(GlyphTrait::Newline)) != 0);
	}
	glyph->charIndex = charIndex;
	glyph->codepoint = codepoint;

	// Insert the glyph in the glyph map.
	auto rt = myGlyphs.insert(pair(codepoint, glyph));
	VortexAssert(rt.second);

	// Returns a pointer to the rendered glyph.
	return glyph;
}

void GlyphPage::optimize()
{
	double timeLimit = float(System::getElapsedTime() - 2.0);
	for (auto it = myGlyphs.begin(); it != myGlyphs.end(); )
	{
		auto glyph = it->second;
		if (glyph->lastUseTime < timeLimit)
		{
			if (glyph->slot.w() > 0 && glyph->slot.h() > 0)
			{
				glyph->isInUse = 0;
				myEmptySlots.push_back(glyph);
			}
			else
			{
				free(glyph);
			}
			it = myGlyphs.erase(it);
		}
		else
		{
			++it;
		}
	}
}

bool GlyphPage::isEmpty() const
{
	return myGlyphs.size() == 0;
}

// =====================================================================================================================
// FontData.

FontData::~FontData()
{
	FT_Done_Face((FT_Face)myFace);
	for (auto page : myPages)
	{
		delete page.second;
	}
}

FontData::FontData(FontFace* face, FontHint hinting)
	: myFace(face)
	, myActivePage(nullptr)
	, myActiveFontSize(0)
{
	FT_Select_Charmap((FT_Face)face, FT_ENCODING_UNICODE);
	myLoadFlags = FT_LOAD_RENDER;
	switch (hinting)
	{
	case FontHint::Normal:
		myLoadFlags |= FT_LOAD_TARGET_NORMAL;
		break;
	case FontHint::Auto:
		myLoadFlags |= FT_LOAD_TARGET_NORMAL | FT_LOAD_FORCE_AUTOHINT;
		break;
	case FontHint::None:
		myLoadFlags |= FT_LOAD_TARGET_NORMAL | FT_LOAD_NO_HINTING;
		break;
	case FontHint::Light:
		myLoadFlags |= FT_LOAD_TARGET_LIGHT;
		break;
	case FontHint::Mono:
		myLoadFlags |= FT_LOAD_TARGET_MONO;
		break;
	};
}

GlyphPage* FontData::getPage(int fontSize)
{
	if (myActiveFontSize == fontSize)
	{
		return myActivePage;
	}
	else
	{
		auto page = myPages[fontSize];
		if (page == nullptr)
		{
			page = new GlyphPage(this, fontSize);
			myPages[fontSize] = page;
		}
		myActiveFontSize = fontSize;
		myActivePage = page;
		return page;
	}
}

Glyph* FontData::getGlyph(int fontSize, int codepoint)
{
	return getPage(fontSize)->getGlyph(codepoint);
}

int FontData::getKerning(const Glyph* left, const Glyph* right) const
{
	if (left && right)
	{
		FT_Vector delta;
		FT_Get_Kerning((FT_Face)myFace, left->charIndex, right->charIndex, FT_KERNING_DEFAULT, &delta);
		return delta.x >> 6;
	}
	return 0;
}

void FontData::optimize()
{
	for (auto it = myPages.begin(); it != myPages.end();)
	{
		it->second->optimize();
		if (it->second->isEmpty())
		{
			if (myActivePage == it->second)
			{
				myActivePage = nullptr;
				myActiveFontSize = 0;
			}
			delete it->second;
			it = myPages.erase(it);
		}
		else
		{
			++it;
		}
	}
}

} // namespace AV
