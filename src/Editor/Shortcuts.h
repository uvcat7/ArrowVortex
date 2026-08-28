#pragma once

#include <Core/Input.h>

#include <Editor/Action.h>

namespace Vortex {

struct Shortcuts {
    static void create();
    static void destroy();

    /// Returns if the current input event is valid for the given action.
    virtual bool isAction(KeyPress* press, Action::Type action) = 0;
    virtual bool isAction(MouseScroll* scroll, Action::Type action) = 0;

    /// Returns the key notation for the shortcut associated with the given
    /// action.
    virtual std::string getNotation(Action::Type action, bool fullList) = 0;

    /// Returns the action associated with the given key press / keyflags
    /// combination.
    virtual Action::Type getAction(int keyflags, Key::Code key) = 0;

    /// Returns the action associated with the given mouse scroll/ keyflags
    /// combination.
    virtual Action::Type getAction(int keyflags, bool scrollUp) = 0;
};

extern Shortcuts* gShortcuts;

};  // namespace Vortex
