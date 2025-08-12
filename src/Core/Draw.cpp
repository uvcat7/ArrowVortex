#include <Core/Draw.h>

#include <Core/Utils.h>
#include <Core/Renderer.h>

namespace Vortex {
namespace {

#define QPOS(l, t, r, b) \
    *vp = l;             \
    ++vp;                \
    *vp = t;             \
    ++vp;                \
    *vp = r;             \
    ++vp;                \
    *vp = t;             \
    ++vp;                \
    *vp = l;             \
    ++vp;                \
    *vp = b;             \
    ++vp;                \
    *vp = r;             \
    ++vp;                \
    *vp = b;             \
    ++vp;

#define QUVS(l, t, r, b) \
    *vt = l;             \
    ++vt;                \
    *vt = t;             \
    ++vt;                \
    *vt = r;             \
    ++vt;                \
    *vt = t;             \
    ++vt;                \
    *vt = l;             \
    ++vt;                \
    *vt = b;             \
    ++vt;                \
    *vt = r;             \
    ++vt;                \
    *vt = b;             \
    ++vt;

#define QCOL(c) \
    *vc = c;    \
    ++vc;       \
    *vc = c;    \
    ++vc;       \
    *vc = c;    \
    ++vc;       \
    *vc = c;    \
    ++vc;

#define QCOL4(a, b, c, d) \
    *vc = a;              \
    ++vc;                 \
    *vc = b;              \
    ++vc;                 \
    *vc = c;              \
    ++vc;                 \
    *vc = d;              \
    ++vc;

};  // anonymous namespace

// ================================================================================================
// Common colors.

const colorf Colorsf::white = {1, 1, 1, 1};
const colorf Colorsf::black = {0, 0, 0, 1};
const colorf Colorsf::blank = {0, 0, 0, 0};

const uint32_t Colors::white = RGBAtoColor32(255, 255, 255, 255);
const uint32_t Colors::black = RGBAtoColor32(0, 0, 0, 255);
const uint32_t Colors::blank = RGBAtoColor32(0, 0, 0, 0);

// ================================================================================================
// TileBar.

static void TB_setPosH(int* vp, int a, int b, int c, int d, int u, int v) {
    QPOS(a, u, b, v);
    QPOS(b, u, c, v);
    QPOS(c, u, d, v);
}

static void TB_setPosV(int* vp, int a, int b, int c, int d, int u, int v) {
    QPOS(u, a, v, b);
    QPOS(u, b, v, c);
    QPOS(u, c, v, d);
}

static void TB_setUvs(float* vt, float a, float b, float c, float d, float u,
                      float v) {
    QUVS(a, u, b, v);
    QUVS(b, u, c, v);
    QUVS(c, u, d, v);
}

static void TB_setVerts(const TileBar& bar, int* vp, float* vt, recti r,
                        areaf uvs, int flags) {
    if (flags & TileBar::VERTICAL) {
        swapValues(r.x, r.y);
        swapValues(r.w, r.h);
    }

    // Vertex positions: left-to-right [a,b,c,d] top-to-bottom [e,f].
    int a = r.x, d = r.x + r.w, b = a + bar.border, c = d - bar.border;
    int e = r.y, f = r.y + r.h;

    if (b > c) b = c = (a + d) / 2;

    // Fill in the vertex positions.
    if (flags & TileBar::VERTICAL) {
        TB_setPosV(vp, a, b, c, d, e, f);
    } else {
        TB_setPosH(vp, a, b, c, d, e, f);
    }

    // Texture coordinates: left-to-right [s,t,u,v] top-to-bottom [w,x].
    float rw = 1.0f / max(bar.texture.width(), 1);
    float s = uvs.l, t = uvs.l + (b - a) * rw, u = uvs.r - (d - c) * rw,
          v = uvs.r;
    float w = uvs.t, x = uvs.b;

    if (flags & TileBar::FLIP_H) {
        swapValues(s, v);
        swapValues(t, u);
    }
    if (flags & TileBar::FLIP_V) {
        swapValues(w, x);
    }

    // Fill in the texture coordinates.
    TB_setUvs(vt, s, t, u, v, w, x);
}

TileBar::TileBar() : uvs({0, 0, 1, 1}), border(0) {}

void TileBar::draw(recti rect, uint32_t color, int flags) const {
    int vp[24];
    float vt[24];
    TB_setVerts(*this, vp, vt, rect, uvs, flags);

    Renderer::setColor(color);
    Renderer::bindTexture(texture.handle());
    Renderer::bindShader(Renderer::SH_TEXTURE);
    Renderer::drawQuads(3, vp, vt);
}

void TileBar::draw(QuadBatchTC* out, recti rect, uint32_t color,
                   int flags) const {
    out->push(3);

    uint32_t *vc = out->col, *end = vc + 12;
    while (vc != end) {
        QCOL(color);
    }

    TB_setVerts(*this, out->pos, out->uvs, rect, uvs, flags);
}

// ================================================================================================
// TileRect.

static void TR_setPosH(int* vp, int a, int b, int c, int d, int u, int v, int w,
                       int x) {
    QPOS(a, u, b, v);
    QPOS(b, u, c, v);
    QPOS(c, u, d, v);
    QPOS(a, v, b, w);
    QPOS(b, v, c, w);
    QPOS(c, v, d, w);
    QPOS(a, w, b, x);
    QPOS(b, w, c, x);
    QPOS(c, w, d, x);
}

static void TR_setPosV(int* vp, int a, int b, int c, int d, int u, int v, int w,
                       int x) {
    QPOS(u, a, v, b);
    QPOS(u, b, v, c);
    QPOS(u, c, v, d);
    QPOS(v, a, w, b);
    QPOS(v, b, w, c);
    QPOS(v, c, w, d);
    QPOS(w, a, x, b);
    QPOS(w, b, x, c);
    QPOS(w, c, x, d);
}

static void TR_setUvs(float* vt, float a, float b, float c, float d, float u,
                      float v, float w, float x) {
    QUVS(a, u, b, v);
    QUVS(b, u, c, v);
    QUVS(c, u, d, v);
    QUVS(a, v, b, w);
    QUVS(b, v, c, w);
    QUVS(c, v, d, w);
    QUVS(a, w, b, x);
    QUVS(b, w, c, x);
    QUVS(c, w, d, x);
}

static void TR_setVerts(const TileRect& rect, int* vp, float* vt, recti r,
                        areaf uvs, int flags) {
    if (flags & TileRect::VERTICAL) {
        swapValues(r.x, r.y);
        swapValues(r.w, r.h);
    }

    // Vertex positions: left-to-right [a,b,c,d] top-to-bottom [e,f,g,h].
    int a = r.x, d = r.x + r.w, b = a + rect.border, c = d - rect.border;
    int e = r.y, h = r.y + r.h, f = e + rect.border, g = h - rect.border;

    if (b > c) b = c = (a + d) / 2;
    if (f > g) f = g = (e + h) / 2;

    // Fill in the vertex positions.
    if (flags & TileRect::VERTICAL) {
        TR_setPosV(vp, a, b, c, d, e, f, g, h);
    } else {
        TR_setPosH(vp, a, b, c, d, e, f, g, h);
    }

    // Texture coordinates: left-to-right [s,t,u,v] top-to-bottom [w,x,y,z].
    vec2i size = rect.texture.size();
    float rw = 1.f / max(size.x, 1);
    float rh = 1.f / max(size.y, 1);
    float s = uvs.l, t = uvs.l + (b - a) * rw, u = uvs.r - (d - c) * rw,
          v = uvs.r;
    float w = uvs.t, x = uvs.t + (f - e) * rh, y = uvs.b - (h - g) * rh,
          z = uvs.b;

    if (flags & Draw::FLIP_H) {
        swapValues(s, v);
        swapValues(t, u);
    }
    if (flags & Draw::FLIP_V) {
        swapValues(w, z);
        swapValues(x, y);
    }

    // Fill in the texture coordinates.
    TR_setUvs(vt, s, t, u, v, w, x, y, z);
}

TileRect::TileRect() : uvs({0, 0, 1, 1}), border(0) {}

void TileRect::draw(recti rect, uint32_t color, int flags) const {
    int vp[72];
    float vt[72];
    TR_setVerts(*this, vp, vt, rect, uvs, flags);

    Renderer::setColor(color);
    Renderer::bindTexture(texture.handle());
    Renderer::bindShader(Renderer::SH_TEXTURE);
    Renderer::drawQuads(9, vp, vt);
}

void TileRect::draw(QuadBatchTC* out, recti rect, uint32_t color,
                    int flags) const {
    out->push(9);

    uint32_t *vc = out->col, *end = vc + 36;
    while (vc != end) {
        QCOL(color);
    }

    TR_setVerts(*this, out->pos, out->uvs, rect, uvs, flags);
}

// ================================================================================================
// TileRect2.

static void TR2_setVerts(const TileRect2& rect, int* vp, float* vt, recti r,
                         int rounding, int flags) {
    if (flags & TileRect2::VERTICAL) {
        swapValues(r.x, r.y);
        swapValues(r.w, r.h);
    }

    // Vertex positions: left-to-right [a,b,c,d] top-to-bottom [e,f,g,h].
    int a = r.x, d = r.x + r.w, b = a + rect.border, c = d - rect.border;
    int e = r.y, h = r.y + r.h, f = e + rect.border, g = h - rect.border;

    if (b > c) b = c = (a + d) / 2;
    if (f > g) f = g = (e + h) / 2;

    // Fill in the vertex positions.
    if (flags & TileRect::VERTICAL) {
        TR_setPosV(vp, a, b, c, d, e, f, g, h);
    } else {
        TR_setPosH(vp, a, b, c, d, e, f, g, h);
    }

    // Texture coordinates: left-to-right [s,t,u,v] top-to-bottom [w,x,y,z].
    vec2i size = rect.texture.size();
    float rw = 1.f / max(size.x, 1);
    float rh = 1.f / max(size.y, 1);
    float s = 0, t = (b - a) * rw, u = 0.5f - (d - c) * rw, v = 0.5f;
    float w = 0, x = (f - e) * rh, y = 1.0f - (h - g) * rh, z = 1.0f;

    if (flags & TileRect2::FLIP_H) {
        swapValues(s, v);
        swapValues(t, u);
    }
    if (flags & TileRect2::FLIP_V) {
        swapValues(w, z);
        swapValues(x, y);
    }

    // Texture coordinate offsets.
    float tl = 0.5f * (rounding & TileRect2::TL);
    float tr = 0.25f * (rounding & TileRect2::TR);
    float bl = 0.125f * (rounding & TileRect2::BL);
    float br = 0.0625f * (rounding & TileRect2::BR);

    // Fill in the texture coordinates.
    QUVS(s + tl, w, t + tl, x);
    QUVS(t, w, u, x);
    QUVS(u + tr, w, v + tr, x);
    QUVS(s, x, t, y);
    QUVS(t, x, u, y);
    QUVS(u, x, v, y);
    QUVS(s + bl, y, t + bl, z);
    QUVS(t, y, u, z);
    QUVS(u + br, y, v + br, z);
}

void TileRect2::draw(recti rect, int rounding, uint32_t color,
                     int flags) const {
    int vp[72];
    float vt[72];
    TR2_setVerts(*this, vp, vt, rect, rounding, flags);

    Renderer::setColor(color);
    Renderer::bindTexture(texture.handle());
    Renderer::bindShader(Renderer::SH_TEXTURE);
    Renderer::drawQuads(9, vp, vt);
}

void TileRect2::draw(QuadBatchTC* out, recti rect, int rounding, uint32_t color,
                     int flags) const {
    out->push(9);

    uint32_t *vc = out->col, *end = vc + 36;
    while (vc != end) {
        QCOL(color);
    }

    TR2_setVerts(*this, out->pos, out->uvs, rect, rounding, flags);
}

// ================================================================================================
// Rectangle drawing.

void Draw::fill(recti r, uint32_t col) {
    // Vertex positions.
    int pos[8], *vp = pos;
    QPOS(r.x, r.y, r.x + r.w, r.y + r.h);

    // Render quad.
    Renderer::setColor(col);
    Renderer::unbindTexture();
    Renderer::bindShader(Renderer::SH_COLOR);
    Renderer::drawQuads(1, pos);
}

void Draw::fill(QuadBatchC* out, recti r, uint32_t color) {
    out->push();

    // Vertex positions.
    int* vp = out->pos;
    QPOS(r.x, r.y, r.x + r.w, r.y + r.h);

    // Vertex colors.
    uint32_t* vc = out->col;
    QCOL(color);
}

void Draw::fill(recti r, uint32_t tl, uint32_t tr, uint32_t bl, uint32_t br,
                bool hsv) {
    // Vertex positions.
    int pos[8], *vp = pos;
    QPOS(r.x, r.y, r.x + r.w, r.y + r.h);

    // Vertex colors.
    uint32_t col[4], *vc = col;
    QCOL4(tl, tr, bl, br);

    // Render quad.
    Renderer::bindShader(hsv ? Renderer::SH_COLOR_HSV : Renderer::SH_COLOR);
    Renderer::drawQuads(1, pos, col);
}

void Draw::fill(recti r, uint32_t col, TextureHandle tex, Texture::Format fmt) {
    // Vertex positions.
    int pos[8], *vp = pos;
    QPOS(r.x, r.y, r.x + r.w, r.y + r.h);

    // Texture coordinates.
    float uvs[8], *vt = uvs;
    QUVS(0, 0, 1, 1);

    // Render quad.
    Renderer::setColor(col);
    Renderer::bindShader((fmt == Texture::ALPHA) ? Renderer::SH_TEXTURE_ALPHA
                                                 : Renderer::SH_TEXTURE);
    Renderer::bindTexture(tex);
    Renderer::drawQuads(1, pos, uvs);
}

void Draw::fill(recti r, uint32_t col, TextureHandle tex, areaf uva,
                Texture::Format fmt) {
    // Vertex positions.
    int pos[8], *vp = pos;
    QPOS(r.x, r.y, r.x + r.w, r.y + r.h);

    // Texture coordinates.
    float uvs[8], *vt = uvs;
    QUVS(uva.l, uva.t, uva.r, uva.b);

    // Render quad.
    Renderer::setColor(col);
    Renderer::bindTexture(tex);
    Renderer::bindShader((fmt == Texture::ALPHA) ? Renderer::SH_TEXTURE_ALPHA
                                                 : Renderer::SH_TEXTURE);
    Renderer::drawQuads(1, pos, uvs);
}

void Draw::outline(recti r, uint32_t col) {
    static const uint32_t indices[24] = {0, 3, 1, 0, 2, 3, 4, 7, 5, 4, 6, 7,
                                         0, 4, 2, 0, 6, 4, 3, 7, 1, 3, 5, 7};

    // Vertex positions.
    int vp[16];
    vp[0] = vp[12] = r.x;
    vp[2] = vp[14] = r.x + r.w;
    vp[4] = vp[8] = r.x + 1;
    vp[6] = vp[10] = r.x + r.w - 1;
    vp[1] = vp[3] = r.y;
    vp[5] = vp[7] = r.y + 1;
    vp[9] = vp[11] = r.y + r.h - 1;
    vp[13] = vp[15] = r.y + r.h;

    // Render quads.
    Renderer::setColor(col);
    Renderer::unbindTexture();
    Renderer::bindShader(Renderer::SH_COLOR);
    Renderer::drawTris(8, indices, vp);
}

void Draw::roundedBox(recti r, uint32_t c) {
    Renderer::getRoundedBox().draw(r, c);
}

// ================================================================================================
// Sprite drawing.

void Draw::sprite(const Texture& tex, vec2i pos, int flags) {
    Draw::sprite(tex, pos, Colors::white, flags);
}

void Draw::sprite(const Texture& tex, vec2i pos, uint32_t col, int flags) {
    vec2i size = tex.size();
    int w = size.x, x = pos.x - w / 2;
    int h = size.y, y = pos.y - h / 2;

    // Vertex positions.
    int vp[8];
    vp[0] = vp[4] = x;
    vp[1] = vp[3] = y;
    vp[2] = vp[6] = x + w;
    vp[5] = vp[7] = y + h;

    // Texture coordinates.
    float vt[8];
    if (flags == 0) {
        vt[0] = vt[1] = vt[3] = vt[4] = 0;
        vt[2] = vt[5] = vt[6] = vt[7] = 1;
    } else {
        float u = 0, v = 0, s = 1, w = 1;
        if (flags & Draw::FLIP_H) swapValues(u, s);
        if (flags & Draw::FLIP_V) swapValues(v, w);
        if (flags & Draw::ROT_90) {
            vt[2] = vt[0] = u;
            vt[3] = vt[7] = v;
            vt[6] = vt[4] = s;
            vt[1] = vt[5] = w;
        } else {
            vt[0] = vt[4] = u;
            vt[1] = vt[3] = v;
            vt[2] = vt[6] = s;
            vt[5] = vt[7] = w;
        }
    }

    // Render quad.
    Renderer::setColor(col);
    Renderer::bindTexture(tex.handle());
    Renderer::bindShader((tex.format() == Texture::ALPHA)
                             ? Renderer::SH_TEXTURE_ALPHA
                             : Renderer::SH_TEXTURE);
    Renderer::drawQuads(1, vp, vt);
}

};  // namespace Vortex
