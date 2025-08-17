#pragma once

#include <Core/Core.h>

namespace Vortex {

struct Editor {
    static void create();
    static void destroy();

    virtual void tick() = 0;

    /// Tries to close the simfile. If there are unsaved changes and the user
    /// selects cancel when prompted, then the simfile is not closed and false
    /// is returned. Otherwise, the simfile is closed and true is returned.
    virtual bool closeSimfile() = 0;

    /// Prompts the user to select a file, and calls "openSimfile" with the
    /// resulting path.
    virtual bool openSimfile() = 0;

    /// Closes the current simfile and tries to load a new simfile from the
    /// given path. If closing is aborted or loading fails, false is returned.
    /// If loading succeeds, true is returned.
    virtual bool openSimfile(const std::string& path) = 0;

    /// Calls "openSimfile" with a path from the list of recently opened files.
    virtual bool openSimfile(int recentFileIndex) = 0;

    /// Calls "openSimfile" with the path of the next (or previous) simfile in
    /// the current pack. If finding or loading the simfile failed, false is
    /// returned. Otherwise, true is returned.
    virtual bool openNextSimfile(bool iterateForward) = 0;

    /// Tries to save the simfile. Returns true if saving succeeds, false
    /// otherwise.
    virtual bool saveSimfile(bool showSaveAsDialog) = 0;

    /// Empties the recently opened files list.
    virtual void clearRecentFiles() = 0;

    /// Returns the number of active entries in the recently opened files list.
    virtual int getNumRecentFiles() = 0;

    /// Returns the path string of a recently opened file.
    virtual const std::string& getRecentFile(int recentFileIndex) = 0;

    /// Use this to report changes in the simfile (combination of flags from
    /// "VortexChangesMade").
    virtual void reportChanges(int changes) = 0;

    /// Opens the dialog window with the given id, if it's currently closed.
    virtual void openDialog(int dialogId) = 0;

    /// Closes the dialog window with the given id, if it's currently open.
    virtual void onDialogClosed(int dialogId) = 0;

    /// Called by system when the application starts, with the command line
    /// arguments.
    virtual void onCommandLineArgs(const std::string* args, int numArgs) = 0;

    /// Called by system when a menu action is selected by the user.
    virtual void onMenuAction(int action) = 0;

    /// Called by system when the window close button is pressed.
    virtual void onExitProgram() = 0;

    /// Returns the main gui of the editor.
    virtual GuiContext* getGui() const = 0;

    /// Returns true if the multithreading is enabled in the editor settings,
    /// false otherwise.
    virtual bool hasMultithreading() const = 0;

    /// Sets the background scaling style.
    virtual void setBackgroundStyle(int style) = 0;

    /// Returns the background scaling style.
    virtual int getBackgroundStyle() const = 0;

    /// Returns the tags export mode set in the editor settings.
    virtual int getDefaultSaveFormat() const = 0;
};

extern Editor* gEditor;

};  // namespace Vortex