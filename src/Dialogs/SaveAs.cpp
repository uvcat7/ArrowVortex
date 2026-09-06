#include <Dialogs/SaveAs.h>

#include <Core/Core.h>
#include <Core/WidgetsLayout.h>

#include <Editor/Editor.h>

#include <Managers/SimfileMan.h>

namespace Vortex {
namespace {

struct FormatEntry {
    const char* name;
    SimFormat format;
};

// The formats a simfile can be written in, in the order the dialog lists them.
const FormatEntry FORMATS[] = {
    {"Stepmania/ITG (.sm)", SIM_SM},
    {"Stepmania 5 (.ssc)", SIM_SSC},
    {"Osu!mania (.osu)", SIM_OSU},
    {"Dance With Intensity (.dwi)", SIM_DWI},
};

const int NUM_FORMATS = sizeof(FORMATS) / sizeof(FORMATS[0]);

};  // anonymous namespace

DialogSaveAs::~DialogSaveAs() = default;

DialogSaveAs::DialogSaveAs() {
    setTitle("SAVE AS");

    myFormat = 0;

    // The format the simfile came from is the one to open on.
    auto sim = gSimfile->get();
    if (sim) {
        for (int i = 0; i < NUM_FORMATS; ++i) {
            if (FORMATS[i].format == sim->format) myFormat = i;
        }
    }

    myLayout.row().col(64).col(280);

    auto formats = myLayout.add<WgDroplist>("Format");
    formats->value.bind(&myFormat);
    formats->onChange.bind(this, &DialogSaveAs::onFormatChanged);
    for (auto& entry : FORMATS) formats->addItem(entry.name);
    formats->setTooltip("The format the file is written in");

    myLayout.row().col(64).col(280);

    myNameField = myLayout.add<WgLineEdit>("Name");
    myNameField->text.bind(&myName);
    myNameField->setTooltip(
        "The name the chosen format asks for. Edit it if you want another");

    myLayout.row().col(172).col(172);

    auto save = myLayout.add<WgButton>();
    save->text.set("Save...");
    save->onPress.bind(this, &DialogSaveAs::onSave);
    save->setTooltip("Choose where the file goes and write it");

    auto cancel = myLayout.add<WgButton>();
    cancel->text.set("Cancel");
    cancel->onPress.bind(this, &DialogSaveAs::onCancel);

    myUpdateName();
}

void DialogSaveAs::myUpdateName() {
    myName = gEditor->getSuggestedSaveName(FORMATS[myFormat].format);
}

void DialogSaveAs::onFormatChanged() { myUpdateName(); }

void DialogSaveAs::onCancel() { requestClose(); }

void DialogSaveAs::onSave() {
    if (gEditor->saveSimfileAs(FORMATS[myFormat].format, myName)) {
        requestClose();
    }
}

};  // namespace Vortex
