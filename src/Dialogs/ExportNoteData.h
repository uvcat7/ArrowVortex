#pragma once

#include <Dialogs/Dialog.h>
#include <Core/Widgets.h>
#include <Core/WidgetsLayout.h>

namespace Vortex {

class DialogExportNoteData : public EditorDialog, public InputHandler {
   public:
    ~DialogExportNoteData();
    DialogExportNoteData();

    void onChanges(int changes);

    void onAction();
    void onCopy();

   private:
    void myCreateWidgets();
    std::string ExportData(int noteCap);

    std::string myExportText;

    int myFormat;

    bool includeRow;
    bool includeBeat;
    bool includeSecond;
    bool includeColumn;
    bool includeType;
    bool includeQuantization;
    bool includeLength;

    bool optionOffsetColumn;
    bool optionPadNumbers;
    bool optionSMTypes;
    bool optionMinify;

    struct ExportBox;
};

};  // namespace Vortex
#pragma once
