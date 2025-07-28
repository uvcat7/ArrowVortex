#pragma once

#include <Core/Input.h>

namespace Vortex {

struct Minimap : public InputHandler {
    enum Mode { NOTES, DENSITY };

    static void create();
    static void destroy();

    virtual void tick() = 0;
    virtual void draw() = 0;

    virtual void setMode(Mode mode) = 0;
    virtual Mode getMode() const = 0;

    /// Called by the editor when changes were made to the simfile.
    virtual void onChanges(int changes) = 0;
};

extern Minimap* gMinimap;

};  // namespace Vortex
