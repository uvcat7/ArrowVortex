#include <Editor/Shortcuts.h>

#include <Core/Xmr.h>
#include <Core/Vector.h>
#include <Core/StringUtils.h>

#include <System/Debug.h>

#include <Editor/Common.h>

#include <algorithm>

namespace Vortex {
namespace {

/// Returns the name of the next key in the shortcut string, and advances the
/// string position p.
static const char* ReadNextKey(char*& p) {
    while (*p == ' ') ++p;
    char* begin = p;
    while (*p && *p != '+') ++p;
    char* end = p;
    while (end > begin && end[-1] == ' ') --end;
    *end = 0;
    if (*p == '+') ++p;
    return begin;
}

/// Returns the first element in map that matches key, or null if it does not
/// exist.
template <typename T, typename K, typename C>
T* BinarySearch(T* map, int size, const K& key, C comp) {
    int pos = 0, count = size, step, i;
    while (count > 0) {
        step = count >> 1;
        i = pos + step;
        if (comp(map[i], key) < 0) {
            pos = ++i;
            count -= ++step;
        } else
            count = step;
    }
    if (pos == size || comp(map[pos], key) != 0) {
        return nullptr;
    }
    return map + pos;
}

// ================================================================================================
// Mapping of key names to key codes.

using namespace Key;

struct KeyEntry {
    const char* name;
    char chr;
    Code code;
};

static KeyEntry keyMap[] = {

    {"Accent", '`', ACCENT},
    {"Dash", '-', DASH},
    {"Equal", '=', EQUAL},
    {"Left Bracket", '[', BRACKET_L},
    {"Right Bracket", ']', BRACKET_R},
    {"Semicolon", ';', SEMICOLON},
    {"Quote", '\'', QUOTE},
    {"Backslash", '\\', BACKSLASH},
    {"Comma", ',', COMMA},
    {"Period", '.', PERIOD},
    {"Slash", '/', SLASH},
    {"Space", ' ', SPACE},

    {"Ctrl", 0, CTRL_L},
    {"Alt", 0, ALT_L},
    {"Shift", 0, SHIFT_L},
    {"Escape", 0, ESCAPE},
    {"Tab", 0, TAB},
    {"Caps", 0, CAPS},
    {"Return", 0, RETURN},
    {"Backspace", 0, BACKSPACE},
    {"Page Up", 0, PAGE_UP},
    {"Page Down", 0, PAGE_DOWN},
    {"Home", 0, HOME},
    {"End", 0, END},
    {"Insert", 0, INSERT},
    {"Delete", 0, DELETE},
    {"Print Screen", 0, PRINT_SCREEN},
    {"Scroll Lock", 0, SCROLL_LOCK},
    {"Pause", 0, PAUSE},
    {"Left", 0, LEFT},
    {"Right", 0, RIGHT},
    {"Up", 0, UP},
    {"Down", 0, DOWN},
    {"Num Lock", 0, NUM_LOCK},

    {"Numpad Divide", 0, NUMPAD_DIVIDE},
    {"Numpad Multiply", 0, NUMPAD_MULTIPLY},
    {"Numpad Subtract", 0, NUMPAD_SUBTRACT},
    {"Numpad Add", 0, NUMPAD_ADD},
    {"Numpad Seperator", 0, NUMPAD_SEPERATOR},

    {"Numpad 0", 0, NUMPAD_0},
    {"Numpad 1", 0, NUMPAD_1},
    {"Numpad 2", 0, NUMPAD_2},
    {"Numpad 3", 0, NUMPAD_3},
    {"Numpad 4", 0, NUMPAD_4},
    {"Numpad 5", 0, NUMPAD_5},
    {"Numpad 6", 0, NUMPAD_6},
    {"Numpad 7", 0, NUMPAD_7},
    {"Numpad 8", 0, NUMPAD_8},
    {"Numpad 9", 0, NUMPAD_9},

    {"F1", 0, F1},
    {"F2", 0, F2},
    {"F3", 0, F3},
    {"F4", 0, F4},
    {"F5", 0, F5},
    {"F6", 0, F6},
    {"F7", 0, F7},
    {"F8", 0, F8},
    {"F9", 0, F9},
    {"F10", 0, F10},
    {"F11", 0, F11},
    {"F12", 0, F12},
    {"F13", 0, F13},
    {"F14", 0, F14},
    {"F15", 0, F15},

    {"A", 0, A},
    {"B", 0, B},
    {"C", 0, C},
    {"D", 0, D},
    {"E", 0, E},
    {"F", 0, F},
    {"G", 0, G},
    {"H", 0, H},
    {"I", 0, I},
    {"J", 0, J},
    {"K", 0, K},
    {"L", 0, L},
    {"M", 0, M},
    {"N", 0, N},
    {"O", 0, O},
    {"P", 0, P},
    {"Q", 0, Q},
    {"R", 0, R},
    {"S", 0, S},
    {"T", 0, T},
    {"U", 0, U},
    {"V", 0, V},
    {"W", 0, W},
    {"X", 0, X},
    {"Y", 0, Y},
    {"Z", 0, Z},

    {"0", 0, DIGIT_0},
    {"1", 0, DIGIT_1},
    {"2", 0, DIGIT_2},
    {"3", 0, DIGIT_3},
    {"4", 0, DIGIT_4},
    {"5", 0, DIGIT_5},
    {"6", 0, DIGIT_6},
    {"7", 0, DIGIT_7},
    {"8", 0, DIGIT_8},
    {"9", 0, DIGIT_9},

};

static const int NUM_MAPPED_KEYS = sizeof(keyMap) / sizeof(keyMap[0]);

// ================================================================================================
// Mapping of action names to actions codes.

using namespace Action;

struct ActionEntry {
    const char* name;
    Action::Type code;
};

static ActionEntry actionMap[] = {

#define E(x) {#x, x},

    E(FILE_OPEN) E(FILE_SAVE) E(FILE_SAVE_AS) E(FILE_CLOSE)

        E(OPEN_DIALOG_SONG_PROPERTIES) E(OPEN_DIALOG_CHART_PROPERTIES) E(
            OPEN_DIALOG_CHART_LIST) E(OPEN_DIALOG_NEW_CHART) E(OPEN_DIALOG_ADJUST_SYNC)
            E(OPEN_DIALOG_ADJUST_TEMPO) E(OPEN_DIALOG_ADJUST_TEMPO_SM5) E(
                OPEN_DIALOG_DANCING_BOT) E(OPEN_DIALOG_GENERATE_NOTES)
                E(OPEN_DIALOG_TEMPO_BREAKDOWN) E(OPEN_DIALOG_LABEL_BREAKDOWN) E(
                    OPEN_DIALOG_WAVEFORM_SETTINGS) E(OPEN_DIALOG_CUSTOM_SNAP) E(OPEN_DIALOG_ZOOM)

                    E(TOGGLE_JUMP_TO_NEXT_NOTE) E(TOGGLE_UNDO_REDO_JUMP) E(
                        TOGGLE_TIME_BASED_COPY)

                        E(SET_VISUAL_SYNC_CURSOR_ANCHOR) E(
                            SET_VISUAL_SYNC_RECEPTOR_ANCHOR)
                            E(INJECT_BOUNDING_BPM_CHANGE) E(
                                SHIFT_ROW_NONDESTRUCTIVE) E(SHIFT_ROW_DESTRUCTIVE)

                                E(SELECT_REGION) E(SELECT_ALL_STEPS) E(
                                    SELECT_ALL_MINES) E(SELECT_ALL_HOLDS) E(SELECT_ALL_ROLLS)
                                    E(SELECT_ALL_FAKES) E(SELECT_ALL_LIFTS) E(
                                        SELECT_REGION_BEFORE_CURSOR) E(SELECT_REGION_AFTER_CURSOR)

                                        E(SELECT_QUANT_4) E(SELECT_QUANT_8) E(
                                            SELECT_QUANT_12) E(SELECT_QUANT_16) E(SELECT_QUANT_24)
                                            E(SELECT_QUANT_32) E(SELECT_QUANT_48) E(
                                                SELECT_QUANT_64) E(SELECT_QUANT_192)

                                                E(SELECT_TEMPO_BPM) E(
                                                    SELECT_TEMPO_STOP) E(SELECT_TEMPO_DELAY)
                                                    E(SELECT_TEMPO_WARP) E(
                                                        SELECT_TEMPO_TIME_SIG) E(SELECT_TEMPO_TICK_COUNT)
                                                        E(SELECT_TEMPO_COMBO) E(
                                                            SELECT_TEMPO_SPEED) E(SELECT_TEMPO_SCROLL)
                                                            E(SELECT_TEMPO_FAKE) E(
                                                                SELECT_TEMPO_LABEL)

                                                                E(CHART_PREVIOUS) E(
                                                                    CHART_NEXT) E(CHART_DELETE)

                                                                    E(SIMFILE_PREVIOUS) E(
                                                                        SIMFILE_NEXT)

                                                                        E(CHART_CONVERT_COUPLES_TO_ROUTINE) E(
                                                                            CHART_CONVERT_ROUTINE_TO_COUPLES)

                                                                            E(CHANGE_NOTES_TO_MINES)
                                                                                E(CHANGE_NOTES_TO_FAKES)
                                                                                    E(CHANGE_NOTES_TO_LIFTS)
                                                                                        E(CHANGE_MINES_TO_NOTES)
                                                                                            E(CHANGE_MINES_TO_FAKES)
                                                                                                E(CHANGE_MINES_TO_LIFTS)
                                                                                                    E(CHANGE_FAKES_TO_NOTES)
                                                                                                        E(CHANGE_LIFTS_TO_NOTES)
                                                                                                            E(CHANGE_HOLDS_TO_STEPS)
                                                                                                                E(CHANGE_HOLDS_TO_MINES)
                                                                                                                    E(CHANGE_BETWEEN_HOLDS_AND_ROLLS)
                                                                                                                        E(CHANGE_BETWEEN_PLAYER_NUMBERS)

                                                                                                                            E(MIRROR_NOTES_VERTICALLY)
                                                                                                                                E(MIRROR_NOTES_HORIZONTALLY)
                                                                                                                                    E(MIRROR_NOTES_FULL)

                                                                                                                                        E(EXPORT_NOTES_AS_LUA_TABLE)

                                                                                                                                            E(SCALE_NOTES_2_TO_1) E(SCALE_NOTES_3_TO_2) E(SCALE_NOTES_4_TO_3) E(SCALE_NOTES_1_TO_2) E(SCALE_NOTES_2_TO_3) E(
                                                                                                                                                SCALE_NOTES_3_TO_4)

                                                                                                                                                E(SWITCH_TO_SYNC_MODE)

                                                                                                                                                    E(VOLUME_RESET) E(
                                                                                                                                                        VOLUME_INCREASE)
                                                                                                                                                        E(VOLUME_DECREASE)
                                                                                                                                                            E(VOLUME_MUTE)

                                                                                                                                                                E(SPEED_RESET) E(SPEED_INCREASE) E(SPEED_DECREASE)

                                                                                                                                                                    E(TOGGLE_BEAT_TICK) E(
                                                                                                                                                                        TOGGLE_NOTE_TICK)

                                                                                                                                                                        E(TOGGLE_SHOW_WAVEFORM) E(
                                                                                                                                                                            TOGGLE_SHOW_BEAT_LINES)
                                                                                                                                                                            E(TOGGLE_SHOW_NOTES)
                                                                                                                                                                                E(TOGGLE_SHOW_TEMPO_BOXES)
                                                                                                                                                                                    E(TOGGLE_SHOW_TEMPO_HELP) E(TOGGLE_REVERSE_SCROLL) E(TOGGLE_CHART_PREVIEW)

                                                                                                                                                                                        E(MINIMAP_SET_NOTES)
                                                                                                                                                                                            E(MINIMAP_SET_DENSITY)

                                                                                                                                                                                                E(BACKGROUND_HIDE)
                                                                                                                                                                                                    E(BACKGROUND_INCREASE_ALPHA)
                                                                                                                                                                                                        E(BACKGROUND_DECREASE_ALPHA) E(BACKGROUND_SET_STRETCH) E(BACKGROUND_SET_LETTERBOX) E(
                                                                                                                                                                                                            BACKGROUND_SET_CROP)

                                                                                                                                                                                                            E(USE_TIME_BASED_VIEW) E(
                                                                                                                                                                                                                USE_ROW_BASED_VIEW)

                                                                                                                                                                                                                E(ZOOM_RESET) E(
                                                                                                                                                                                                                    ZOOM_IN) E(ZOOM_OUT) E(SCALE_INCREASE) E(SCALE_DECREASE)

                                                                                                                                                                                                                    E(SNAP_RESET)
                                                                                                                                                                                                                        E(SNAP_NEXT)
                                                                                                                                                                                                                            E(SNAP_PREVIOUS)

                                                                                                                                                                                                                                E(CURSOR_UP) E(CURSOR_DOWN) E(CURSOR_PREVIOUS_BEAT) E(
                                                                                                                                                                                                                                    CURSOR_NEXT_BEAT)
                                                                                                                                                                                                                                    E(CURSOR_PREVIOUS_MEASURE)
                                                                                                                                                                                                                                        E(CURSOR_NEXT_MEASURE)
                                                                                                                                                                                                                                            E(CURSOR_STREAM_START)
                                                                                                                                                                                                                                                E(CURSOR_STREAM_END)
                                                                                                                                                                                                                                                    E(
                                                                                                                                                                                                                                                        CURSOR_SELECTION_START) E(CURSOR_SELECTION_END) E(CURSOR_CHART_START) E(CURSOR_CHART_END)

                                                                                                                                                                                                                                                        E(TOGGLE_STATUS_CHART) E(TOGGLE_STATUS_SNAP) E(TOGGLE_STATUS_BPM)
                                                                                                                                                                                                                                                            E(TOGGLE_STATUS_ROW) E(
                                                                                                                                                                                                                                                                TOGGLE_STATUS_BEAT)
                                                                                                                                                                                                                                                                E(TOGGLE_STATUS_MEASURE) E(TOGGLE_STATUS_TIME) E(TOGGLE_STATUS_TIMING_MODE)
                                                                                                                                                                                                                                                                    E(TOGGLE_STATUS_SCROLL)
                                                                                                                                                                                                                                                                        E(TOGGLE_STATUS_SPEED)

                                                                                                                                                                                                                                                                            E(PREVIEW_TOGGLE_ENABLED) E(PREVIEW_TOGGLE_SHOW_BEAT_LINES) E(PREVIEW_TOGGLE_REVERSE_SCROLL)
                                                                                                                                                                                                                                                                                E(PREVIEW_VIEW_CMOD)
                                                                                                                                                                                                                                                                                    E(PREVIEW_VIEW_XMOD)
                                                                                                                                                                                                                                                                                        E(PREVIEW_VIEW_XMOD_ALL)

                                                                                                                                                                                                                                                                                            E(SHOW_SHORTCUTS) E(SHOW_MESSAGE_LOG)
                                                                                                                                                                                                                                                                                                E(SHOW_DEBUG_LOG)
                                                                                                                                                                                                                                                                                                    E(SHOW_ABOUT)
#undef E

};

static const int NUM_MAPPED_ACTIONS = sizeof(actionMap) / sizeof(actionMap[0]);

// ================================================================================================
// ShortcutsImpl :: member data.

struct ShortcutsImpl : public Shortcuts {
    struct ShortcutEntry {
        ActionEntry* action;
        KeyEntry* key;
        int keyflags;
        bool scrollUp;
    };

    Vector<ShortcutEntry> shortcutMappings_;

    // ================================================================================================
    // ShortcutsImpl :: constructor and destructor.

    void ParseShortcuts(ActionEntry* action, char* str) {
        ShortcutEntry entry = {action, nullptr, 0, false};
        bool isScroll = false;

        // Translate the shortcut to a combination of key codes.
        for (char* p = str; *p;) {
            // Find the key code corresponding to the key name.
            const char* keyName = ReadNextKey(p);
            auto key = BinarySearch(keyMap, NUM_MAPPED_KEYS, keyName,
                                    [](const KeyEntry& a, const char* b) {
                                        return Str::icompare(a.name, b);
                                    });

            // If the key was not found, it might be "scroll up" or "scroll
            // down" for mouse scrolling.
            if (!key) {
                if (Str::iequal(keyName, "scroll up")) {
                    entry.scrollUp = true;
                    isScroll = true;
                } else if (Str::iequal(keyName, "scroll down")) {
                    entry.scrollUp = false;
                    isScroll = true;
                } else {
                    HudWarning("Shortcut with invalid key \"%s\"", keyName);
                    return;
                }
            } else {
                // Translate the key code to keyflags if it is a modifier key.
                switch (key->code) {
                    case CTRL_L:
                        entry.keyflags |= Keyflag::CTRL;
                        break;
                    case ALT_L:
                        entry.keyflags |= Keyflag::ALT;
                        break;
                    case SHIFT_L:
                        entry.keyflags |= Keyflag::SHIFT;
                        break;
                    default:
                        entry.key = key;
                };
            }
        }

        // Store the shortcut if it has a key or scroll direction.
        if (entry.key || isScroll) {
            shortcutMappings_.push_back(entry);
        }
    }

    ShortcutsImpl() {
        /// Sort the named actions by name.
        std::sort(actionMap, actionMap + NUM_MAPPED_ACTIONS,
                  [](const ActionEntry& a, const ActionEntry& b) -> bool {
                      return Str::icompare(a.name, b.name) < 0;
                  });

        /// Sort the keycode map.
        std::sort(keyMap, keyMap + NUM_MAPPED_KEYS,
                  [](const KeyEntry& a, const KeyEntry& b) -> bool {
                      return Str::icompare(a.name, b.name) < 0;
                  });

        // Load the shortcuts file.
        XmrDoc doc;
        if (doc.loadFile("settings/shortcuts.txt") != XMR_SUCCESS) {
            HudError("Could not load shortcuts file.");
        }

        // Create a list of shortcuts.
        ForXmrAttribs(attribute, &doc) {
            if (strlen(attribute->name) > 0 && attribute->numValues > 0) {
                if (*attribute->values[0] == 0) continue;

                // Find the action that corresponds to the shortcut.
                auto action =
                    BinarySearch(actionMap, NUM_MAPPED_ACTIONS, attribute->name,
                                 [](const ActionEntry& a, const char* b) {
                                     return Str::icompare(a.name, b);
                                 });
                if (!action) {
                    HudWarning("Invalid shortcut \"%s\"", attribute->name);
                    continue;
                }

                // Parse the strings that define the key combinations.
                for (int i = 0; i < attribute->numValues; ++i) {
                    ParseShortcuts(action, attribute->values[i]);
                }
            }
        }
    }

    ~ShortcutsImpl() {}

    // ================================================================================================
    // ShortcutsImpl :: API functions.

    std::string Shortcuts::getNotation(Action::Type action,
                                       bool fullList = false) {
        std::string out;
        for (auto& shortcut : shortcutMappings_) {
            if (shortcut.action->code == action) {
                if (out.length()) {
                    out = out + ", ";
                }

                if (shortcut.keyflags & Keyflag::CTRL) {
                    out = out + "Ctrl+";
                }
                if (shortcut.keyflags & Keyflag::SHIFT) {
                    out = out + "Shift+";
                }
                if (shortcut.keyflags & Keyflag::ALT) {
                    out = out + "Alt+";
                }
                if (shortcut.key == nullptr) {
                    out =
                        out + (shortcut.scrollUp ? "Scroll Up" : "Scroll Down");
                } else if (shortcut.key->chr) {
                    out = out + shortcut.key->chr;
                } else {
                    out = out + shortcut.key->name;
                }

                if (!fullList) {
                    break;
                }
            }
        }
        return out;
    }

    Action::Type Shortcuts::getAction(int keyflags, Code key) {
        for (auto& shortcut : shortcutMappings_) {
            if (shortcut.key && shortcut.key->code == key) {
                if (shortcut.keyflags == keyflags) {
                    return (Action::Type)shortcut.action->code;
                }
            }
        }
        return Action::NONE;
    }

    Action::Type getAction(int keyflags, bool scrollUp) {
        for (auto& shortcut : shortcutMappings_) {
            if (shortcut.key == nullptr && shortcut.scrollUp == scrollUp) {
                if (shortcut.keyflags == keyflags) {
                    return (Action::Type)shortcut.action->code;
                }
            }
        }
        return Action::NONE;
    }

};  // ShortcutsImpl
};  // anonymous namespace.

// ================================================================================================
// Shortcuts API.

Shortcuts* gShortcuts = nullptr;

void Shortcuts::create() { gShortcuts = new ShortcutsImpl; }

void Shortcuts::destroy() {
    delete (ShortcutsImpl*)gShortcuts;
    gShortcuts = nullptr;
}

};  // namespace Vortex
