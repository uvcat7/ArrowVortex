#pragma once

#include <System/Menu.h>
#include <Core/Input.h>

namespace Vortex {

struct Menubar : public InputHandler {
    static void create();
    static void destroy();

    /// Enumeration of properties that can be updated.
    enum Property {

        ALL_PROPERTIES,

        OPEN_FILE,
        RECENT_FILES,

        SHOW_WAVEFORM,
        SHOW_NOTES,
        SHOW_TEMPO_BOXES,
        SHOW_TEMPO_HELP,

        USE_JUMP_TO_NEXT_NOTE,
        USE_UNDO_REDO_JUMP,
        USE_TIME_BASED_COPY,
        USE_REVERSE_SCROLL,
        USE_CHART_PREVIEW,

        VISUAL_SYNC_ANCHOR,
        TEMPO_EDIT_ANCHOR,

        SELECTION_TEMPO_EDITOR,

        SELECT_DENSITY,

        BEATLINE_ENABLED,
        BEATLINE_SNAP,
        BEATLINE_COLOR,
        BEATLINE_HOVER,

        VIEW_MINIMAP,
        VIEW_MODE,
        VIEW_BACKGROUND,
        VIEW_NOTESKIN,

        PREVIEW_ENABLED,
        PREVIEW_VIEW_MODE,
        PREVIEW_SHOW_BEATLINES,
        PREVIEW_SHOW_REVERSE_SCROLL,

        STATUSBAR_CHART,
        STATUSBAR_SNAP,
        STATUSBAR_BPM,
        STATUSBAR_ROW,
        STATUSBAR_BEAT,
        STATUSBAR_MEASURE,
        STATUSBAR_HOVER,
        STATUSBAR_TIME,
        STATUSBAR_TIMING_MODE,
        STATUSBAR_SCROLL,
        STATUSBAR_SPEED,
        PRESERVE_PITCH,

        NUM_PROPERTIES

    };

    /// Creates the menu's and submenu's when the window is initialized.
    virtual void init(MenuItem* menu) = 0;

    /// Updates the menu items associated with the given property.
    virtual void update(Property prop) = 0;

    /// Draws the menu bar for non-Windows platforms.
    virtual void draw() = 0;

    /// Closes all open menus.
    virtual void closeMenus() = 0;

    virtual int getMenubarHeight() = 0;
};

extern Menubar* gMenubar;

};  // namespace Vortex
