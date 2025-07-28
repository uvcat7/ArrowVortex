#pragma once

#include <Core/Draw.h>

namespace Vortex {

struct GuiDraw {
    struct Dialog {
        TileRect2 titlebar, frame;
        Texture vshape;
    };

    struct Icons {
        Texture pin, unpin;
        Texture grab, arrow;
        Texture plus, minus, cross;
        Texture check;
    };

    struct Button {
        TileRect2 base, hover, pressed;
    };

    struct Scrollbar {
        TileRect2 bar, base, hover, pressed;
        Texture grab;
    };

    struct TextBox {
        TileRect2 base, hover, active;
    };

    struct Misc {
        Texture checkerboard;
        uint32_t colDisabled;
        TileRect imgSelect;
    };

    static void create();
    static void destroy();

    static Dialog& getDialog();
    static Button& getButton();
    static Scrollbar& getScrollbar();
    static TextBox& getTextBox();
    static Icons& getIcons();
    static Misc& getMisc();

    static void button(TileRect* tr, const recti& r, bool hover, bool focus);

    static void checkerboard(recti r, uint32_t color);
};

};  // namespace Vortex
