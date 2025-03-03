#include <Precomp.h>

#include <bitset>
#include <filesystem>

#include <Core/Utils/Util.h>
#include <Core/Utils/Unicode.h>

#include <Core/Graphics/Vulkan.h>

#include <Core/System/Window.h>
#include <Core/System/System.h>
#include <Core/System/Debug.h>
#include <Core/System/Log.h>
#include <Core/System/File.h>
#include <Core/System/Resources/Resources.h>

#include <Core/Utils/Flag.h>

#include <Core/Interface/UiMan.h>

#define UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <commdlg.h>
#include <shobjidl_core.h>

namespace AV {

using namespace std;

typedef UINT MenuId;

struct MenuImpl;
struct MenuItemImpl;

// =====================================================================================================================
// Static data.

static const wchar_t* WindowClassName = L"ArrowVortex";

struct KeyPair { int raw; KeyCode key; };

// Mapping of windows virtual keys to vortex key codes.
static const KeyPair KeyPairs[] =
{
	{VK_OEM_3, KeyCode::Accent},
	{VK_OEM_MINUS, KeyCode::Dash},
	{VK_OEM_PLUS, KeyCode::Equal},
	{VK_OEM_4, KeyCode::BracketL},
	{VK_OEM_6, KeyCode::BracketR},
	{VK_OEM_1, KeyCode::Semicolon},
	{VK_OEM_7, KeyCode::Quote},
	{VK_OEM_5, KeyCode::Backslash},
	{VK_OEM_COMMA, KeyCode::Comma},
	{VK_OEM_PERIOD, KeyCode::Period},
	{VK_OEM_2, KeyCode::Slash},
	{VK_SPACE, KeyCode::Space},
	{VK_ESCAPE, KeyCode::Escape},
	{VK_LWIN, KeyCode::SystemL},
	{VK_RWIN, KeyCode::SystemR},
	{VK_TAB, KeyCode::Tab},
	{VK_CAPITAL, KeyCode::Caps},
	{VK_RETURN, KeyCode::Return},
	{VK_BACK, KeyCode::Backspace},
	{VK_PRIOR, KeyCode::PageUp},
	{VK_NEXT, KeyCode::PageDown},
	{VK_HOME, KeyCode::Home},
	{VK_END, KeyCode::End},
	{VK_INSERT, KeyCode::Insert},
	{VK_DELETE, KeyCode::Delete},
	{VK_SNAPSHOT, KeyCode::PrintScreen},
	{VK_SCROLL, KeyCode::ScrollLock},
	{VK_PAUSE, KeyCode::Pause},
	{VK_LEFT, KeyCode::Left},
	{VK_RIGHT, KeyCode::Right},
	{VK_UP, KeyCode::Up},
	{VK_DOWN, KeyCode::Down},
	{VK_NUMLOCK, KeyCode::NumLock},
	{VK_DIVIDE, KeyCode::NumpadDivide},
	{VK_MULTIPLY, KeyCode::NumpadMultiply},
	{VK_SUBTRACT, KeyCode::NumpadSubtract},
	{VK_ADD, KeyCode::NumpadAdd},
	{VK_SEPARATOR, KeyCode::NumpadSeperator},
};

namespace Window
{
	struct State
	{
		Size2 windowSize = { 900, 900 };
		HINSTANCE hinstance = GetModuleHandle(nullptr);
		Window::CursorIcon cursor = Window::CursorIcon::Arrow;
		KeyCode keyMap[256] = {};
		wstring textInput;
		string title = "ArrowVortex";
		DWORD style = 0;
		DWORD exStyle = 0;
		HWND hwnd = nullptr;
		bool isActive = false;
		bool isMinimized = false;
		double previousFrameTime = 0.0;
		double deltaTime = 1.0 / 60.0;
		Vec2 mousePos = { 0, 0 };
		set<KeyCode> keys;
		set<MouseButton> mouse;
		unique_ptr<MenuImpl> rootMenu;
	};
	static State* state = nullptr;
}
using Window::state;

// =====================================================================================================================
// Application menu.

struct MenuChild
{
	virtual void run() {}
	virtual void update(bool open) {}
};

Window::MenuItem::Logic::~Logic()
{
}

struct MenuItemImpl : MenuChild, Window::MenuItem
{
	MenuItemImpl(HMENU parent, UINT pos, unique_ptr<Window::MenuItem::Logic> l)
		: parent(parent)
		, pos(pos)
		, logic(move(l))
	{
		if (!logic)
			setEnabled(false);
	}
	void run() override
	{
		if (logic)
			logic->run();
	}
	void update(bool open) override
	{
		if (logic)
			logic->update(*this);
	}
	void setEnabled(bool enabled) override
	{
		EnableMenuItem(parent, pos, MF_BYPOSITION | (enabled ? MF_ENABLED : MF_GRAYED));
	}
	void setChecked(bool checked) override
	{
		CheckMenuItem(parent, pos, MF_BYPOSITION | (checked ? MF_CHECKED : MF_UNCHECKED));
	}
	HMENU parent;
	UINT pos;
	unique_ptr<Window::MenuItem::Logic> logic;
};

Window::Menu::Logic::~Logic()
{
}

struct MenuImpl : MenuChild, Window::Menu
{
	MenuImpl(HMENU h, unique_ptr<Window::Menu::Logic> l)
		: handle(h)
		, logic(move(l))
	{
		MENUINFO mi = {};
		mi.cbSize = sizeof(mi);
		mi.fMask = MIM_STYLE | MIM_MENUDATA;
		mi.dwStyle = MNS_NOTIFYBYPOS;
		mi.dwMenuData = (ULONG_PTR)this;
		SetMenuInfo(h, &mi);
	}
	~MenuImpl()
	{
		DestroyMenu(handle);
	}
	void update(bool open) override
	{
		if (!open)
			return;

		if (logic)
			logic->update(*this);

		for (auto& child : children)
		{
			if (child)
				child->update(false);
		}
	}
	void run(size_t index)
	{
		if (index >= 0 && index < children.size())
			children[index]->run();
		else
			VortexAssertf(false, "Bad menu index");
	}
	Menu& addMenu(stringref name, unique_ptr<Window::Menu::Logic> logic) override
	{
		auto child = make_shared<MenuImpl>(CreatePopupMenu(), move(logic));
		children.push_back(child);
		AppendMenuW(handle, MF_STRING | MF_POPUP, (UINT_PTR)child->handle, Unicode::widen(name).data());
		return *child;
	}
	Window::MenuItem& addItem(stringref name, unique_ptr<Window::MenuItem::Logic> logic) override
	{
		AppendMenuW(handle, MF_STRING, 0, Unicode::widen(name).data());
		auto child = make_shared<MenuItemImpl>(handle, (UINT)children.size(), move(logic));
		children.push_back(child);
		return *child;
	}
	void addSeparator() override
	{
		children.emplace_back();
		AppendMenuW(handle, MF_SEPARATOR, 0, nullptr);
	}
	void clear() override
	{
		for (int i = GetMenuItemCount(handle) - 1; i >= 0; --i)
			DeleteMenu(handle, i, MF_BYPOSITION);
		children.clear();
	}
	HMENU handle;
	unique_ptr<Window::Menu::Logic> logic;
	vector<shared_ptr<MenuChild>> children;
};


// =====================================================================================================================
// Initialization

LRESULT CALLBACK GlobalProc(HWND, UINT, WPARAM, LPARAM);

static bool CheckErr(bool result, const char* description)
{
	if (result)
	{
		Log::info(format("{} :: OK", description));
	}
	else
	{
		DWORD code = GetLastError();
		Log::blockBegin(description);
		Log::error(format("Windows error code: {}", code));
		Log::blockEnd();
	}
	return !result;
}

void Window::initialize()
{
	state = new State();

	// Initialize the keymap, which maps windows virtual keys to vortex key codes.
	memset(state->keyMap, 0, sizeof(state->keyMap));
	auto k = size(KeyPairs);
	for (size_t i = 0; i < k; ++i) state->keyMap[KeyPairs[i].raw] = KeyPairs[i].key;
	for (int i = 0; i < 26; ++i) state->keyMap['A' + i] = (KeyCode)((int)KeyCode::A + i);
	for (int i = 0; i < 10; ++i) state->keyMap['0' + i] = (KeyCode)((int)KeyCode::Digit0 + i);
	for (int i = 0; i < 15; ++i) state->keyMap[VK_F1 + i] = (KeyCode)((int)KeyCode::F1 + i);
	for (int i = 0; i < 9; ++i) state->keyMap[VK_NUMPAD0 + i] = (KeyCode)((int)KeyCode::Numpad0 + i);

	// Register the window class.
	WNDCLASSEX wcex = {};
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wcex.lpfnWndProc = GlobalProc;
	wcex.hInstance = state->hinstance;
	wcex.hIcon = LoadIcon(state->hinstance, MAKEINTRESOURCE(MAIN_ICON));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.lpszClassName = WindowClassName;

	ATOM classAtom = RegisterClassExW(&wcex);
	if (CheckErr(classAtom != 0, "Register window class")) return;

	// Create a window handle.
	state->style = WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_OVERLAPPEDWINDOW;
	state->exStyle = WS_EX_WINDOWEDGE | WS_EX_ACCEPTFILES | WS_EX_APPWINDOW;
	state->hwnd = CreateWindowExW(state->exStyle, WindowClassName, L"ArrowVortex", state->style,
		CW_USEDEFAULT, CW_USEDEFAULT, 640, 480, nullptr, nullptr, state->hinstance, state);
	if (CheckErr(state->hwnd != 0, "Create window")) return;

	// Center the window on the desktop.
	int width = state->windowSize.w;
	int height = state->windowSize.h;
	RECT wr;
	wr.left = max(0, GetSystemMetrics(SM_CXSCREEN) / 2 - width / 2);
	wr.top = max(0, GetSystemMetrics(SM_CYSCREEN) / 2 - height / 2);
	wr.right = wr.left + max(width, 0);
	wr.bottom = wr.top + max(height, 0);
	AdjustWindowRectEx(&wr, state->style, FALSE, state->exStyle);
	int flags = SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOZORDER;
	SetWindowPos(state->hwnd, nullptr, wr.left, wr.top, wr.right - wr.left, wr.bottom - wr.top, flags);

	// Initialize Vulkan.
	Vulkan::initialize(state->hinstance, state->hwnd);

	// Create the root menu so the application can add items to it.
	state->rootMenu = make_unique<MenuImpl>(CreateMenu(), nullptr);
}

void Window::deinitialize()
{
	// Deinitialize Vulkan.
	Vulkan::deinitialize();

	// Destroy the window.
	if (state->hwnd) DestroyWindow(state->hwnd);

	// Deregister the window class.
	UnregisterClassW(WindowClassName, state->hinstance);

	Util::reset(state);
}

// =====================================================================================================================
// Window management.

void Window::show()
{
	SetMenu(state->hwnd, state->rootMenu->handle);
	ShowWindow(state->hwnd, SW_SHOW);
	SetFocus(state->hwnd);
	state->isActive = true;
}

// =====================================================================================================================
// Message handling.

#define GET_LPX(lp) ((INT)(SHORT)LOWORD(lp))
#define GET_LPY(lp) ((INT)(SHORT)HIWORD(lp))

static void UpdateCursor()
{
	static LPCWSTR cursorMap[(int)Window::CursorIcon::Max] =
	{
		IDC_ARROW, IDC_HAND, IDC_IBEAM, IDC_SIZEALL, IDC_SIZEWE, IDC_SIZENS, IDC_SIZENESW, IDC_SIZENWSE,
	};
	auto resource = cursorMap[(int)state->cursor];
	auto cursor = LoadCursorW(nullptr, resource);
	if (cursor) SetCursor(cursor);
}

static KeyCode TranslateKeyCode(WPARAM vkCode, LPARAM flags)
{
	static const UINT lshift = MapVirtualKey(VK_LSHIFT, MAPVK_VK_TO_VSC);

	if (vkCode == VK_SHIFT)
		return (((flags & 0xFF0000) >> 16) == lshift) ? KeyCode::ShiftL : KeyCode::ShiftR;

	if (vkCode == VK_MENU)
		return ((HIWORD(flags) & KF_EXTENDED) ? KeyCode::AltR : KeyCode::AltL);

	if (vkCode == VK_CONTROL)
		return ((HIWORD(flags) & KF_EXTENDED) ? KeyCode::CtrlR : KeyCode::CtrlL);

	if (vkCode >= 0 && vkCode < 256)
		return state->keyMap[vkCode];

	return KeyCode::None;
}

static bool HandleMessage(UINT msg, WPARAM wp, LPARAM lp, LRESULT& result)
{
	static const MouseButton MouseCodes[4] =
	{
		MouseButton::None,
		MouseButton::LMB,
		MouseButton::MMB,
		MouseButton::RMB,
	};

	int mc = 0;
	switch (msg)
	{
	case WM_CLOSE:
	{
		EventSystem::publish<System::CloseRequested>();
		result = 0;
		return true;
	}
	case WM_PAINT:
	{
		ValidateRect(state->hwnd, NULL);
		return true;
	}
	case WM_ACTIVATE:
	{
		int active = LOWORD(wp), minimized = HIWORD(wp);
		if (active == WA_ACTIVE && minimized) break; // Ignore minimize messages.
		state->isActive = (active != WA_INACTIVE);
		if (!state->isActive)
		{
			state->mouse.clear();
			state->keys.clear();
		}
		break;
	}
	case WM_GETMINMAXINFO:
	{
		Size2 minSize = { 256, 256 }, maxSize = { 0, 0 };
		MINMAXINFO* mm = (MINMAXINFO*)lp;
		if (minSize.w > 0 && minSize.h > 0)
		{
			RECT r = { 0, 0, minSize.w, minSize.h };
			AdjustWindowRectEx(&r, state->style, FALSE, state->exStyle);
			mm->ptMinTrackSize.x = r.right - r.left;
			mm->ptMinTrackSize.y = r.bottom - r.top;
		}
		if (maxSize.w > 0 && maxSize.h > 0)
		{
			RECT r = { 0, 0, maxSize.w, maxSize.h };
			AdjustWindowRectEx(&r, state->style, FALSE, state->exStyle);
			mm->ptMaxTrackSize.x = r.right - r.left;
			mm->ptMaxTrackSize.y = r.bottom - r.top;
		}
		break;
	}
	case WM_SIZE:
	{
		int w = LOWORD(lp), h = HIWORD(lp);
		if (w > 0 && h > 0)
		{
			state->windowSize = { w, h };
			state->isMinimized = false;
		}
		else
		{
			state->isMinimized = true;
		}
		break;
	}
	case WM_MOUSEMOVE:
	{
		int x = GET_LPX(lp), y = GET_LPY(lp);
		state->mousePos = { x, y };
		break;
	}
	case WM_MOUSEWHEEL:
	{
		POINT pos = { GET_LPX(lp), GET_LPY(lp) };
		ScreenToClient(state->hwnd, &pos);
		MouseScroll scroll;
		scroll.isUp = GET_WHEEL_DELTA_WPARAM(wp) > 0;
		scroll.pos = { pos.x, pos.y };
		scroll.modifiers = Window::getKeyFlags();
		UiMan::send(scroll);
		break;
	}
	case WM_SYSKEYDOWN:
	case WM_KEYDOWN:
	{
		KeyPress press;
		press.key.code = TranslateKeyCode(wp, lp);
		press.key.modifiers = Window::getKeyFlags();
		press.isRepeated = bool(lp & (1 << 30));
		state->keys.insert(press.key.code);
		UiMan::send(press);
		if (press.key.code == KeyCode::AltL || press.key.code == KeyCode::AltR)
		{
			result = 0;
			return true;
		}
		break;
	}
	case WM_SYSKEYUP:
	case WM_KEYUP:
	{
		KeyRelease release;
		release.key.code = TranslateKeyCode(wp, lp);
		release.key.modifiers = Window::getKeyFlags();
		state->keys.erase(release.key.code);
		UiMan::send(release);
		break;
	}
	case WM_RBUTTONDOWN: ++mc; [[fallthrough]];
	case WM_MBUTTONDOWN: ++mc; [[fallthrough]];
	case WM_LBUTTONDOWN: ++mc;
	{
		SetCapture(state->hwnd);
		MousePress press;
		press.button = MouseCodes[mc];
		press.isDoubleClick = false;
		press.modifiers = Window::getKeyFlags();
		press.pos = { GET_LPX(lp), GET_LPY(lp) };
		state->mouse.insert(press.button);
		UiMan::send(press);
		break;
	}
	case WM_RBUTTONDBLCLK: ++mc; [[fallthrough]];
	case WM_MBUTTONDBLCLK: ++mc; [[fallthrough]];
	case WM_LBUTTONDBLCLK: ++mc;
	{
		SetCapture(state->hwnd);
		MousePress press;
		press.button = MouseCodes[mc];
		press.isDoubleClick = true;
		press.modifiers = Window::getKeyFlags();
		press.pos = { GET_LPX(lp), GET_LPY(lp) };
		UiMan::send(press);
		break;
	}
	case WM_RBUTTONUP: ++mc; [[fallthrough]];
	case WM_MBUTTONUP: ++mc; [[fallthrough]];
	case WM_LBUTTONUP: ++mc;
	{
		ReleaseCapture();
		MouseRelease release;
		release.button = MouseCodes[mc];
		release.modifiers = Window::getKeyFlags();
		release.pos = { GET_LPX(lp), GET_LPY(lp) };
		state->mouse.erase(release.button);
		UiMan::send(release);
		break;
	}
	case WM_MENUCHAR:
	{
		// Removes beep sound for unused alt+key accelerators.
		result = MNC_CLOSE << 16;
		return true;
	}
	case WM_CHAR:
	{
		if ((wp >= 0x20 && wp <= 0x7e) || wp > 0xa0)
			state->textInput.push_back((wchar_t)wp);
		break;
	}
	case WM_DROPFILES:
	{
		POINT pos;
		DragQueryPoint((HDROP)wp, &pos);
		// Passing 0xFFFFFFFF returns the file count.
		UINT numFiles = DragQueryFileW((HDROP)wp, 0xFFFFFFFF, 0, 0);
		System::FileDropped drop;
		for (UINT i = 0; i < numFiles; ++i)
		{
			// Passing null as string buffer returns path size without null byte.
			UINT len = DragQueryFileW((HDROP)wp, i, nullptr, 0);
			wstring wstr(len, 0);
			DragQueryFileW((HDROP)wp, i, (LPWSTR)wstr.data(), len + 1);
			drop.paths.emplace_back(Unicode::narrow(wstr.data()));
		}
		DragFinish((HDROP)wp);
		EventSystem::publish(drop);
		break;
	}
	case WM_SETCURSOR:
	{
		if (LOWORD(lp) == HTCLIENT)
		{
			UpdateCursor();
			result = TRUE;
			return true;
		}
		break;
	}
	case WM_MENUCOMMAND:
	{
		MENUINFO mi = {};
		mi.cbSize = sizeof(MENUINFO);
		mi.fMask = MIM_MENUDATA;
		if (GetMenuInfo((HMENU)lp, &mi))
			((MenuImpl*)mi.dwMenuData)->run((size_t)wp);
		break;
	}
	case WM_INITMENUPOPUP:
	{
		MENUINFO mi = {};
		mi.cbSize = sizeof(MENUINFO);
		mi.fMask = MIM_MENUDATA;
		if (GetMenuInfo((HMENU)wp, &mi))
			((MenuImpl*)mi.dwMenuData)->update(true);
		break;
	}
	}; // end of message switch.

	return false;
}

static LRESULT CALLBACK GlobalProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	LRESULT res = 0;
	bool handled = false;
	if (msg == WM_CREATE)
	{
		LPVOID userdata = ((LPCREATESTRUCT)lp)->lpCreateParams;
		SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)userdata);
	}
	else
	{
		if (state) handled = HandleMessage(msg, wp, lp, res);
	}
	return handled ? res : DefWindowProcW(hwnd, msg, wp, lp);
}

void Window::handleMessages()
{
	// Process messages.
	MSG message;
	while (PeekMessage(&message, nullptr, 0, 0, PM_NOREMOVE))
	{
		GetMessageW(&message, nullptr, 0, 0);
		TranslateMessage(&message);
		DispatchMessage(&message);
	}

	// Combine multiple text input messages into a single event.
	if (state->textInput.length())
	{
		TextInput textInput;
		textInput.text = Unicode::narrow(state->textInput);
		UiMan::send(textInput);
		state->textInput.clear();
	}

	// Update delta startTime.
	double time = System::getElapsedTime();
	state->deltaTime = min(max(0.0001, time - state->previousFrameTime), 0.25);
	state->previousFrameTime = time;
}

// =====================================================================================================================
// Window utilities.

Window::Menu& Window::addMenu(stringref name)
{
	return state->rootMenu->addMenu(name, nullptr);
}

void Window::setTitle(stringref text)
{
	if (state->title == text) return;

	SetWindowTextW(state->hwnd, Unicode::widen(text).data());
	state->title = text;
}

void Window::setCursor(CursorIcon cursor)
{
	if (state->cursor == cursor) return;

	state->cursor = cursor;
	UpdateCursor();
}

bool Window::isMinimized()
{
	return state->isMinimized;
}

Size2 Window::getSize()
{
	return state->windowSize;
}

double Window::getDeltaTime()
{
	return state->deltaTime;
}

bool Window::isAnyKeyDown()
{
	return !state->keys.empty();
}

bool Window::isKeyDown(KeyCode key)
{
	return state->keys.find(key) != state->keys.end();
}

bool Window::isMouseDown(MouseButton button)
{
	return state->mouse.find(button) != state->mouse.end();
}

Vec2 Window::getMousePos()
{
	return state->mousePos;
}

ModifierKeys Window::getKeyFlags()
{
	ModifierKeys flags = ModifierKeys::None;

	if (isKeyDown(KeyCode::CtrlL) || isKeyDown(KeyCode::CtrlR))
		flags |= ModifierKeys::Ctrl;

	if (isKeyDown(KeyCode::AltL) || isKeyDown(KeyCode::AltR))
		flags |= ModifierKeys::Alt;

	if (isKeyDown(KeyCode::ShiftL) || isKeyDown(KeyCode::ShiftR))
		flags |= ModifierKeys::Shift;

	return flags;
}

// =====================================================================================================================
// Dialog boxes.

// Translates a dialog button type to a windows message box type. 
static const int DlgType[(int)Window::Button::Count] = { MB_OK, MB_OKCANCEL, MB_YESNO, MB_YESNOCANCEL };

// Translates a dialog icon type to a windows message box icon.
static const int DlgIcon[(int)Window::Icon::Count] = { 0, MB_ICONASTERISK, MB_ICONWARNING, MB_ICONHAND };

// Shows an open/save message box and returns the path selected by the user.
static FilePath ShowFileDialog(string title, const FilePath& path, string filters, int& index, bool save)
{
	wstring wfilter = Unicode::widen(filters);
	wstring wtitle = Unicode::widen(title);

	// Split the input path into a directory and filename.
	wstring wdir = Unicode::widen(path.directory().str);
	wstring wfile = Unicode::widen(path.filename());

	// Write the input filename to the output path buffer.
	wchar_t outPath[MAX_PATH + 1] = {};
	if (wfile.length() && wfile.length() <= MAX_PATH)
		memcpy(outPath, wfile.data(), sizeof(wchar_t) * wfile.length());

	// Prepare the open/save file dialog.
	OPENFILENAMEW ofns = { sizeof(OPENFILENAMEW) };
	ofns.lpstrFilter = wfilter.data();
	ofns.hwndOwner = state->hwnd;
	ofns.lpstrFile = outPath;
	ofns.nMaxFile = MAX_PATH;
	ofns.lpstrTitle = wtitle.data();
	if (wdir.length()) ofns.lpstrInitialDir = wdir.data();
	ofns.Flags = save ? 0 : OFN_FILEMUSTEXIST;
	ofns.nFilterIndex = index;

	BOOL res = save ? GetSaveFileNameW(&ofns) : GetOpenFileNameW(&ofns);
	index = ofns.nFilterIndex;
	if (res == 0) outPath[0] = 0;

	auto exeDir = System::getExeDir().str;
	filesystem::current_path(Unicode::widen(exeDir));

	return FilePath(Unicode::narrow(outPath));
}

static DirectoryPath ShowDirectoryDialog(string title, const DirectoryPath& path)
{
	wstring wtitle = Unicode::widen(title);
	wstring wpath = Unicode::widen(path.str);

	IFileOpenDialog* pFileOpen = nullptr;
	PWSTR pszFilePath = nullptr;
	HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
		IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));
	if (SUCCEEDED(hr))
	{
		FILEOPENDIALOGOPTIONS opt = 0;
		pFileOpen->GetOptions(&opt);
		pFileOpen->SetOptions(opt | FOS_PICKFOLDERS);
		pFileOpen->SetTitle(wtitle.data());
		hr = pFileOpen->Show(state->hwnd);
		if (SUCCEEDED(hr))
		{
			IShellItem* pItem;
			hr = pFileOpen->GetResult(&pItem);
			if (SUCCEEDED(hr))
			{
				hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
				pItem->Release();
			}
		}
		pFileOpen->Release();
	}

	string result;
	if (pszFilePath)
		result = Unicode::narrow(pszFilePath);

	return DirectoryPath(result);
}

Window::Result Window::showMessageDlg(stringref title, stringref text, Window::Button b, Window::Icon i)
{
	auto wtitle = Unicode::widen(title);
	auto wtext = Unicode::widen(text);
	int flags = DlgType[(int)b] | DlgIcon[(int)i];
	switch (MessageBoxW(state->hwnd, wtext.data(), wtitle.data(), flags))
	{
	case IDOK:
		return Window::Result::Ok;
	case IDYES:
		return Window::Result::Yes;
	case IDNO:
		return Window::Result::No;
	};
	return Window::Result::Cancel;
}

FilePath Window::openFileDlg(stringref title, int& filterIndex, FilePath initial, stringref filters)
{
	return ShowFileDialog(title, initial, filters, filterIndex, false);
}

FilePath Window::saveFileDlg(stringref title, int& filterIndex, FilePath initial, stringref filters)
{
	return ShowFileDialog(title, initial, filters, filterIndex, true);
}

DirectoryPath Window::openDirDlg(stringref title, DirectoryPath initial)
{
	return ShowDirectoryDialog(title, initial);
}

} // namespace AV