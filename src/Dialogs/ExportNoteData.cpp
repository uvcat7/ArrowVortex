#include <Dialogs/ExportNoteData.h>

#include <Core/Draw.h>
#include <Core/StringUtils.h>
#include <Core/Widgets.h>
#include <Core/WidgetsLayout.h>

#include <Editor/View.h>
#include <Editor/Selection.h>

#include <Managers/TempoMan.h>

#include <Simfile/Tempo.h>
#include <Simfile/NoteList.h>

namespace Vortex {

// ================================================================================================
// DialogExportNoteData :: export formatting helpers.

struct ExportField {
    const char* key;
    std::string value;
    bool isString;
};

static const char* ToNoteChar(uint32_t type, bool full) {
    uint8_t off = full ? NUM_NOTE_TYPES : 0;
    static const char* text[NUM_NOTE_TYPES * 2] = {
        "1", "M", "2", "L", "F", "Tap", "Mine", "Roll", "Lift", "Fake"};
    return (type >= 0 && type < NUM_NOTE_TYPES) ? text[type + off] : text[0];
}

static const char* ToHoldChar(uint32_t type, bool full) {
    if (full) return (type == NOTE_STEP_OR_HOLD) ? "Hold" : "Roll";
    return (type == NOTE_STEP_OR_HOLD) ? "2" : "4";
}

static const char* ToQuant(RowType st) {
    static const char* text[NUM_ROW_TYPES] = {"4",  "8",  "12", "16", "24",
                                              "32", "48", "64", "192"};
    return (st >= 0 && st < NUM_ROW_TYPES) ? text[st] : text[0];
}

static double ToBeat(int row) {
    return static_cast<double>(row) / ROWS_PER_BEAT;
}

// ================================================================================================
// DialogExportNoteData :: dialog.

const uint32_t MAX_PREVIEW_NOTES = 20;

enum Format { JSON, LUA, CSV, YAML, TEXT };

struct DialogExportNoteData::ExportBox : public GuiWidget {
    explicit ExportBox(GuiContext* gui) : GuiWidget(gui) {}

    TextSlot text;
    CallSlot onPress;

    void onMousePress(MousePress& evt) override {
        if (isMouseOver()) {
            if (isEnabled() && evt.button == Mouse::LMB && !evt.handled) {
                startCapturingMouse();
                onPress.call();
            }
            evt.handled = true;
        }
    }

    void onMouseRelease(MouseRelease& evt) override {
        if (isCapturingMouse() && evt.button == Mouse::LMB) {
            stopCapturingMouse();
        }
    }

    void onDraw() override {
        recti r = rect_;

        auto& box = GuiDraw::getTextBox();
        box.base.draw(r);
        if (isCapturingMouse()) {
            box.active.draw(r);
        } else if (isMouseOver()) {
            box.hover.draw(r);
        }

        recti area = {r.x + 6, r.y + 4, r.w - 6 * 2, r.h - 4 * 2};
        if (area.w <= 0 || area.h <= 0) return;

        TextStyle style;
        style.textFlags |= Text::ELLIPSES;

        Renderer::pushScissorRect(r.x + 3, r.y + 1, r.w - 6, r.h - 2);
        Text::arrange(Text::TL, style, area.w, text.get());
        Text::draw(vec2i{area.x, area.y});
        Renderer::popScissorRect();
    }
};

DialogExportNoteData::~DialogExportNoteData() = default;

DialogExportNoteData::DialogExportNoteData() {
    myFormat = 0;

    includeBeat = true;
    includeSecond = false;
    includeColumn = true;
    includeType = true;
    includeQuantization = false;
    includeLength = true;

    optionOffsetColumn = false;
    optionPadNumbers = false;
    optionFullNames = false;
    optionMinify = false;

    myExportText = ExportData(MAX_PREVIEW_NOTES);

    setTitle("EXPORT NOTE DATA");
    myCreateWidgets();
}

void DialogExportNoteData::myCreateWidgets() {
    myLayout.row().col(180).col(260);

    // Column 1
    RowLayout* options = new RowLayout(getGui(), gSystem->applyScaleFactor(4));
    myLayout.add(options);
    options->row().col(180);

    WgCheckbox* option = options->add<WgCheckbox>();
    option->text.set("Row");
    option->value.bind(&includeRow);
    option->onChange.bind(this, &DialogExportNoteData::onAction);

    option = options->add<WgCheckbox>();
    option->text.set("Beat");
    option->value.bind(&includeBeat);
    option->onChange.bind(this, &DialogExportNoteData::onAction);

    option = options->add<WgCheckbox>();
    option->text.set("Second");
    option->value.bind(&includeSecond);
    option->onChange.bind(this, &DialogExportNoteData::onAction);

    option = options->add<WgCheckbox>();
    option->text.set("Column");
    option->value.bind(&includeColumn);
    option->onChange.bind(this, &DialogExportNoteData::onAction);

    option = options->add<WgCheckbox>();
    option->text.set("Type");
    option->value.bind(&includeType);
    option->onChange.bind(this, &DialogExportNoteData::onAction);

    option = options->add<WgCheckbox>();
    option->text.set("Quantization");
    option->value.bind(&includeQuantization);
    option->onChange.bind(this, &DialogExportNoteData::onAction);

    option = options->add<WgCheckbox>();
    option->text.set("Length");
    option->value.bind(&includeLength);
    option->onChange.bind(this, &DialogExportNoteData::onAction);
    option->setTooltip("Total length of a note in beats.");

    options->add<WgSeperator>();

    option = options->add<WgCheckbox>();
    option->text.set("Offset Column");
    option->value.bind(&optionOffsetColumn);
    option->onChange.bind(this, &DialogExportNoteData::onAction);
    option->setTooltip("Start counting column numbers from 1 instead of 0.");

    option = options->add<WgCheckbox>();
    option->text.set("Pad numbers");
    option->value.bind(&optionPadNumbers);
    option->onChange.bind(this, &DialogExportNoteData::onAction);
    option->setTooltip("Pad decimals to three places.");

    option = options->add<WgCheckbox>();
    option->text.set("Full Names");
    option->value.bind(&optionFullNames);
    option->onChange.bind(this, &DialogExportNoteData::onAction);
    option->setTooltip("Use full names instead of Stepmania shorthand.");

    option = options->add<WgCheckbox>();
    option->text.set("Minify");
    option->value.bind(&optionMinify);
    option->onChange.bind(this, &DialogExportNoteData::onAction);
    option->setTooltip("Remove newlines, whitespace, and optional data.");

    options->add<WgSeperator>();

    WgDroplist* drop = options->add<WgDroplist>();
    drop->value.bind(&myFormat);
    drop->onChange.bind(this, &DialogExportNoteData::onAction);
    drop->addItem("JSON");
    drop->addItem("Lua");
    drop->addItem("CSV");
    drop->addItem("YAML");
    drop->addItem("Text");

    // Column 2
    RowLayout* col2 = new RowLayout(getGui(), gSystem->applyScaleFactor(4));
    myLayout.add(col2);
    col2->row().col(260);

    ExportBox* output = col2->addH<ExportBox>(316);
    output->text.bind(&myExportText);
    output->setTooltip(
        "Preview is truncated to show only a few notes.\n"
        "Click to copy full output.");
    output->onPress.bind(this, &DialogExportNoteData::onCopy);

    WgButton* copy = col2->add<WgButton>();
    copy->text.set("{g:copy}");
    copy->setTooltip("Click to copy full output.");
    copy->onPress.bind(this, &DialogExportNoteData::onCopy);
}

void DialogExportNoteData::onAction() {
    myExportText = ExportData(MAX_PREVIEW_NOTES);
}

void DialogExportNoteData::onCopy() {
    auto out = ExportData(0);
    if (out.empty()) {
        HudInfo("There is no note data to copy...");
    } else {
        gSystem->setClipboardText(out.c_str());
        HudInfo("Note data copied to clipboard.");
    }
}

void DialogExportNoteData::onChanges(int changes) {
    if (changes & VCM_SELECTION_CHANGED) {
        myExportText = ExportData(MAX_PREVIEW_NOTES);
    }
}

std::string DialogExportNoteData::ExportData(int noteCap) {
    NoteList notes;
    gSelection->getSelectedNotes(notes);
    if (notes.empty() && noteCap == 0) {
        HudInfo("No notes currently selected...");
        return std::string();
    }
    // Example needs a note.
    else if (notes.empty()) {
        notes.append({0, 0, 0, 0, NOTE_STEP_OR_HOLD, 4});
    }

    const int minDec = optionPadNumbers ? 3 : 0;
    const int colOff = optionOffsetColumn ? 1 : 0;
    const bool mini = optionMinify;
    const bool names = optionFullNames;
    int count = 0;

    std::vector<ExportField> fields;
    fields.reserve(7);
    auto collect = [&](const Note& n) {
        fields.clear();
        if (includeRow) {
            fields.push_back({"row", Str::val(n.row), false});
        }
        if (includeBeat) {
            fields.push_back(
                {"beat", Str::val(ToBeat(n.row), minDec, 3), false});
        }
        if (includeSecond) {
            fields.push_back(
                {"time", Str::val(gTempo->rowToTime(n.row), minDec, 3), false});
        }
        if (includeColumn) {
            fields.push_back(
                {"column", Str::val(static_cast<int>(n.col) + colOff), false});
        }
        if (includeType) {
            auto c = (n.row != n.endrow) ? ToHoldChar(n.type, names)
                                         : ToNoteChar(n.type, names);
            fields.push_back({"type", c, true});
        }
        if (includeQuantization) {
            fields.push_back({"quant", ToQuant(ToRowType(n.row)), false});
        }
        if (includeLength) {
            fields.push_back({"length",
                              Str::val(ToBeat(n.endrow - n.row), minDec, 3),
                              false});
        }
    };

    std::string out;
    out.reserve(notes.size() * 48);
    auto value = [&](const ExportField& f, bool quoteStrings) {
        if (f.isString && quoteStrings) {
            out += '"';
            out += f.value;
            out += '"';
        } else {
            out += f.value;
        }
    };

    switch (myFormat) {
        case TEXT: {
            for (const Note& n : notes) {
                if (noteCap > 0 && count++ >= noteCap) break;
                collect(n);
                for (size_t i = 0; i < fields.size(); ++i) {
                    if (i) out += ' ';
                    if (!mini) {
                        out += fields[i].key;
                        out += '=';
                    }
                    value(fields[i], false);
                }
                out += '\n';
            }
            break;
        }
        case JSON:
        case LUA: {
            const bool lua = (myFormat == LUA);
            const char* nl = mini ? "" : "\n";
            const char* indent = mini ? "" : "  ";
            const char* sep = mini ? "," : ", ";
            const char* eq = mini ? "=" : " = ";

            if (lua) {
                out += mini ? "notes={" : "notes = {";
            } else {
                out += '[';
            }
            out += nl;

            bool firstNote = true;
            for (const Note& n : notes) {
                if (noteCap > 0 && count++ >= noteCap) break;
                collect(n);

                if (!firstNote) {
                    out += ',';
                    out += nl;
                }
                firstNote = false;

                out += indent;
                out += lua ? '{' : '[';
                for (size_t i = 0; i < fields.size(); ++i) {
                    if (i) out += sep;
                    if (lua && !mini) {
                        out += fields[i].key;
                        out += eq;
                    }
                    value(fields[i], true);
                }
                out += lua ? '}' : ']';
            }

            out += nl;
            out += lua ? '}' : ']';
            break;
        }
        case CSV: {
            bool header = !mini;
            for (const Note& n : notes) {
                if (noteCap > 0 && count++ >= noteCap) break;
                collect(n);
                if (header) {
                    for (size_t i = 0; i < fields.size(); ++i) {
                        if (i) out += ',';
                        out += fields[i].key;
                    }
                    out += '\n';
                    header = false;
                }
                for (size_t i = 0; i < fields.size(); ++i) {
                    if (i) out += ',';
                    value(fields[i], false);
                }
                out += '\n';
            }
            break;
        }
        case YAML: {
            for (const Note& n : notes) {
                if (noteCap > 0 && count++ >= noteCap) break;
                collect(n);
                if (mini) {
                    out += "- {";
                    for (size_t i = 0; i < fields.size(); ++i) {
                        if (i) out += ", ";
                        out += fields[i].key;
                        out += ": ";
                        value(fields[i], true);
                    }
                    out += "}\n";
                } else {
                    for (size_t i = 0; i < fields.size(); ++i) {
                        out += (i == 0) ? "- " : "  ";
                        out += fields[i].key;
                        out += ": ";
                        value(fields[i], true);
                        out += '\n';
                    }
                }
            }
            break;
        }
    }

    return out;
}

};  // namespace Vortex