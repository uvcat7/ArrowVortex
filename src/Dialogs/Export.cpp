#include <Dialogs/Export.h>

#include <Core/Core.h>
#include <Core/WidgetsLayout.h>

#include <Editor/ExportArchive.h>

#include <Managers/SimfileMan.h>

namespace Vortex {
namespace {

struct FormatEntry {
    const char* name;
    SimFormat format;
};

// The archives a song can be packed into, in the order the dialog lists them.
const FormatEntry FORMATS[] = {
    {"osu! beatmap (.osz)", SIM_OSU},
    {"StepMania (.sm) in a .zip", SIM_SM},
    {"StepMania 5 (.ssc) in a .zip", SIM_SSC},
};

const int NUM_FORMATS = sizeof(FORMATS) / sizeof(FORMATS[0]);

};  // anonymous namespace

DialogExport::~DialogExport() = default;

DialogExport::DialogExport() {
    setTitle("EXPORT");

    myFormat = 0;

    // The format the simfile came from is the one to open on, so that a song
    // opened as a .osu offers an .osz.
    auto sim = gSimfile->get();
    if (sim) {
        for (int i = 0; i < NUM_FORMATS; ++i) {
            if (FORMATS[i].format == sim->format) myFormat = i;
        }
    }

    myLayout.row().col(64).col(300);

    auto formats = myLayout.add<WgDroplist>("Format");
    formats->value.bind(&myFormat);
    formats->onChange.bind(this, &DialogExport::onFormatChanged);
    for (auto& entry : FORMATS) formats->addItem(entry.name);
    formats->setTooltip("The game the archive is packed for");

    myLayout.row().col(64).col(300);

    auto name = myLayout.add<WgLineEdit>("Name");
    name->text.bind(&myName);
    name->setTooltip(
        "The name the chosen archive asks for. Edit it if you want another");

    myLayout.row().col(182).col(182);

    auto save = myLayout.add<WgButton>();
    save->text.set("Export...");
    save->onPress.bind(this, &DialogExport::onExport);
    save->setTooltip("Choose where the archive goes and pack it");

    auto cancel = myLayout.add<WgButton>();
    cancel->text.set("Cancel");
    cancel->onPress.bind(this, &DialogExport::onCancel);

    myUpdateName();
}

void DialogExport::myUpdateName() {
    myName = SuggestedArchiveName(FORMATS[myFormat].format);
}

void DialogExport::onFormatChanged() { myUpdateName(); }

void DialogExport::onCancel() { requestClose(); }

void DialogExport::onExport() {
    ExportArchive(FORMATS[myFormat].format, myName);
    requestClose();
}

};  // namespace Vortex
