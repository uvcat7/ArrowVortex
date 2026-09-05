#pragma once

#include <Core/Input.h>

#include <Editor/Action.h>

namespace Vortex {

struct Shortcuts {
    static void create();
    static void destroy();

    /// Returns the key notation for the shortcut associated with the given
    /// action.
    virtual std::string getNotation(Action::Type action, bool fullList) = 0;

    /// Returns the action associated with the given key press / keyflags
    /// combination.
    virtual Action::Type getAction(int keyflags, Key::Code key) = 0;

    /// Returns the action associated with the given mouse scroll/ keyflags
    /// combination.
    virtual Action::Type getAction(int keyflags, bool scrollUp) = 0;

    // The functions below walk the list of bindable actions, whose order is
    // alphabetical on their name.

    /// Returns the number of actions that can be bound to a key.
    virtual int getNumActions() = 0;

    /// Returns the name of the action at the given index, as it is written in
    /// the shortcuts file.
    virtual const char* getActionName(int index) = 0;

    /// Returns the action at the given index.
    virtual Action::Type getActionCode(int index) = 0;

    /// Replaces the bindings of an action with a single key combination. Any
    /// other action that used the same combination loses it.
    virtual void setBinding(Action::Type action, int keyflags,
                            Key::Code key) = 0;

    /// Replaces the bindings of an action with a single mouse scroll
    /// combination.
    virtual void setScrollBinding(Action::Type action, int keyflags,
                                  bool scrollUp) = 0;

    /// Removes every binding of the given action.
    virtual void clearBindings(Action::Type action) = 0;

    /// Writes the current bindings to the shortcuts file.
    virtual bool saveToFile() = 0;

    /// Discards the current bindings and reads them back from the file.
    virtual void reloadFromFile() = 0;

    /// Replaces the bindings with the ones the program ships with. The change
    /// is not written to disk until the bindings are saved.
    virtual void restoreDefaults() = 0;
};

extern Shortcuts* gShortcuts;

};  // namespace Vortex
