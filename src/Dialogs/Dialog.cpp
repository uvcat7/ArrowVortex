#include <Dialogs/Dialog.h>
#include <Editor/Editor.h>
#include <System/System.h>

namespace Vortex {

// One name per entry of DialogId, in the same order. The size comes from the
// list rather than the enum, so that a missing name is caught here instead of
// leaving a null pointer for the settings reader to walk into.
static const char* IdStrings[] = {
    "adjustSync",     "adjustTempo",     "adjustTempoSM5",
    "chartList",      "chartProperties", "dancingBot",
    "generateNotes",  "newChart",        "songProperties",
    "tempoBreakdown", "labelBreakdown",  "waveformSettings",
    "zoom",           "customSnap",      "previewSettings",
    "editSegment",    "export"};
static_assert(sizeof(IdStrings) / sizeof(IdStrings[0]) == NUM_DIALOG_IDS,
              "every dialog needs a name");

EditorDialog::~EditorDialog() { gEditor->onDialogClosed(myId); }

EditorDialog::EditorDialog()
    : GuiDialog(gEditor->getGui()),
      myLayout(gEditor->getGui(), gSystem->applyScaleFactor(4)) {}

void EditorDialog::onUpdateSize() {
    myLayout.onUpdateSize();
    setWidth(myLayout.getWidth());
    setHeight(myLayout.getHeight());
}

void EditorDialog::onTick() {
    myLayout.onArrange(getInnerRect());
    myLayout.onTick();
}

void EditorDialog::onDraw() { myLayout.onDraw(); }

void EditorDialog::setId(DialogId id) { myId = id; }

void EditorDialog::setWidgetId(GuiWidget* widget, const char* name) {
    myWidgetMap.push_back({widget, name});
}

void EditorDialog::setFocus(const char* name) {
    for (WidgetMapping w : myWidgetMap) {
        if (strcmp(w.name, name) == 0) {
            w.widget->setFocus();
            break;
        }
    }
}

DialogId EditorDialog::getId(const char* name) {
    for (int i = 0; i < NUM_DIALOG_IDS; ++i) {
        if (strcmp(IdStrings[i], name) == 0) {
            return static_cast<DialogId>(i);
        }
    }
    return NUM_DIALOG_IDS;
}

const char* EditorDialog::getName(DialogId id) { return IdStrings[id]; }

};  // namespace Vortex
