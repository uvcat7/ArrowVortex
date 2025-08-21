#include <Core/GuiDraw.h>

#include <Core/GuiContext.h>
#include <Core/Canvas.h>
#include <Core/Utils.h>
#include <Core/Renderer.h>

namespace Vortex {

struct GuiDrawGraphics {
    GuiDraw::Dialog dialog;
    GuiDraw::Button button;
    GuiDraw::Scrollbar scrollbar;
    GuiDraw::TextBox textbox;
    GuiDraw::Icons icons;
    GuiDraw::Misc misc;
};

GuiDrawGraphics* GD = nullptr;

// ================================================================================================
// Canvas drawing functions.

struct CanvasHelper {
    CanvasHelper(int w, int h, float r, float l)
        : canvas(w * 2, h, l), myW(w), myH(h), myR(r) {
        canvas.setBlendMode(Canvas::BM_NONE);
    }
    void clear(int x, int y, float r, float l) {
        canvas.clear(l);
        myX = x, myY = y, myR = r;
    }
    void layer(float l, float a = 1.0f) {
        canvas.setColor({l, l, l, a});
        canvas.box(myX, myY, myW - myX, myH - myY);
        canvas.box(myX + myW, myY, myW * 2 - myX, myH - myY, myR);
        myR -= 1.0f;
        ++myX, ++myY;
    }
    void render(TileRect2& tile, int border) {
        tile.texture = canvas.createTexture();
        tile.border = border;
    }
    Canvas canvas;
    int myX = 0, myY = 0, myW, myH;
    float myR;
};

static void CreateDialogTextures() {
    CanvasHelper c(16, 16, 4.5f, 0.05f);
    c.layer(0.05f);
    c.layer(0.18f);
    c.layer(0.15f);
    c.render(GD->dialog.titlebar, 5);

    c.clear(0, 0, 4.5f, 0.05f);
    c.layer(0.05f);
    c.layer(0.35f);
    c.layer(0.2f);
    c.render(GD->dialog.frame, 5);

    Canvas c2(16, 16, 0.05f);
    float lum[] = {0.05f, 0.35f, 0.2f};
    for (int i = 0; i < 3; ++i) {
        float s = static_cast<float>(i) + 0.25f;
        float x[] = {8 + s, 18, 18, 8 + s, 8 + s, s, 8 + s};
        float y[] = {-2, -2, 18, 18, 16, 8, 0};
        c2.setColor(lum[i]);
        c2.polygon(x, y, 7);
    }
    GD->dialog.vshape = c2.createTexture();
}

static void CreateButtonTextures() {
    CanvasHelper c(16, 16, 4.5f, 0.1f);
    c.layer(0.1f);
    c.layer(0.32f);
    c.layer(0.28f);
    c.render(GD->button.base, 4);

    c.clear(1, 1, 3.5f, 0.2f);
    c.layer(1, 0.2f);
    c.layer(1, 0.025f);
    c.render(GD->button.hover, 4);

    c.clear(1, 1, 3.5f, 0.2f);
    c.layer(1, 0.3f);
    c.layer(1, 0.0f);
    c.render(GD->button.pressed, 4);
}

static void CreateScrollbarTextures() {
    CanvasHelper c(14, 16, 7, 0.25f);
    c.layer(0.25f);
    c.layer(0.08f);
    c.layer(0.12f);
    c.render(GD->scrollbar.bar, 7);

    c.clear(1, 1, 7, 0.1f);
    c.layer(0.1f);
    c.layer(0.32f);
    c.layer(0.28f);
    c.render(GD->scrollbar.base, 7);

    c.clear(2, 2, 6, 0.2f);
    c.layer(1, 0.2f);
    c.layer(1, 0.025f);
    c.render(GD->scrollbar.hover, 7);

    c.clear(2, 2, 6, 0.2f);
    c.layer(1, 0.3f);
    c.layer(1, 0.0f);
    c.render(GD->scrollbar.pressed, 7);
}

static void CreateTextBoxTextures() {
    CanvasHelper c(16, 16, 4.5f, 0.25f);
    c.layer(0.25f);
    c.layer(0.1f);
    c.layer(0.13f);
    c.render(GD->textbox.base, 4);

    c.clear(0, 0, 4.5f, 0.25f);
    c.layer(0.3f);
    c.layer(0.1f);
    c.layer(0.13f);
    c.render(GD->textbox.hover, 4);

    c.clear(0, 0, 4.5f, 0.25f);
    c.layer(0.4f);
    c.layer(0.1f);
    c.layer(0.13f);
    c.render(GD->textbox.active, 4);
}

static void CreateIcons() {
    Canvas c(8, 8, 0.2f);

    // Grab : three horizontal lines.
    c.setColor(0.2f);
    c.box(1, 2, 2, 6);
    c.box(4, 2, 5, 6);
    c.box(7, 2, 8, 6);
    c.setColor(0.6f);
    c.box(0, 2, 1, 6);
    c.box(3, 2, 4, 6);
    c.box(6, 2, 7, 6);
    GD->icons.grab = c.createTexture();

    // Icons.
    c.setColor(1.0f);

    // Pin : diagonal thumbtack.
    c.clear(1.0f);
    float pinneedle[6] = {0, 2, 4, 8, 4, 6};
    float pinhandle[12] = {2, 4, 6, 8, 6, 6, 2, 2, 0, 2, 4, 6};
    c.polygon(pinneedle, pinneedle + 3, 3);
    c.polygon(pinhandle, pinhandle + 6, 6);
    GD->icons.pin = c.createTexture();

    // Unpin : vertical thumbtack.
    c.clear(1.0f);
    float unpinneedle[8] = {
        1, 2, 4, 3, 5, 4, 6, 7,
    };
    c.polygon(unpinneedle, unpinneedle + 4, 4);
    c.polygon(pinhandle, pinhandle + 6, 6);
    GD->icons.unpin = c.createTexture();

    // Cross : two diagonal lines.
    c.clear(1.0f);
    c.line(1, 1, 7, 7, 2);
    c.line(1, 7, 7, 1, 2);
    GD->icons.cross = c.createTexture();

    // Minus : horizontal line.
    c.clear(1.0f);
    c.line(1, 4, 7, 4, 2);
    GD->icons.minus = c.createTexture();

    // Plus : horizontal and vertical line.
    c.clear(1.0f);
    c.line(1, 4, 7, 4, 2);
    c.line(4, 1, 4, 7, 2);
    GD->icons.plus = c.createTexture();

    // Arrow : right pointing triangle.
    c.clear(1.0f);
    float arrow[6] = {2, 6, 2, 1, 4, 7};
    c.polygon(arrow, arrow + 3, 3);
    GD->icons.arrow = c.createTexture();

    // Check : checkmark.
    c = Canvas(10, 10, 1.f);
    float check[12] = {7, 10, 5, 0, 2, 4, 0, 1, 10, 5, 3, 6};
    c.polygon(check, check + 6, 6);
    GD->icons.check = c.createTexture();
}

static void CreateOtherGraphics() {
    GD->misc.colDisabled = Color32(166);

    // Selection box image.
    {
        Canvas c(8, 8);
        c.setColor({0.5f, 0.5f, 0.6f, 1});
        c.box(0, 0, 8, 8, 1.5f);
        c.setColor({0.3f, 0.3f, 0.4f, 1});
        c.box(1, 1, 7, 7, 0.5f);
        GD->misc.imgSelect.texture = c.createTexture();
        GD->misc.imgSelect.border = 4;
    }

    // Checkerboard.
    {
        Canvas c(16, 16, 0.4f);
        c.setColor({0.4f, 0.4f, 0.4f, 1});
        c.box(0, 0, 16, 16);
        c.setColor({0.6f, 0.6f, 0.6f, 1});
        c.box(0, 0, 8, 8);
        c.box(8, 8, 16, 16);
        GD->misc.checkerboard = c.createTexture();
        GD->misc.checkerboard.setWrapping(true);
    }
}

void GuiDraw::create() {
    GD = new GuiDrawGraphics;

    CreateDialogTextures();
    CreateButtonTextures();
    CreateScrollbarTextures();
    CreateTextBoxTextures();
    CreateIcons();

    CreateOtherGraphics();
}

void GuiDraw::destroy() {
    delete GD;
    GD = nullptr;
}

GuiDraw::Dialog& GuiDraw::getDialog() { return GD->dialog; }

GuiDraw::Button& GuiDraw::getButton() { return GD->button; }

GuiDraw::Scrollbar& GuiDraw::getScrollbar() { return GD->scrollbar; }

GuiDraw::TextBox& GuiDraw::getTextBox() { return GD->textbox; }

GuiDraw::Icons& GuiDraw::getIcons() { return GD->icons; }

GuiDraw::Misc& GuiDraw::getMisc() { return GD->misc; }

template <typename T>
T* Set8(T* p, T l, T t, T r, T b) {
    *p = l;
    ++p;
    *p = t;
    ++p;
    *p = r;
    ++p;
    *p = t;
    ++p;
    *p = l;
    ++p;
    *p = b;
    ++p;
    *p = r;
    ++p;
    *p = b;
    ++p;
    return p;
}

template <typename T>
T* Set8(T* p, T l, T t, T r, T b, T dx) {
    l += dx, r += dx;
    *p = l;
    ++p;
    *p = t;
    ++p;
    *p = r;
    ++p;
    *p = t;
    ++p;
    *p = l;
    ++p;
    *p = b;
    ++p;
    *p = r;
    ++p;
    *p = b;
    ++p;
    return p;
}

void GuiDraw::button(TileRect* tr, const recti& r, bool hover, bool focus) {
    if (!focus && !hover) {
        tr[0].draw(r);
    } else {
        tr[1].draw(r, Color32(255, focus ? 230 : 255));
    }
}

void GuiDraw::checkerboard(recti r, uint32_t color) {
    // Vertex positions.
    int vp[8];
    vp[0] = vp[4] = r.x;
    vp[1] = vp[3] = r.y;
    vp[2] = vp[6] = r.x + r.w;
    vp[5] = vp[7] = r.y + r.h;

    // Texture coordinates.
    float uv[8];
    uv[0] = uv[1] = uv[3] = uv[4] = 0;
    uv[2] = uv[6] = static_cast<float>(r.w) / 16.0f;
    uv[5] = uv[7] = static_cast<float>(r.h) / 16.0f;

    // Render quad.
    Renderer::setColor(color);
    Renderer::bindShader(Renderer::SH_TEXTURE);
    Renderer::bindTexture(GD->misc.checkerboard.handle());
    Renderer::drawQuads(1, vp, uv);
}

};  // namespace Vortex
