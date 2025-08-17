#pragma once

#include <Core/Input.h>

#include <Editor/Action.h>

namespace Vortex {

struct Shortcuts {
    static void create();
    static void destroy();

    /// Returns the key notation for the shortcut associated with the given
    /// action.
    virtual std::string getNotation(Action::Type action,
                                    bool fullList = false) = 0;

    /// Returns the action associated with the given key press / keyflags
    /// combination.
    virtual Action::Type getAction(int keyflags, Key::Code key) = 0;

    /// Returns the action associated with the given mouse scroll/ keyflags
    /// combination.
    virtual Action::Type getAction(int keyflags, bool scrollUp) = 0;
};

extern Shortcuts* gShortcuts;

};  // namespace Vortex
