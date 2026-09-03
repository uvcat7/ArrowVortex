#pragma once

#include <vector>

#include <Dialogs/Dialog.h>
#include <Core/Widgets.h>
#include <Core/WidgetsLayout.h>

#include <Editor/Action.h>

namespace Vortex {

/// Lists every action the editor knows about and lets the user rebind it.
class DialogKeyBindings : public EditorDialog, public InputHandler {
   public:
    ~DialogKeyBindings();
    DialogKeyBindings();

    void onTick() override;
    void onKeyPress(KeyPress& evt) override;
    void onMouseScroll(MouseScroll& evt) override;

   private:
    void myCreateWidgets();
    void myUpdateList();
    void myUpdateSelectionLabel();

    void onFilterChanged();
    void onSelectionChanged();
    void onRebind();
    void onClear();
    void onSave();
    void onReload();
    void onRestoreDefaults();

    /// Returns the action of the selected row, or Action::NONE.
    Action::Type mySelectedAction() const;

    WgLineEdit* myFilter;
    WgSelectList* myList;
    WgLabel* myStatus;
    WgButton* myRebindButton;

    /// Indices into the action list of the rows currently shown.
    std::vector<int> myVisibleActions;

    std::string myFilterText;
    int mySelectedRow;

    /// While true, the next key press or scroll becomes the new binding.
    bool myIsListening;
};

};  // namespace Vortex
