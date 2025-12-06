#pragma once

#include <System/System.h>

namespace Vortex {

struct Menubar {
    static void create();
    static void destroy();

    /// Enumeration of properties that can be updated.
    enum Property {

        ALL_PROPERTIES,

        OPEN_FILE,
        RECENT_FILES,

        SHOW_WAVEFORM,
        SHOW_BEATLINES,
        SHOW_NOTES,
        SHOW_TEMPO_BOXES,
        SHOW_TEMPO_HELP,

        USE_JUMP_TO_NEXT_NOTE,
        USE_UNDO_REDO_JUMP,
        USE_TIME_BASED_COPY,
        USE_REVERSE_SCROLL,
        USE_CHART_PREVIEW,

        VISUAL_SYNC_ANCHOR,

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
        STATUSBAR_TIME,
        STATUSBAR_TIMING_MODE,
        STATUSBAR_SCROLL,
        STATUSBAR_SPEED,

        NUM_PROPERTIES

    };

    /// Creates the menu's and submenu's when the window is initialized.
    virtual void init(System::MenuItem* menu) = 0;

    /// Updates the menu items associated with the given property.
    virtual void update(Property prop) = 0;
};

extern Menubar* gMenubar;

};  // namespace Vortex
