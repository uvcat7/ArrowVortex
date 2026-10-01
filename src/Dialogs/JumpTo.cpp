#include <Dialogs/JumpTo.h>

#include <Core/Draw.h>
#include <Core/WidgetsLayout.h>
#include <Core/StringUtils.h>

#include <Editor/View.h>

#include <Managers/NoteMan.h>

#include <Simfile/Tempo.h>

namespace Vortex {

enum Target { ROW, BEAT, MEASURE, TIME, NOTE_ID };

DialogJumpTo::~DialogJumpTo() = default;

DialogJumpTo::DialogJumpTo() {
    myTargetType = 1;

    setTitle("JUMP TO");
    myCreateWidgets();
}

void DialogJumpTo::myCreateWidgets() {
    myLayout.row().col(190);

    myLayout.add<WgLabel>()->text.set("Target Type");
    myTypeList = myLayout.addH<WgSelectList>(gSystem->applyScaleFactor(96));
    myTypeList->value.bind(&myTargetType);
    myTypeList->setTooltip("Type of value to jump to.");

    myTypeList->addItem("Time");
    myTypeList->addItem("{tc:BBB}R{tc}ow");
    myTypeList->addItem("{tc:BBB}B{tc}eat");
    myTypeList->addItem("{tc:BBB}M{tc}easure");
    myTypeList->addItem("{tc:BBB}N{tc}ote");

    WgLineEdit* target = myLayout.add<WgLineEdit>();
    target->text.bind(&myJumpToTarget);
    target->setTooltip(
        "Type prefixes can also be used directly, such as:\n"
        "r192\nb16.125\nm4.25");

    WgButton* go = myLayout.add<WgButton>();
    go->text.set("Go");
    go->onPress.bind(this, &DialogJumpTo::onJump);
}

void DialogJumpTo::onJump() {
    static const std::string chars = "rbm:n";

    // Check for target type.
    std::string text = myJumpToTarget;
    Str::toLower(text);

    Target type = static_cast<Target>(myTargetType);
    size_t pos = text.find_first_of(chars);
    if (pos != std::string::npos)
        type = static_cast<Target>(chars.find(text[pos]));

    // Strip invalid chars.
    text.erase(std::remove_if(text.begin(), text.end(),
                              [](unsigned char c) {
                                  return !((c >= '0' && c <= '9') || c == '.' ||
                                           c == ':');
                              }),
               text.end());

    // off we go
    switch (type) {
        case NOTE_ID: {
            if (gNotes->empty()) return;

            // assume it's 1-indexed cause person.
            int target = std::stoi(text);
            int noteCount = static_cast<int>(gNotes->end() - gNotes->begin());
            target = std::clamp(target - 1, 0, noteCount - 1);

            const ExpandedNote* note = gNotes->begin() + target;
            gView->setCursorRow(note->row);
            break;
        }
        case TIME: {
            double target = Str::readTime(text);
            gView->setCursorTime(target);
            break;
        }
        case MEASURE: {
            double target = std::stod(text);
            gView->setCursorRow(static_cast<int>(target * (ROWS_PER_BEAT * 4)));
            break;
        }
        case ROW: {
            int target = std::stoi(text);
            gView->setCursorRow(target);
            break;
        }
        case BEAT:
        default: {
            double target = std::stod(text);
            gView->setCursorRow(static_cast<int>(target * ROWS_PER_BEAT));
            break;
        }
    }
}
};  // namespace Vortex