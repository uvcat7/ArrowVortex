#include <Dialogs/Dialog.h>
#include <Editor/Editor.h>
#include <System/System.h>

namespace Vortex {

static const char* IdStrings[NUM_DIALOG_IDS] = {
    "adjustSync",     "adjustTempo",     "adjustTempoSM5",
    "chartList",      "chartProperties", "dancingBot",
    "generateNotes",  "newChart",        "songProperties",
    "tempoBreakdown", "labelBreakdown",  "waveformSettings",
    "zoom",           "customSnap",      "previewSettings",
    "editSegment"};

EditorDialog::~EditorDialog() { gEditor->onDialogClosed(myId); }

EditorDialog::EditorDialog()
    : GuiDialog(gEditor->getGui()),
      myLayout(gEditor->getGui(),
               static_cast<int>(gSystem->getScaleFactor() * 4)) {}

void EditorDialog::onUpdateSize() {
    myLayout.onUpdateSize();
    float scale = gSystem->getScaleFactor();
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
