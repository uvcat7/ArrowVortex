#include <Precomp.h>

#include <Core/Utils/Flag.h>

#include <Core/Graphics/Draw.h>
#include <Core/Graphics/Texture.h>

namespace AV {

using namespace std;

static const float SpriteUVs[8] =
{
	0, 0, 1, 0, 0, 1, 1, 1
};

#define SET_POS_H(l, t, r, b)\
	*vp = l; ++vp; *vp = t; ++vp; *vp = r; ++vp; *vp = t; ++vp;\
	*vp = l; ++vp; *vp = b; ++vp; *vp = r; ++vp; *vp = b; ++vp;

#define SET_POS_V(l, t, r, b)\
	*vp = l; ++vp; *vp = t; ++vp; *vp = l; ++vp; *vp = b; ++vp;\
	*vp = r; ++vp; *vp = t; ++vp; *vp = r; ++vp; *vp = b; ++vp;

#define SET_UVS(l, t, r, b)\
	*vt = l; ++vt; *vt = t; ++vt; *vt = r; ++vt; *vt = t; ++vt;\
	*vt = l; ++vt; *vt = b; ++vt; *vt = r; ++vt; *vt = b; ++vt;

#define SET_COL1(c)\
	*vc = c; ++vc; *vc = c; ++vc; *vc = c; ++vc; *vc = c; ++vc;

#define SET_COL4(a, b, c, d)\
	*vc = a; ++vc; *vc = b; ++vc; *vc = c; ++vc; *vc = d; ++vc;

// =====================================================================================================================
// TileBar :: helper functions

static void TileBarSetPosH(int* vp, int a, int b, int c, int d, int u, int v)
{
	SET_POS_H(a, u, b, v);
	SET_POS_H(b, u, c, v);
	SET_POS_H(c, u, d, v);
}

static void TileBarSetPosV(int* vp, int a, int b, int c, int d, int u, int v)
{
	SET_POS_V(u, a, v, b);
	SET_POS_V(u, b, v, c);
	SET_POS_V(u, c, v, d);
}

static void TileBarSetUvs(float* vt, float a, float b, float c, float d, float u, float v)
{
	SET_UVS(a, u, b, v);
	SET_UVS(b, u, c, v);
	SET_UVS(c, u, d, v);
}

static void TileBarSetVertexData(const TileBar& bar, int* vp, float* vt, Rect r, Rectf uvs, TileBar::Flags flags)
{
	auto& texture = bar.texture;

	if (flags & TileBar::Flags::Vertical)
	{
		swap(r.l, r.t);
		swap(r.r, r.b);
	}

	// Vertex positions: left-to-right [a,b,c,d] top-to-bottom [e,f].
	int a = r.l, d = r.r, b = a + bar.border, c = d - bar.border;
	int e = r.t, f = r.b;

	if (b > c) b = c = (a + d) / 2;

	// Fill in the vertex positions.
	if (flags & TileBar::Flags::Vertical)
	{
		TileBarSetPosV(vp, a, b, c, d, e, f);
	}
	else
	{
		TileBarSetPosH(vp, a, b, c, d, e, f);
	}

	// Texture coordinates: left-to-right [s,t,u,v] top-to-bottom [w,x].
	float rw = 1.0f / max(texture->width(), 1);
	float s = uvs.l, t = uvs.l + (b - a) * rw, u = uvs.r - (d - c) * rw, v = uvs.r;
	float w = uvs.t, x = uvs.b;

	if (flags & TileBar::Flags::FlipH) { swap(s, v); swap(t, u); }
	if (flags & TileBar::Flags::FlipV) { swap(w, x); }

	// Fill in the texture coordinates.
	TileBarSetUvs(vt, s, t, u, v, w, x);
}

// =====================================================================================================================
// TileRect :: helper functions

static void TileRectSetPosH(int* vp, int a, int b, int c, int d, int w, int x, int y, int z)
{
	SET_POS_H(a, w, b, x); SET_POS_H(b, w, c, x); SET_POS_H(c, w, d, x);
	SET_POS_H(a, x, b, y); SET_POS_H(b, x, c, y); SET_POS_H(c, x, d, y);
	SET_POS_H(a, y, b, z); SET_POS_H(b, y, c, z); SET_POS_H(c, y, d, z);
}

static void TileRectSetPosV(int* vp, int a, int b, int c, int d, int w, int x, int y, int z)
{
	SET_POS_V(w, a, x, b); SET_POS_V(w, b, x, c); SET_POS_V(w, c, x, d);
	SET_POS_V(x, a, y, b); SET_POS_V(x, b, y, c); SET_POS_V(x, c, y, d);
	SET_POS_V(y, a, z, b); SET_POS_V(y, b, z, c); SET_POS_V(y, c, z, d);
}

static void TileRectSetUvs(float* vt, float a, float b, float c, float d, float w, float x, float y, float z)
{
	SET_UVS(a, w, b, x); SET_UVS(b, w, c, x); SET_UVS(c, w, d, x);
	SET_UVS(a, x, b, y); SET_UVS(b, x, c, y); SET_UVS(c, x, d, y);
	SET_UVS(a, y, b, z); SET_UVS(b, y, c, z); SET_UVS(c, y, d, z);
}

static void TileRectSetVertexData(const TileRect& rect, int* vp, float* vt, Rect r, Rectf uvs, TileRect::Flags flags)
{
	auto& texture = rect.texture;

	if (flags & TileRect::Flags::Vertical)
	{
		swap(r.l, r.t);
		swap(r.r, r.b);
	}

	// Vertex positions: left-to-right [a,b,c,d] top-to-bottom [e,f,g,h].
	int a = r.l, d = r.r, b = a + rect.border, c = d - rect.border;
	int e = r.t, h = r.b, f = e + rect.border, g = h - rect.border;

	if (b > c) b = c = (a + d) / 2;
	if (f > g) f = g = (e + h) / 2;

	// Fill in the vertex positions.
	if (flags & TileRect::Flags::Vertical)
	{
		TileRectSetPosV(vp, a, b, c, d, e, f, g, h);
	}
	else
	{
		TileRectSetPosH(vp, a, b, c, d, e, f, g, h);
	}

	// Texture coordinates: left-to-right [s,t,u,v] top-to-bottom [w,x,y,z].
	float rw = 1.f / max(texture->width(), 1);
	float rh = 1.f / max(texture->height(), 1);
	float s = uvs.l, t = uvs.l + (b - a) * rw, u = uvs.r - (d - c) * rw, v = uvs.r;
	float w = uvs.t, x = uvs.t + (f - e) * rh, y = uvs.b - (h - g) * rh, z = uvs.b;

	if (flags & TileRect::Flags::FlipH) { swap(s, v); swap(t, u); }
	if (flags & TileRect::Flags::FlipV) { swap(w, z); swap(x, y); }

	// Fill in the texture coordinates.
	TileRectSetUvs(vt, s, t, u, v, w, x, y, z);
}

// =====================================================================================================================
// RoundedTileRect :: helper functions

static void RoundedTileRectSetVertexData(const TileRect2& rect, int* vp, float* vt, Rect r,
	TileRect2::Rounding rounding, TileRect2::Flags flags)
{
	auto texture = rect.texture.get();

	if (flags & TileRect2::Flags::Vertical)
	{
		swap(r.l, r.t);
		swap(r.r, r.b);
	}

	// Vertex positions: left-to-right [a,b,c,d] top-to-bottom [e,f,g,h].
	int a = r.l, d = r.r, b = a + rect.border, c = d - rect.border;
	int e = r.t, h = r.b, f = e + rect.border, g = h - rect.border;

	if (b > c) b = c = (a + d) / 2;
	if (f > g) f = g = (e + h) / 2;

	// Fill in the vertex positions.
	if (flags & TileRect2::Flags::Vertical)
	{
		TileRectSetPosV(vp, a, b, c, d, e, f, g, h);
	}
	else
	{
		TileRectSetPosH(vp, a, b, c, d, e, f, g, h);
	}

	// Texture coordinates: left-to-right [s,t,u,v] top-to-bottom [w,x,y,z].
	float rw = 1.f / max(texture->width(), 1);
	float rh = 1.f / max(texture->height(), 1);
	float s = 0, t = (b - a) * rw, u = 0.5f - (d - c) * rw, v = 0.5f;
	float w = 0, x = (f - e) * rh, y = 1.0f - (h - g) * rh, z = 1.0f;

	if (flags & TileRect2::Flags::FlipH) { swap(s, v); swap(t, u); }
	if (flags & TileRect2::Flags::FlipV) { swap(w, z); swap(x, y); }

	// Texture coordinate offsets.
	float tl = 0.5f    * (rounding & TileRect2::Rounding::TL);
	float tr = 0.25f   * (rounding & TileRect2::Rounding::TR);
	float bl = 0.125f  * (rounding & TileRect2::Rounding::BL);
	float br = 0.0625f * (rounding & TileRect2::Rounding::BR);

	// Fill in the texture coordinates.
	SET_UVS(s + tl, w, t + tl, x); SET_UVS(t, w, u, x); SET_UVS(u + tr, w, v + tr, x);
	SET_UVS(s, x, t, y);           SET_UVS(t, x, u, y); SET_UVS(u, x, v, y);
	SET_UVS(s + bl, y, t + bl, z); SET_UVS(t, y, u, z); SET_UVS(u + br, y, v + br, z);
}

// =====================================================================================================================
// TileBar.

TileBar::TileBar()
	: uvs(0, 0, 1, 1)
	, border(0)
{
}

void TileBar::draw(Rect rect, Color color, Flags flags) const
{
	int vp[24];
	float vt[24];
	TileBarSetVertexData(*this, vp, vt, rect, uvs, flags);

	// Renderer::bindTextureAndShader(texture);
	Renderer::setColor(color);
	Renderer::drawQuads(3, vp, vt);
}

void TileBar::drawBatched(Rect rect, Color color, Flags flags) const
{
	auto data = Renderer::pushColoredTexturedQuads(3);

	Color* vc = data.col;
	for (int i = 0; i < 12; ++i)
		*vc++ = color;

	TileBarSetVertexData(*this, data.pos, data.uvs, rect, uvs, flags);
}

// =====================================================================================================================
// TileRect.

TileRect::TileRect()
	: uvs(0, 0, 1, 1)
	, border(0)
{
}

void TileRect::draw(Rect rect, Color color, Flags flags) const
{
	int vp[72];
	float vt[72];
	TileRectSetVertexData(*this, vp, vt, rect, uvs, flags);

	// Renderer::bindTextureAndShader(texture);
	Renderer::setColor(color);
	Renderer::drawQuads(9, vp, vt);
}

void TileRect::drawBatched(Rect rect, Color color, Flags flags) const
{
	auto data = Renderer::pushColoredTexturedQuads(9);

	Color* vc = data.col;
	for (int i = 0; i < 36; ++i)
		*vc++ = color;

	TileRectSetVertexData(*this, data.pos, data.uvs, rect, uvs, flags);
}

// =====================================================================================================================
// TileRect2.

void TileRect2::draw(Rect rect, Rounding rounding, Color color, Flags flags) const
{
	int vp[72];
	float vt[72];
	RoundedTileRectSetVertexData(*this, vp, vt, rect, rounding, flags);

	// Renderer::bindTextureAndShader(texture);
	Renderer::setColor(color);
	Renderer::drawQuads(9, vp, vt);
}

void TileRect2::drawBatched(Rect rect, Rounding rounding, Color color, Flags flags) const
{
	auto data = Renderer::pushColoredTexturedQuads(9);

	Color* vc = data.col;
	for (int i = 0; i < 36; ++i)
		*vc++ = color;

	RoundedTileRectSetVertexData(*this, data.pos, data.uvs, rect, rounding, flags);
}

// =====================================================================================================================
// Sprite.

static const float spriteUVs[6][8] =
{
	{0, 0, 1, 0, 0, 1, 1, 1}, // NORMAL
	{1, 0, 0, 0, 1, 1, 0, 1}, // FLIP_H
	{0, 1, 1, 1, 0, 0, 1, 0}, // FLIP_V
	{0, 1, 0, 0, 1, 1, 1, 0}, // ROT_90
	{1, 1, 0, 1, 1, 0, 0, 0}, // ROT_180
	{1, 0, 1, 1, 0, 0, 0, 1}, // ROT_270
};

template<>
const char* Enum::toString<Sprite::Fill>(Sprite::Fill value)
{
	switch (value)
	{
	case Sprite::Fill::Center:
		return "center";
	case Sprite::Fill::Stretch:
		return "stretch";
	case Sprite::Fill::Letterbox:
		return "letterbox";
	case Sprite::Fill::Crop:
		return "crop";
	}
	return "unknown";
}

template<>
optional<Sprite::Fill> Enum::fromString<Sprite::Fill>(stringref str)
{
	if (str == "center")
		return Sprite::Fill::Center;
	if (str == "stretch")
		return Sprite::Fill::Stretch;
	if (str == "letterbox")
		return Sprite::Fill::Letterbox;
	if (str == "crop")
		return Sprite::Fill::Crop;
	return std::nullopt;
}

Sprite::Sprite()
{
}

Sprite::Sprite(unique_ptr<Texture>&& texture)
	: texture(texture.release())
{
}

Sprite::Sprite(const shared_ptr<Texture>& texture)
	: texture(texture)
{
}

void Sprite::draw(Vec2 pos, Color col, Orientation orientation) const
{
	auto source = texture.get();
	int width = source->width();
	int height = source->height();

	int l = pos.x - width / 2;
	int t = pos.y - height / 2;
	int r = l + width;
	int b = t + height;

	// Vertex positions.
	int ppos[8], *vp = ppos;
	SET_POS_H(l, t, r, b);

	// Drawing operation.
	// Renderer::bindTextureAndShader(texture);
	Renderer::setColor(col);
	Renderer::drawQuads(1, ppos, spriteUVs[(size_t)orientation]);
}

void Sprite::draw(Rect rect, Color col, Orientation orientation, Fill fill) const
{
	auto source = texture.get();
	int width = source->width();
	int height = source->height();

	switch (fill)
	{
	case Fill::Center:
		break;

	case Fill::Letterbox:
	case Fill::Crop:
		if (width != rect.w() || height != rect.h())
		{
			double sizeRatio = (double)width / (double)height;
			double rectRatio = (double)rect.w() / (double)rect.h();
			if ((sizeRatio > rectRatio) == (fill == Fill::Letterbox))
			{
				height *= rect.w();
				height /= max(width, 1);
				width = rect.w();
			}
			else
			{
				width *= rect.h();
				width /= max(height, 1);
				height = rect.h();
			}
		}
		break;

	case Fill::Stretch:
		width = rect.w();
		height = rect.h();
		break;
	}

	// Vertex positions.
	int pos[8], *vp = pos;
	rect = Rect(rect.center(), Size2(width, height));
	int l = rect.l;
	int t = rect.t;
	int r = rect.r;
	int b = rect.b;
	SET_POS_H(l, t, r, b);

	// Render quad.
	// Renderer::bindTextureAndShader(texture);
	Renderer::setColor(col);
	Renderer::drawQuads(1, pos, spriteUVs[(size_t)orientation]);
}

// =====================================================================================================================
// SpriteEx.

SpriteEx::SpriteEx() : SpriteEx(0, 0)
{
}

SpriteEx::SpriteEx(float w, float h)
{
	myDx = w * 0.5f;
	myDy = h * 0.5f;
	memcpy(myUvs, SpriteUVs, sizeof(float) * 8);
}

void SpriteEx::fromTileset(SpriteEx* spr, int count, int cols, Row rows, int sprW, int sprH)
{
	float du = 1.f / (float)cols, dv = 1.f / (float)rows;
	for (int i = 0; i != count; ++i)
	{
		float u = float(i % cols) * du, v = float(i / cols) * dv;
		float uvs[8] = {u, v, u + du, v, u, v + dv, u + du, v + dv};
		memcpy(spr[i].myUvs, uvs, sizeof(float) * 8);
		spr[i].myDx = (float)sprW * 0.5f;
		spr[i].myDy = (float)sprH * 0.5f;
	}
}

void SpriteEx::setUvs(Rectf uvs)
{
	myUvs[0] = uvs.l;
	myUvs[1] = uvs.t;
	myUvs[2] = uvs.r;
	myUvs[3] = uvs.t;
	myUvs[4] = uvs.l;
	myUvs[5] = uvs.b;
	myUvs[6] = uvs.r;
	myUvs[7] = uvs.b;
}

void SpriteEx::setUvs(const float uvs[8])
{
	memcpy(myUvs, uvs, sizeof(float) * 8);
}

void SpriteEx::drawBatched(float scale, float x, float y) const
{
	float dx = myDx * scale;
	float dy = myDy * scale;
	Renderer::pushQuad(x - dx, y - dy, x + dx, y + dy, myUvs);
}

void SpriteEx::drawBatched(float scale, float x, float ty, float by) const
{
	float dx = myDx * scale;
	Renderer::pushQuad(x - dx, ty, x + dx, by, myUvs);
}

void SpriteEx::drawBatched(float scale, float x, float y, uchar alpha) const
{
	drawBatched(scale, x, y, Color(255, 255, 255, alpha));
}

void SpriteEx::drawBatched(float scale, float x, float y, Color col) const
{
	float dx = myDx * scale;
	float dy = myDy * scale;
	Renderer::pushQuad(x - dx, y - dy, x + dx, y + dy, col, myUvs);
}

// =====================================================================================================================
// Rectangle drawing.

void Draw::fill(Rect r, Color col)
{
	// Vertex positions.
	int pos[8], *vp = pos;
	SET_POS_H(r.l, r.t, r.r, r.b);

	// Render quad.
	Renderer::bindShader(Renderer::Shader::Color);
	Renderer::setColor(col);
	Renderer::drawQuads(1, pos);
}

void Draw::fill(Rectf r, Color col)
{
	// Vertex positions.
	float pos[8], *vp = pos;
	SET_POS_H(r.l, r.t, r.r, r.b);

	// Render quad.
	Renderer::bindShader(Renderer::Shader::Color);
	Renderer::setColor(col);
	Renderer::drawQuads(1, pos);
}

void Draw::fill(Rect r, Color tl, Color tr, Color bl, Color br, bool hsv)
{
	// Vertex positions.
	int pos[8], *vp = pos;
	SET_POS_H(r.l, r.t, r.r, r.b);

	// Vertex colors.
	Color col[4], *vc = col;
	SET_COL4(tl, tr, bl, br);

	// Render quad.
	Renderer::bindShader(hsv ? Renderer::Shader::ColorHSV : Renderer::Shader::Color);
	Renderer::drawQuads(1, pos, col);
}

void Draw::fill(Rect r, Color col, const Texture& texture)
{
	// Vertex positions.
	int pos[8], *vp = pos;
	SET_POS_H(r.l, r.t, r.r, r.b);

	// Render quad.
	Renderer::bindTextureAndShader(texture);
	Renderer::setColor(col);
	Renderer::drawQuads(1, pos, SpriteUVs);
}

void Draw::fill(Rect r, Color col, const Texture& texture, Rectf uvsArea)
{
	// Vertex positions.
	int pos[8], *vp = pos;
	SET_POS_H(r.l, r.t, r.r, r.b);

	// Texture coordinates.
	float uvs[8], *vt = uvs;
	SET_UVS(uvsArea.l, uvsArea.t, uvsArea.r, uvsArea.b);

	// Render quad.
	Renderer::bindTextureAndShader(texture);
	Renderer::setColor(col);
	Renderer::drawQuads(1, pos, uvs);
}

void Draw::fill(Rect r, Color col, const Texture& texture, const float uvs[8])
{
	// Vertex positions.
	int pos[8], *vp = pos;
	SET_POS_H(r.l, r.t, r.r, r.b);

	// Render quad.
	Renderer::bindTextureAndShader(texture);
	Renderer::setColor(col);
	Renderer::drawQuads(1, pos, uvs);
}

void Draw::outline(Rect r, Color col)
{
	static const uint Indices[24] =
	{
		0, 3, 1, 0, 2, 3, 4, 7, 5, 4, 6, 7, 0, 4, 2, 0, 6, 4, 3, 7, 1, 3, 5, 7
	};

	// Vertex positions.
	int vp[16];
	vp[ 0] = vp[12] = r.l;
	vp[ 2] = vp[14] = r.r;
	vp[ 4] = vp[ 8] = r.l + 1;
	vp[ 6] = vp[10] = r.r - 1;
	vp[ 1] = vp[ 3] = r.t;
	vp[ 5] = vp[ 7] = r.t + 1;
	vp[ 9] = vp[11] = r.b - 1;
	vp[13] = vp[15] = r.b;

	// Render quads.
	Renderer::bindShader(Renderer::Shader::Color);
	Renderer::setColor(col);
	Renderer::unbindTexture();
	Renderer::drawTris(8, Indices, vp);
}

void Draw::roundedBox(Rect r, Color c)
{
	Renderer::getRoundedBox().draw(r, c);
}

} // namespace AV