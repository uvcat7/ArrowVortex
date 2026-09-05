#pragma once

#include <string>

#include <Core/Widgets.h>
#include <Dialogs/Dialog.h>

#include <Simfile/Simfile.h>

namespace Vortex {

/// Asks which game the song is packed for, and shows the name that game's
/// archive wants. Picking a format rewrites the name, which is still free to
/// edit.
class DialogExport : public EditorDialog {
   public:
    ~DialogExport();
    DialogExport();

   private:
    void onFormatChanged();
    void onExport();
    void onCancel();

    /// Fills the name field with the name the current format asks for.
    void myUpdateName();

    int myFormat;
    std::string myName;
};

};  // namespace Vortex
