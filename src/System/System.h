#pragma once

#include <Core/Input.h>

namespace Vortex {

// Example filters string for open/save file: "Text (*.txt)\0*.txt\0All Files
// (*.*)\0*.*\0". In save file, the index of the filter selected by the user is
// written to outFilterIndex, starting with index 1. If the user selected a
// custom filter, filterIndex is set to zero.

struct System {
    // Helper struct for building the menu bar.
    struct MenuItem {
        static MenuItem* create();

        void addSeperator();
        void addItem(int item, const std::string& text);
        void addSubmenu(MenuItem* submenu, const std::string& text,
                        bool grayed = false);
        void replaceSubmenu(int pos, MenuItem* submenu, const std::string& text,
                            bool grayed = false);

        void setChecked(int item, bool checked);
        void setEnabled(int item, bool checked);
    };

    /// Helper struct for running system commands.
    struct CommandPipe {
        virtual int read() = 0;
    };

    /// Determines which buttons are shown in a dialog.
    enum Buttons { T_OK, T_OK_CANCEL, T_YES_NO, T_YES_NO_CANCEL, NUM_BUTTONS };

    /// Determines which icon is shown in a dialog.
    enum Icon { I_NONE, I_INFO, I_WARNING, I_ERROR, NUM_ICONS };

    /// Indicates which button was pressed by the user in a dialog.
    enum Result { R_CANCEL, R_OK, R_YES, R_NO, NUM_RESULTS };

    /// Shows a message box dialog.
    virtual Result showMessageDlg(const std::string& title,
                                  const std::string& text,
                                  Buttons buttons = T_OK,
                                  Icon icon = I_INFO) = 0;

    /// Shows an open file dialog, see class description.
    virtual std::string openFileDlg(
        const std::string& title,
        const std::string& initialPath = std::string(),
        const std::string& extFilters = std::string()) = 0;

    /// Shows a save file dialog, see class description.
    virtual std::string saveFileDlg(
        const std::string& title,
        const std::string& initialPath = std::string(),
        const std::string& extFilters = std::string(),
        int* outFilterIndex = nullptr) = 0;

    /// Runs a system command.
    virtual bool runSystemCommand(const std::string& cmd) = 0;

    /// Runs a system command and pipes data to it.
    virtual bool runSystemCommand(const std::string& cmd, CommandPipe* pipe,
                                  void* buffer) = 0;

    /// Opens a link to a webpage in the default browser.
    virtual void openWebpage(const std::string& link) = 0;

    /// Sets the current working directory to the given path.
    virtual void setWorkingDir(const std::string& path) = 0;

    /// Sends the given text to the clipboard.
    virtual bool setClipboardText(const std::string& text) = 0;

    /// Sets the icon type of the application's mouse cursor.
    virtual void setCursor(Cursor::Icon c) = 0;

    /// Disables vsync.
    virtual void disableVsync() = 0;

    /// Returns the elapsed time since the application was started.
    virtual double getElapsedTime() const = 0;

    /// Returns the HWND of the main window.
    virtual void* getHWND() const = 0;

    /// Returns the current clipboard text.
    virtual std::string getClipboardText() const = 0;

    /// Returns the directory of the executable.
    virtual std::string getExeDir() const = 0;

    /// Returns the directory from which the program was run.
    virtual std::string getRunDir() const = 0;

    /// Returns the current mouse cursor icon.
    virtual Cursor::Icon getCursor() const = 0;

    /// Returns true if the given key is currently down.
    virtual bool isKeyDown(Key::Code key) const = 0;

    /// Returns true if the given mouse button is currently down.
    virtual bool isMouseDown(Mouse::Code button) const = 0;

    /// Returns the current position of the mouse cursor.
    virtual vec2i getMousePos() const = 0;

    /// Returns the state of several system keys as a combination of KeyFlags.
    virtual int getKeyFlags() const = 0;

    /// Sets the window title text.
    virtual void setWindowTitle(const std::string& text) = 0;

    /// Returns the current window size.
    virtual vec2i getWindowSize() const = 0;

    /// Returns the current window title.
    virtual const std::string& getWindowTitle() const = 0;

    /// Returns the list of input events for the current frame.
    virtual InputEvents& getEvents() = 0;

    /// Returns true if the window is currently in focus.
    virtual bool isActive() const = 0;

    /// Terminates the application at the end of the current message loop.
    virtual void terminate() = 0;

    /// Returns the current local time.
    static std::string getLocalTime();

    /// Returns the build date of the application.
    static std::string getBuildData();
};

extern System* gSystem;

};  // namespace Vortex
