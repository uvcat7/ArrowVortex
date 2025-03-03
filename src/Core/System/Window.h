#pragma once

#include <Core/Common/Input.h>
#include <Core/Types/Vec2.h>
#include <Core/Types/Size2.h>
#include <Core/System/File.h>

namespace AV {

// Manages the application window.
namespace Window
{
	// Enumeration of mouse cursor icons.
	enum class CursorIcon
	{
		Arrow,    // Standard arrow.
		Hand,     // Hand icon.
		IBeam,    // I-beam.
		SizeAll,  // Horizontal/vertical arrows.
		SizeWE,   // Horizontal arrows.
		SizeNS,   // Vertical arrows.
		SizeNESW, // Arrows pointing northeast/southwest.
		SizeNWSE, // Arrows pointing northwest/southeast.
	
		Max // One past the highest valid value.
	};

	// A menu item.
	struct MenuItem
	{
		struct Logic
		{
			virtual ~Logic();
			virtual void update(MenuItem& item) {}
			virtual void run() = 0;
		};
		virtual void setEnabled(bool enabled) = 0;
		virtual void setChecked(bool checked) = 0;
	};

	// A menu that contains a list of items.
	struct Menu
	{
		struct Logic
		{
			virtual ~Logic();
			virtual void update(Menu& menu) = 0;
		};
		virtual Menu& addMenu(stringref name, unique_ptr<Logic> logic) = 0;
		virtual MenuItem& addItem(stringref name, unique_ptr<MenuItem::Logic> logic) = 0;
		virtual void addSeparator() = 0;
		virtual void clear() = 0;
	};
	
	void initialize();
	void deinitialize();
	
	// Shows the window.
	void show();
	
	// Handle window messages at the start of a render frame.
	void handleMessages();

	// Adds a top-level menu item to the window.
	Menu& addMenu(stringref name);
	
	// Sets the icon type of the application's mouse cursor.
	void setCursor(CursorIcon icon);
	
	// Sets the window title text.
	void setTitle(stringref text);
	
	// Returns true if any key is currently being pressed.
	bool isAnyKeyDown();
	
	// Returns true if the key is currently being pressed.
	bool isKeyDown(KeyCode key);
	
	// Returns true if the mouse button is currently being pressed.
	bool isMouseDown(MouseButton button);

	// Returns whether the window is minimized.
	bool isMinimized();
	
	// Returns the current mouse position.
	Vec2 getMousePos();
	
	// Returns flags that indicate which keys are currently being pressed.
	ModifierKeys getKeyFlags();
	
	// Returns the size of the window.
	Size2 getSize();
	
	// Returns the elapsed startTime in the previous frame.
	double getDeltaTime();
	
	// Determines which buttons are shown in a dialog.
	enum class Button { Ok, OkCancel, YesNo, YesNoCancel, Count };
	
	// Determines which icon is shown in a dialog.
	enum class Icon { None, Info, Warning, Error, Count };
	
	// Indicates which button was pressed by the user in a dialog.
	enum class Result { Cancel, Ok, Yes, No, Count };
	
	// Shows a message box dialog.
	Result showMessageDlg(stringref title, stringref text, Button buttons = Button::Ok, Icon icon = Icon::Info);

	// Example filters string for open/save file: "TextStyle (*.txt)\0*.txt\0All Files (*.*)\0*.*\0".
	// In save file, the index of the filter selected by the user is written to outFilterIndex,
	// starting with index 1. If the user selected a custom filter, filterIndex is set to zero.
	
	// Shows an open file dialog.
	FilePath openFileDlg(stringref title, int& filterIndex, FilePath initial = FilePath(), stringref extFilters = std::string());
	
	// Shows a save file dialog.
	FilePath saveFileDlg(stringref title, int& filterIndex, FilePath initial = FilePath(), stringref extFilters = std::string());

	// Shows an open directory dialog.
	DirectoryPath openDirDlg(stringref title, DirectoryPath initial);
};

} // namespace AV
