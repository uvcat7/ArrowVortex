#pragma once

#include <Core/Core.h>

namespace Vortex {

namespace Mouse {
/// Enumeration of mouse button codes.
enum Code {

    NONE = 0,

    LMB = 1,  ///< Left mouse button.
    MMB = 2,  ///< Middle mouse button.
    RMB = 3,  ///< Right mouse button.

    MAX_VALUE  ///< One past the highest valid code.

};
};  // namespace Mouse

namespace Cursor {
/// Enumeration of mouse cursor icons.
enum Icon {

    ARROW,      ///< Standard arrow.
    HAND,       ///< Hand icon.
    IBEAM,      ///< I-beam.
    SIZE_ALL,   ///< Horizontal/vertical arrows.
    SIZE_WE,    ///< Horizontal arrows.
    SIZE_NS,    ///< Vertical arrows.
    SIZE_NESW,  ///< Arrows pointing northeast/southwest.
    SIZE_NWSE,  ///< Arrows pointing northwest/southeast.

    NUM_CURSORS  ///< Total number of cursor icons.

};
};  // namespace Cursor

namespace Key {
/// Enumeration of keyboard key codes.
enum Code {

    NONE = 0,

    A = 'a',
    B = 'b',
    C = 'c',
    D = 'd',
    E = 'e',
    F = 'f',
    G = 'g',
    H = 'h',
    I = 'i',
    J = 'j',
    K = 'k',
    L = 'l',
    M = 'm',
    N = 'n',
    O = 'o',
    P = 'p',
    Q = 'q',
    R = 'r',
    S = 's',
    T = 't',
    U = 'u',
    V = 'v',
    W = 'w',
    X = 'x',
    Y = 'y',
    Z = 'z',

    DIGIT_0 = '0',
    DIGIT_1 = '1',
    DIGIT_2 = '2',
    DIGIT_3 = '3',
    DIGIT_4 = '4',
    DIGIT_5 = '5',
    DIGIT_6 = '6',
    DIGIT_7 = '7',
    DIGIT_8 = '8',
    DIGIT_9 = '9',

    ACCENT = '`',
    DASH = '-',
    EQUAL = '=',
    BRACKET_L = '[',
    BRACKET_R = ']',
    SEMICOLON = ';',
    QUOTE = '\'',
    BACKSLASH = '\\',
    COMMA = ',',
    PERIOD = '.',
    SLASH = '/',
    SPACE = ' ',

    // Non-character key codes start at 128 to prevent clashes with ascii
    // characters.
    ESCAPE = 128,

    CTRL_L,    ///< Left control key.
    CTRL_R,    ///< Right control key.
    ALT_L,     ///< Left alt key.
    ALT_R,     ///< Right alt key.
    SHIFT_L,   ///< Left shift key.
    SHIFT_R,   ///< Right shift key.
    SYSTEM_L,  ///< Left system key.
    SYSTEM_R,  ///< Right system key.

    TAB,
    CAPS,
    RETURN,
    BACKSPACE,
    PAGE_UP,
    PAGE_DOWN,
    HOME,
    END,
    INSERT,
    DELETE,
    PRINT_SCREEN,
    SCROLL_LOCK,
    PAUSE,

    LEFT,   ///< Left arrow key.
    RIGHT,  ///< Right arrow key.
    UP,     ///< Up arrow key.
    DOWN,   ///< Down arrow key.

    NUM_LOCK,

    NUMPAD_DIVIDE,     ///< Numpad divide key.
    NUMPAD_MULTIPLY,   ///< Numpad multiply key.
    NUMPAD_SUBTRACT,   ///< Numpad subtract key.
    NUMPAD_ADD,        ///< Numpad add key.
    NUMPAD_SEPERATOR,  ///< Numpad decimal seperation key.

    NUMPAD_0,
    NUMPAD_1,
    NUMPAD_2,
    NUMPAD_3,
    NUMPAD_4,
    NUMPAD_5,
    NUMPAD_6,
    NUMPAD_7,
    NUMPAD_8,
    NUMPAD_9,

    F1,
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    F12,
    F13,
    F14,
    F15,

    MAX_VALUE  ///< One past the highest valid code.

};
};  // namespace Key

namespace Keyflag {
/// Enumeration of bit flags for modifier keys.
enum Flag {

    CTRL = 1 << 0,   ///< Any control key.
    ALT = 1 << 1,    ///< Any alt key.
    SHIFT = 1 << 2,  ///< Any shift key.

};
};  // namespace Keyflag

/// Contains data from a mouse movement event.
struct MouseMove {
    int x, y;  ///< The position of the mouse cursor.
};

/// Contains data from a mouse button press event.
struct MousePress {
    MousePress(Mouse::Code code, int x, int y, int keyflags, bool doubleClick,
               bool handled);
    Mouse::Code button;  ///< The mouse button that was pressed.
    int x, y;            ///< The position of the mouse cursor.
    int keyflags;        ///< Keyflag values of modifier keys that were down.
    bool doubleClick;    ///< True if the press is the second press of a double
                         ///< click.
    bool unhandled() const { return !handled; }
    void setHandled();

   private:
    bool handled;  ///< Used to track if the event is handled.
};

/// Contains data from a mouse button release event.
struct MouseRelease {
    Mouse::Code button;  ///< The mouse button that was released.
    int x, y;            ///< The position of the mouse cursor.
    int keyflags;        ///< Keyflag values of modifier keys that were down.
    bool handled;        ///< Used to track if the event is handled.
};

/// Contains data from a mouse scroll event.
struct MouseScroll {
    bool up;       ///< True when scrolling up, false when scrolling down.
    int x, y;      ///< The position of the mouse cursor.
    int keyflags;  ///< Keyflag values of modifier keys that were down.
    bool handled;  ///< Used to track if the event is handled.
};

/// Contains data from a keyboard key press event.
struct KeyPress {
    Key::Code key;  ///< The key that was pressed.
    int keyflags;   ///< Keyflag values of modifier keys that were down.
    bool repeated;  ///< Indicates the press was an autorepeat triggered by
                    ///< holding down the key.
    bool handled;   ///< Used to track if the event is handled.
};

/// Contains data from a keyboard key release event.
struct KeyRelease {
    Key::Code key;  ///< The key that was released.
    int keyflags;   ///< Keyflag values of modifier keys that were down.
    bool handled;   ///< Used to track if the event is handled.
};

/// Contains data from a text input event.
struct TextInput {
    const char* text;  ///< The text that was entered (for example, by a key
                       ///< press or IME).
    bool handled;      ///< Used to track if the event is handled.
};

/// Contains data from a file drop event.
struct FileDrop {
    const char* const*
        files;  ///< A list of paths to the files that were dropped.
    int count;  ///< The number of files that were dropped.
    int x, y;   ///< The position in the window on which the files were dropped.
    bool handled;  ///< Used to track if the event is handled.
};

/// Contains a list of events that can be forwarded to input handler objects.
///
/// The "next" functions can be used in for-loops to iterate over all events of
/// a specific type. Example usage: for(KeyRelease* it = nullptr;
/// eventList.next(it);) { ... }.
///
class InputEvents {
   public:
    ~InputEvents();

    /// Constructs an empty list of events.
    InputEvents();

    /// Copies the events from another event list.
    InputEvents(const InputEvents& other);

    /// Removes all events from the list.
    void clear();

    /// Adds a mouse movement event; indicates the user moved the mouse cursor.
    void addMouseMove(int x, int y);

    /// Adds a mouse press event; indicates the user pressed a mouse button.
    void addMousePress(Mouse::Code button, int x, int y, int keyflags,
                       bool doubleClick);

    /// Adds a mouse release event; indicates the user released a mouse button.
    void addMouseRelease(Mouse::Code button, int x, int y, int keyflags);

    /// Adds a mouse scroll event; indicates the user scrolled the mouse wheel.
    void addMouseScroll(bool up, int x, int y, int keyflags);

    /// Adds a key press event; indicates the user presses a key.
    void addKeyPress(Key::Code key, int keyflags, bool repeated);

    /// Adds a key release event; indicates the user released a key.
    void addKeyRelease(Key::Code key, int keyflags);

    /// Adds a text input event; indicates the user entered text (e.g. through a
    /// key press or IME).
    void addTextInput(const char* text);

    /// Adds a file drop event; indicates the user dropped one or more files
    /// onto the window.
    void addFileDrop(const char* const* files, int count, int x, int y);

    /// Adds a window inactive event; indicates the window has lost focus.
    void addWindowInactive();

    /// Iterates over the mouse move events in the list, see class description.
    bool next(MouseMove*& it) const;

    /// Iterates over the mouse press events in the list, see class description.
    bool next(MousePress*& it) const;

    /// Iterates over the mouse release events in the list, see class
    /// description.
    bool next(MouseRelease*& it) const;

    /// Iterates over the mouse scroll events in the list, see class
    /// description.
    bool next(MouseScroll*& it) const;

    /// Iterates over the key press events in the list, see class description.
    bool next(KeyPress*& it) const;

    /// Iterates over the key release events in the list, see class description.
    bool next(KeyRelease*& it) const;

    /// Iterates over the text input events in the list, see class description.
    bool next(TextInput*& it) const;

    /// Iterates over the file drop events in the list, see class description.
    bool next(FileDrop*& it) const;

    /// Returns true if the list is empty, false otherwise.
    bool isEmpty() const { return !data_; }

    /// Copies the events from another event list.
    void operator=(const InputEvents& other);

   private:
    void* data_;  // TODO: replace with a more descriptive variable name.
    friend class InputHandler;
};

/// Base class for objects that can handles input events.
class InputHandler {
   public:
    /// Handles all the events in an event list. The base functions forwards
    /// each event in the list to their specific handler functions, but the
    /// function can be overloaded to perform other actions such as propagating
    /// the event list to other objects.
    virtual void handleInputs(InputEvents& inputs);

    /// Occurs when the user moves the mouse cursor.
    virtual void onMouseMove(MouseMove& move) {}

    /// Occurs when the user presses a mouse button.
    virtual void onMousePress(MousePress& press) {}

    /// Occurs when the user releases a mouse button.
    virtual void onMouseRelease(MouseRelease& release) {}

    /// Occurs when the user scrolls the mouse wheel.
    virtual void onMouseScroll(MouseScroll& scroll) {}

    /// Occurs when the user presses a key.
    virtual void onKeyPress(KeyPress& press) {}

    /// Occurs when the user releases a key.
    virtual void onKeyRelease(KeyRelease& release) {}

    /// Occurs when the user enters text (e.g. through a key press or IME).
    virtual void onTextInput(TextInput& input) {}

    /// Occurs when the user drops one or more files onto the window.
    virtual void onFileDrop(FileDrop& drop) {}

    /// Occurs when the window becomes inactive.
    virtual void onWindowInactive() {}
};

};  // namespace Vortex
