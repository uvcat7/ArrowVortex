#include <Dialogs/KeyBindings.h>

#include <Core/StringUtils.h>
#include <Core/Gui.h>
#include <Core/WidgetsLayout.h>

#include <Editor/Common.h>
#include <Editor/Shortcuts.h>

#include <System/System.h>

namespace Vortex {

// ================================================================================================
// Helper functions.

namespace {

/// Turns "OPEN_DIALOG_CHART_LIST" into "Open dialog chart list", which reads a
/// lot better in a list of two hundred entries.
static std::string PrettyName(const char* name) {
    std::string out;
    bool startOfWord = true;
    for (const char* p = name; *p; ++p) {
        char c = *p;
        if (c == '_') {
            out.push_back(' ');
            startOfWord = false;
            continue;
        }
        if (startOfWord) {
            out.push_back(c);
            startOfWord = false;
        } else {
            out.push_back(static_cast<char>(tolower(c)));
        }
    }
    return out;
}

/// Case-insensitive substring test, used by the filter box.
static bool Contains(const std::string& haystack, const std::string& needle) {
    if (needle.empty()) return true;
    if (needle.size() > haystack.size()) return false;
    for (size_t i = 0; i + needle.size() <= haystack.size(); ++i) {
        size_t j = 0;
        while (j < needle.size() &&
               tolower(haystack[i + j]) == tolower(needle[j])) {
            ++j;
        }
        if (j == needle.size()) return true;
    }
    return false;
}

/// Modifier keys are only meaningful in combination with another key, so they
/// cannot be a binding of their own.
static bool IsModifier(Key::Code key) {
    return key == Key::CTRL_L || key == Key::CTRL_R || key == Key::ALT_L ||
           key == Key::ALT_R || key == Key::SHIFT_L || key == Key::SHIFT_R;
}

};  // anonymous namespace

// ================================================================================================
// DialogKeyBindings.

DialogKeyBindings::~DialogKeyBindings() = default;

DialogKeyBindings::DialogKeyBindings() {
    mySelectedRow = 0;
    myIsListening = false;

    setTitle("KEY BINDINGS");
    myCreateWidgets();
    myUpdateList();
}

void DialogKeyBindings::myCreateWidgets() {
    myLayout.row().col(560);
    myFilter = myLayout.add<WgLineEdit>();
    myFilter->text.bind(&myFilterText);
    myFilter->onChange.bind(this, &DialogKeyBindings::onFilterChanged);
    myFilter->setTooltip("Type to narrow down the list of actions");

    myLayout.row().col(560);
    myList = myLayout.addH<WgSelectList>(gSystem->applyScaleFactor(380));
    myList->value.bind(&mySelectedRow);
    myList->onChange.bind(this, &DialogKeyBindings::onSelectionChanged);
    myList->alignItemsLeft();
    myList->setTooltip("Select an action, then press Rebind");

    myLayout.row().col(560);
    myStatus = myLayout.add<WgLabel>();

    myLayout.row().col(278).col(278);
    myRebindButton = myLayout.add<WgButton>();
    myRebindButton->text.set("Rebind");
    myRebindButton->onPress.bind(this, &DialogKeyBindings::onRebind);
    myRebindButton->setTooltip("Press, then hit the key combination to assign");

    WgButton* clear = myLayout.add<WgButton>();
    clear->text.set("Clear");
    clear->onPress.bind(this, &DialogKeyBindings::onClear);
    clear->setTooltip("Remove the bindings of the selected action");

    myLayout.row().col(560);
    myLayout.add<WgSeperator>();

    myLayout.row().col(184).col(184).col(184);
    WgButton* save = myLayout.add<WgButton>();
    save->text.set("Save");
    save->onPress.bind(this, &DialogKeyBindings::onSave);
    save->setTooltip("Write the bindings to settings/shortcuts.txt");

    WgButton* reload = myLayout.add<WgButton>();
    reload->text.set("Reload");
    reload->onPress.bind(this, &DialogKeyBindings::onReload);
    reload->setTooltip("Discard the changes and read the file again");

    WgButton* defaults = myLayout.add<WgButton>();
    defaults->text.set("Defaults");
    defaults->onPress.bind(this, &DialogKeyBindings::onRestoreDefaults);
    defaults->setTooltip("Bring back the bindings the program ships with");
}

// ================================================================================================
// DialogKeyBindings :: list contents.

void DialogKeyBindings::myUpdateList() {
    // Remember what was selected, so the row survives a change of filter.
    Action::Type selected = mySelectedAction();

    myVisibleActions.clear();
    myList->clearItems();

    int numActions = gShortcuts->getNumActions();
    for (int i = 0; i < numActions; ++i) {
        std::string label = PrettyName(gShortcuts->getActionName(i));
        std::string keys = gShortcuts->getNotation(gShortcuts->getActionCode(i),
                                                   /* fullList */ true);

        if (!Contains(label, myFilterText) && !Contains(keys, myFilterText)) {
            continue;
        }

        // The binding goes in a column of its own against the right edge, so
        // it stays readable no matter how long the action name is.
        myList->addItem(label, keys);
        myVisibleActions.push_back(i);
    }

    // Restore the selection, or fall back to the first row.
    mySelectedRow = 0;
    for (int i = 0; i < myVisibleActions.size(); ++i) {
        if (gShortcuts->getActionCode(myVisibleActions[i]) == selected) {
            mySelectedRow = i;
            break;
        }
    }
    myList->value.set(mySelectedRow);

    myUpdateSelectionLabel();
}

Action::Type DialogKeyBindings::mySelectedAction() const {
    if (mySelectedRow < 0 || mySelectedRow >= myVisibleActions.size()) {
        return Action::NONE;
    }
    return gShortcuts->getActionCode(myVisibleActions[mySelectedRow]);
}

void DialogKeyBindings::myUpdateSelectionLabel() {
    if (myIsListening) {
        myStatus->text.set("Press a key combination, escape to cancel");
        return;
    }

    Action::Type action = mySelectedAction();
    if (action == Action::NONE) {
        myStatus->text.set("No action selected");
        return;
    }

    std::string keys = gShortcuts->getNotation(action, true);
    if (keys.empty()) {
        myStatus->text.set("Not bound to a key");
    } else {
        myStatus->text.set("Bound to " + keys);
    }
}

// ================================================================================================
// DialogKeyBindings :: widget callbacks.

void DialogKeyBindings::onFilterChanged() { myUpdateList(); }

void DialogKeyBindings::onSelectionChanged() {
    // Picking another action while listening would assign the key to the wrong
    // one, so the selection cancels the capture.
    if (myIsListening) {
        myIsListening = false;
        myRebindButton->text.set("Rebind");
    }
    myUpdateSelectionLabel();
}

void DialogKeyBindings::onRebind() {
    if (mySelectedAction() == Action::NONE) return;

    myIsListening = !myIsListening;
    myRebindButton->text.set(myIsListening ? "Waiting..." : "Rebind");

    // Capturing text keeps the editor from running the shortcut we are about to
    // record.
    if (myIsListening) {
        myRebindButton->startCapturingText();
    } else {
        myRebindButton->stopCapturingText();
    }

    myUpdateSelectionLabel();
}

void DialogKeyBindings::onClear() {
    Action::Type action = mySelectedAction();
    if (action == Action::NONE) return;

    gShortcuts->clearBindings(action);
    myUpdateList();
}

void DialogKeyBindings::onSave() { gShortcuts->saveToFile(); }

void DialogKeyBindings::onRestoreDefaults() {
    gShortcuts->restoreDefaults();
    myUpdateList();
}

void DialogKeyBindings::onReload() {
    gShortcuts->reloadFromFile();
    myUpdateList();
}

// ================================================================================================
// DialogKeyBindings :: input handling.

void DialogKeyBindings::onTick() {
    handleInputs(getGui()->getEvents());
    EditorDialog::onTick();
}

void DialogKeyBindings::onKeyPress(KeyPress& evt) {
    if (evt.handled) return;

    if (!myIsListening) {
        if (evt.key == Key::ESCAPE) {
            requestClose();
            evt.handled = true;
        }
        return;
    }

    // The user is assigning a key.
    evt.handled = true;

    if (evt.key == Key::ESCAPE) {
        myIsListening = false;
        myRebindButton->text.set("Rebind");
        myRebindButton->stopCapturingText();
        myUpdateSelectionLabel();
        return;
    }

    // Holding a modifier down is part of the combination, not the end of it.
    if (IsModifier(evt.key)) return;

    Action::Type action = mySelectedAction();
    if (action != Action::NONE) {
        gShortcuts->setBinding(action, evt.keyflags, evt.key);
    }

    myIsListening = false;
    myRebindButton->text.set("Rebind");
    myRebindButton->stopCapturingText();
    myUpdateList();
}

void DialogKeyBindings::onMouseScroll(MouseScroll& evt) {
    if (!myIsListening || evt.handled) return;

    Action::Type action = mySelectedAction();
    if (action != Action::NONE) {
        gShortcuts->setScrollBinding(action, evt.keyflags, evt.up);
    }
    evt.handled = true;

    myIsListening = false;
    myRebindButton->text.set("Rebind");
    myRebindButton->stopCapturingText();
    myUpdateList();
}

};  // namespace Vortex
