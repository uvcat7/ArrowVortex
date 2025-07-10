#ifndef NDEBUG
#define CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

#include <System/System.h>
#include <System/Resources.h>
#include <System/File.h>
#include <System/Debug.h>

#include <Core/String.h>
#include <Core/WideString.h>
#include <Core/StringUtils.h>
#include <Core/Shader.h>

#include <Editor/Editor.h>
#include <Editor/Menubar.h>

#define UNICODE

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winuser.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <commdlg.h>
#include <GL/gl.h>
#undef ERROR

#include <algorithm>
#include <chrono>
#include <thread>
#include <numeric>
#include <stdio.h>
#include <ctime>
#include <bitset>
#include <list>
#include <vector>

#undef DELETE

// Enable visual styles.
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

namespace Vortex {

std::chrono::duration<double> deltaTime; // Defined in <Core/Core.h>

namespace {

static wchar_t sRunDir[MAX_PATH + 1] = {};
static wchar_t sExeDir[MAX_PATH + 1] = {};

// Swap interval extension for enabling/disabling vsync.
typedef BOOL(APIENTRY *PFNWGLSWAPINTERVALFARPROC)(int);
static PFNWGLSWAPINTERVALFARPROC wglSwapInterval;

// Mapping of windows virtual keys to vortex key codes.
static const int VKtoKCmap[] =
{
	VK_OEM_3, Key::ACCENT,
	VK_OEM_MINUS, Key::DASH,
	VK_OEM_PLUS, Key::EQUAL,
	VK_OEM_4, Key::BRACKET_L,
	VK_OEM_6, Key::BRACKET_R,
	VK_OEM_1, Key::SEMICOLON,
	VK_OEM_7, Key::QUOTE,
	VK_OEM_5, Key::BACKSLASH,
	VK_OEM_COMMA, Key::COMMA,
	VK_OEM_PERIOD, Key::PERIOD,
	VK_OEM_2, Key::SLASH,
	VK_SPACE, Key::SPACE,
	VK_ESCAPE, Key::ESCAPE,
	VK_LWIN, Key::SYSTEM_L,
	VK_RWIN, Key::SYSTEM_R,
	VK_TAB, Key::TAB,
	VK_CAPITAL, Key::CAPS,
	VK_RETURN, Key::RETURN,
	VK_BACK, Key::BACKSPACE,
	VK_PRIOR, Key::PAGE_UP,
	VK_NEXT, Key::PAGE_DOWN,
	VK_HOME, Key::HOME,
	VK_END, Key::END,
	VK_INSERT, Key::INSERT,
	VK_DELETE, Key::DELETE,
	VK_SNAPSHOT, Key::PRINT_SCREEN,
	VK_SCROLL, Key::SCROLL_LOCK,
	VK_PAUSE, Key::PAUSE,
	VK_LEFT, Key::LEFT,
	VK_RIGHT, Key::RIGHT,
	VK_UP, Key::UP,
	VK_DOWN, Key::DOWN,
	VK_NUMLOCK, Key::NUM_LOCK,
	VK_DIVIDE, Key::NUMPAD_DIVIDE,
	VK_MULTIPLY, Key::NUMPAD_MULTIPLY,
	VK_SUBTRACT, Key::NUMPAD_SUBTRACT,
	VK_ADD, Key::NUMPAD_ADD,
	VK_SEPARATOR, Key::NUMPAD_SEPERATOR,
};

// Translates a dialog button type to a windows message box type. 
static int sDlgType[System::NUM_BUTTONS] = {MB_OK, MB_OKCANCEL, MB_YESNO, MB_YESNOCANCEL};

// Translates a dialog icon type to a windows message box icon.
static int sDlgIcon[System::NUM_ICONS] = {0, MB_ICONASTERISK, MB_ICONWARNING, MB_ICONHAND};

// Shows an open/save message box and returns the path selected by the user.
static String ShowFileDialog(String title, String path, String filters, int* index, bool save)
{
	WideString wfilter = Widen(filters);
	WideString wtitle = Widen(title);

	// Split the input path into a directory and filename.
	Path fpath = path;
	WideString wdir = Widen(fpath.dirWithoutSlash());
	WideString wfile = Widen(fpath.filename());

	// Write the input filename to the output path buffer.
	wchar_t outPath[MAX_PATH + 1] = {};
	if(wfile.size() && wfile.length() <= MAX_PATH)
		memcpy(outPath, wfile.str(), sizeof(wchar_t) * wfile.length());

	// Prepare the open/save file dialog.
	OPENFILENAMEW ofns = {sizeof(OPENFILENAMEW)};
	ofns.lpstrFilter = wfilter.str();
	ofns.hwndOwner = (HWND)gSystem->getHWND();
	ofns.lpstrFile = outPath;
	ofns.nMaxFile = MAX_PATH;
	ofns.lpstrTitle = wtitle.str();
	if(wdir.size()) ofns.lpstrInitialDir = wdir.str();
	ofns.Flags = save ? 0 : OFN_FILEMUSTEXIST;
	ofns.nFilterIndex = index ? *index : 0;

	BOOL res = save ? GetSaveFileNameW(&ofns) : GetOpenFileNameW(&ofns);
	if(index) *index = ofns.nFilterIndex;
	if(res == 0) outPath[0] = 0;
	gSystem->setWorkingDir(gSystem->getExeDir());

	return Narrow(outPath, wcslen(outPath));
}

// ================================================================================================
// SystemImpl :: Debug logging.

static bool LogCheckpoint(bool result, const char* description)
{
	if(result)
	{
		Debug::log("%s :: OK\n", description);
	}
	else
	{
		char lpMsgBuf[100];
		DWORD code = GetLastError();
		FormatMessageA(
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			code,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			lpMsgBuf,
			60, 
			NULL);
		Debug::blockBegin(Debug::ERROR, description);
		Debug::log("windows error code %i: %s", code, lpMsgBuf);
		Debug::blockEnd();
	}
	return !result;
}

// ================================================================================================
// SystemImpl :: menu item.

}; // anonymous namespace

typedef System::MenuItem MItem;

MItem* MItem::create()
{
	return (MenuItem*)CreatePopupMenu();
}

void MItem::addSeperator()
{
	AppendMenuW((HMENU)this, MF_SEPARATOR, 0, nullptr);
}
void MItem::addItem(int item, StringRef text)
{
	AppendMenuW((HMENU)this, MF_STRING, item, Widen(text).str());
}

void MItem::addSubmenu(MItem* submenu, StringRef text, bool grayed)
{
	int flags = MF_STRING | MF_POPUP | (grayed * MF_GRAYED);
	AppendMenuW((HMENU)this, MF_STRING | MF_POPUP, reinterpret_cast<UINT_PTR>(submenu), Widen(text).str());
}

void MItem::replaceSubmenu(int pos, MItem* submenu, StringRef text, bool grayed)
{
	int flags = MF_BYPOSITION | MF_STRING | MF_POPUP | (grayed * MF_GRAYED);
	DeleteMenu((HMENU)this, pos, MF_BYPOSITION);
	InsertMenuW((HMENU)this, pos, flags, reinterpret_cast<UINT_PTR>(submenu), Widen(text).str());
}

void MItem::setChecked(int item, bool state)
{
	CheckMenuItem((HMENU)this, item, state ? MF_CHECKED : MF_UNCHECKED);
}

void MItem::setEnabled(int item, bool state)
{
	EnableMenuItem((HMENU)this, item, state ? MF_ENABLED : MF_GRAYED);
}

namespace {

// ================================================================================================
// SystemImpl :: member data.

struct SystemImpl : public System {

wchar_t* myClassName;
HINSTANCE myInstance;
std::chrono::steady_clock::time_point myApplicationStartTime;
Cursor::Icon myCursor;
Key::Code myKeyMap[256];
InputEvents myEvents;
vec2i myMousePos, mySize;
std::bitset<Key::MAX_VALUE> myKeyState;
std::bitset<Mouse::MAX_VALUE> myMouseState;
String myTitle;
WideString myInput;
DWORD myStyle, myExStyle;
HWND myHWND;
HDC myHDC;
HGLRC myHRC;
bool myIsActive;
bool myInitSuccesful;
bool myIsTerminated;
bool myIsInsideMessageLoop;

// ================================================================================================
// SystemImpl :: constructor and destructor.

~SystemImpl()
{
	// Destroy the rendering context.
	if(myHRC) wglDeleteContext(myHRC);

	// Destroy the window.
	if(myHWND) DestroyWindow(myHWND);

	// Deregister the window class.
	UnregisterClassW(myClassName, myInstance);
}

SystemImpl()
	: myInstance(GetModuleHandle(nullptr))
	, myClassName(L"ArrowVortex")
	, myCursor(Cursor::ARROW)
	, myMousePos({0, 0})
	, mySize({0, 0})
	, myTitle("ArrowVortex")
	, myIsActive(false)
	, myInitSuccesful(false)
	, myIsTerminated(false)
	, myIsInsideMessageLoop(false)
{
	myApplicationStartTime = Debug::getElapsedTime();

	// Register the window class.
	WNDCLASSW wndclass = {};
	wndclass.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wndclass.lpfnWndProc = GlobalProc;
	wndclass.hInstance = myInstance;
	wndclass.hIcon = LoadIcon(myInstance, MAKEINTRESOURCE(MAIN_ICON));
	wndclass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wndclass.lpszClassName = myClassName;
	
	ATOM classAtom = RegisterClassW(&wndclass);
	if(LogCheckpoint(classAtom != 0, "registering window class")) return;

	// Initialize the keymap, which maps windows virtual keys to vortex key codes.
	memset(myKeyMap, 0, sizeof(myKeyMap));
	int k = sizeof(VKtoKCmap) / sizeof(VKtoKCmap[0]);
	for(int i = 0; i < k; i += 2) myKeyMap[VKtoKCmap[i]] = (Key::Code)VKtoKCmap[i + 1];
	for(int i = 0; i < 26; ++i)	myKeyMap['A' + i] = (Key::Code)(Key::A + i);
	for(int i = 0; i < 10; ++i) myKeyMap['0' + i] = (Key::Code)(Key::DIGIT_0 + i);
	for(int i = 0; i < 15; ++i) myKeyMap[VK_F1 + i] = (Key::Code)(Key::F1 + i);
	for(int i = 0; i < 9; ++i) myKeyMap[VK_NUMPAD0 + i] = (Key::Code)(Key::NUMPAD_0 + i);

	// Create a window handle.
	myStyle = WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_OVERLAPPEDWINDOW;
	myExStyle = WS_EX_WINDOWEDGE | WS_EX_ACCEPTFILES | WS_EX_APPWINDOW;
	myHWND = CreateWindowExW(myExStyle, myClassName, L"ArrowVortex", myStyle,
		CW_USEDEFAULT, CW_USEDEFAULT, 640, 480, nullptr, nullptr, myInstance, this);

	if(LogCheckpoint(myHWND != 0, "creating window")) return;

	// Create a device context.
	myHDC = GetDC(myHWND);
	if(LogCheckpoint(myHDC != 0, "creating device context")) return;

	// Create the pixel format descriptor.
	PIXELFORMATDESCRIPTOR pfd;
	memset(&pfd, 0, sizeof(pfd));
	pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 32;
	pfd.cRedBits = 8;
	pfd.cGreenBits = 8;
	pfd.cBlueBits = 8;
	pfd.cAlphaBits = 8;
	pfd.cDepthBits = 24;
	pfd.cStencilBits = 8;

	// Set the pixel format.
	int cpf = ChoosePixelFormat(myHDC, &pfd);
	if(LogCheckpoint(cpf != 0, "choosing pixel format")) return;

	BOOL spf = SetPixelFormat(myHDC, cpf, &pfd);
	if(LogCheckpoint(spf != 0, "setting pixel format")) return;

	// Create the OpenGL rendering context.
	myHRC = wglCreateContext(myHDC);
	if(LogCheckpoint(myHRC != 0, "creating OpenGL context")) return;

	BOOL mc = wglMakeCurrent(myHDC, myHRC);
	if(LogCheckpoint(mc != 0, "activating OpenGL context")) return;

	VortexCheckGlError();

	// Initialize the OpenGL settings.
	glClearColor(0, 0, 0, 1);
	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnableClientState(GL_VERTEX_ARRAY);

	VortexCheckGlError();

	// Enable vsync for now, we will disable it later if the settings require it.
	wglSwapInterval = (PFNWGLSWAPINTERVALFARPROC)wglGetProcAddress("wglSwapIntervalEXT");
	Debug::log("swap interval support :: %s\n", wglSwapInterval ? "OK" : "MISSING");
	if(wglSwapInterval)
	{
		wglSwapInterval(-1);
		VortexCheckGlError();
	}

	// Check for shader support.
	Shader::initExtension();
	Debug::logBlankLine();

	// Make sure the window is centered on the desktop.
	mySize = {900, 900};
	RECT wr;
	wr.left = std::max(0, GetSystemMetrics(SM_CXSCREEN) / 2 - mySize.x / 2);
	wr.top = std::max(0, GetSystemMetrics(SM_CYSCREEN) / 2 - mySize.y / 2);
	wr.right = wr.left + std::max(mySize.x, 0);
	wr.bottom = wr.top + std::max(mySize.y, 0);
	AdjustWindowRectEx(&wr, myStyle, FALSE, myExStyle);
	int flags = SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOZORDER;
	SetWindowPos(myHWND, nullptr, wr.left, wr.top, wr.right - wr.left, wr.bottom - wr.top, flags);

	// Show the window.
	ShowWindow(myHWND, SW_SHOW);
	SetFocus(myHWND);
	myIsActive = true;
	myInitSuccesful = true;
}

// ================================================================================================
// SystemImpl :: message loop.

void forwardArgs()
{
	int numArgs = 0;
	LPWSTR* wideArgs = CommandLineToArgvW(GetCommandLineW(), &numArgs);
	Vector<String> args(numArgs, String());
	for(int i = 0; i < numArgs; ++i)
	{
		args[i] = Narrow(wideArgs[i], wcslen(wideArgs[i]));
	}
	gEditor->onCommandLineArgs(args.data(), args.size());
	LocalFree(wideArgs);
}

void createMenu()
{
	HMENU menu = CreateMenu();
	gMenubar->init((MenuItem*)menu);
	SetMenu(myHWND, menu);
}

void CALLBACK messageLoop()
{
	using namespace std::chrono;

	if(!myInitSuccesful) return;

#ifdef DEBUG
	long long frames = 0;
	auto lowcounts = 0;
	std::list<double> fpsList, sleepList, frameList, inputList, waitList;
	// Adjust frameGuess to your VSync target if you are testing with VSync enabled
	auto frameGuess = 960;
#endif

	Editor::create();
	forwardArgs();
	createMenu();

	// Non-vsync FPS max target
	auto frameTarget = duration<double>(1.0 / 960.0);

	// Enter the message loop.
	MSG message;
	auto prevTime = Debug::getElapsedTime();
	while(!myIsTerminated)
	{

		auto startTime = Debug::getElapsedTime();

		myEvents.clear();
		// Process all windows messages.
		myIsInsideMessageLoop = true;
		while (PeekMessage(&message, nullptr, 0, 0, PM_NOREMOVE | PM_NOYIELD))
		{
			GetMessageW(&message, nullptr, 0, 0);
			TranslateMessage(&message);
			DispatchMessage(&message);
		}
		myIsInsideMessageLoop = false;

		// Check if there were text input events.
		if (!myInput.empty())
		{
			myEvents.addTextInput(Narrow(myInput).str());
			myInput.clear();
		}

		// Set up the OpenGL view.
		glViewport(0, 0, mySize.x, mySize.y);
		glLoadIdentity();
		glOrtho(0, mySize.x, mySize.y, 0, -1, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Reset the mouse cursor.
		myCursor = Cursor::ARROW;

#ifdef DEBUG
		auto inputTime = Debug::getElapsedTime();
#endif

		VortexCheckGlError();

		gEditor->tick();

		// Display.
		SwapBuffers(myHDC);

#ifdef DEBUG
		auto renderTime = Debug::getElapsedTime();
#endif
		// Tick function.
		duration<double> frameTime = Debug::getElapsedTime() - prevTime;
		auto waitTime = frameTarget.count() - frameTime.count();

		if (wglSwapInterval)
		{
			while (Debug::getElapsedTime() - prevTime < frameTarget)
			{
				std::this_thread::yield();
			}
		}

		// End of frame
		auto curTime = Debug::getElapsedTime();
		deltaTime = duration<double>(std::min(std::max(0.0, duration<double>(curTime - prevTime).count()), 0.25));
		prevTime = curTime;

#ifdef DEBUG
		// Do frame statistics
		// Note that these will be wrong with VSync enabled.
		fpsList.push_front(deltaTime.count());
		waitList.push_front(duration<double>(curTime - renderTime).count());
		frameList.push_front(duration<double>(renderTime - inputTime).count());
		inputList.push_front(duration<double>(inputTime - startTime).count());

		if (abs(deltaTime.count() - 1.0 / (double)frameGuess) / (1.0 / (double)frameGuess) > 0.01)
		{
			lowcounts++;
		}
		if (fpsList.size() >= frameGuess * 2)
		{
			fpsList.pop_back();
			frameList.pop_back();
			inputList.pop_back();
			waitList.pop_back();
		}
		auto min = *std::min_element(fpsList.begin(), fpsList.end());
		auto max = *std::max_element(fpsList.begin(), fpsList.end());
		auto maxIndex = std::distance(fpsList.begin(), std::max_element(fpsList.begin(), fpsList.end()));
		auto siz = fpsList.size();
		auto avg = std::accumulate(fpsList.begin(), fpsList.end(), 0.0) / siz;
		auto varianceFunc = [&avg, &siz](double accumulator, double val) {
			return accumulator + (val - avg) * (val - avg);
			};
		auto std = sqrt(std::accumulate(fpsList.begin(), fpsList.end(), 0.0, varianceFunc) / siz);
		auto frameAvg = std::accumulate(frameList.begin(), frameList.end(), 0.0) / frameList.size();
		auto frameMax = frameList.begin();
		std::advance(frameMax, maxIndex);
		auto inputMax = inputList.begin();
		std::advance(inputMax, maxIndex);
		auto waitMax = waitList.begin();
		std::advance(waitMax, maxIndex);
		if (frames % (frameGuess * 2) == 0)
		{
			Debug::log("frame total average: %f, frame render average %f, std dev %f, lowest FPS %f, highest FPS %f, highest FPS render time %f, highest FPS input time %f, highest FPS wait time %f, lag frames %d\n",
				avg,
				frameAvg,
				std,
				1.0 / max,
				1.0 / min,
				*frameMax,
				*inputMax,
				*waitMax,
				lowcounts);
			lowcounts = 0;
		}
		frames++;
#endif
	}
	Editor::destroy();
}

// ================================================================================================
// SystemImpl :: clipboard functions.

bool setClipboardText(StringRef text)
{
	bool result = false;
	if(OpenClipboard(nullptr))
	{
		EmptyClipboard();
		WideString wtext = Widen(text);
		size_t size = sizeof(wchar_t) * (wtext.length() + 1);		
		HGLOBAL bufferHandle = GlobalAlloc(GMEM_DDESHARE, size);
		char* buffer = (char*)GlobalLock(bufferHandle);
		if(buffer)
		{
			memcpy(buffer, wtext.str(), size);
			GlobalUnlock(bufferHandle);
			if(SetClipboardData(CF_UNICODETEXT, bufferHandle))
			{
				result = true;
			}
			else
			{
				GlobalFree(bufferHandle);
			}
		}
		CloseClipboard();
	}
	return result;
}

String getClipboardText() const
{
	String str;
	if(OpenClipboard(nullptr))
	{
		HANDLE hData = GetClipboardData(CF_UNICODETEXT);
		wchar_t* src = (wchar_t*)GlobalLock(hData);
		if(src)
		{
			str = Narrow(src, wcslen(src));
			GlobalUnlock(hData);
			Str::replace(str, "\n", "");
		}
		CloseClipboard();
	}
	return str;
}

// ================================================================================================
// SystemImpl :: message handling.

#define GET_LPX(lp) ((INT)(SHORT)LOWORD(lp))
#define GET_LPY(lp) ((INT)(SHORT)HIWORD(lp))

LPCWSTR getCursorResource()
{
	static LPCWSTR cursorMap[Cursor::NUM_CURSORS] =
	{
		IDC_ARROW, IDC_HAND, IDC_IBEAM, IDC_SIZEALL, IDC_SIZEWE, IDC_SIZENS, IDC_SIZENESW, IDC_SIZENWSE,
	};
	return cursorMap[std::min(std::max(0, static_cast<int>(myCursor)), Cursor::NUM_CURSORS - 1)];
}

int getKeyFlags() const
{
	int kc[6] = { Key::SHIFT_L, Key::SHIFT_R, Key::CTRL_L, Key::CTRL_R, Key::ALT_L, Key::ALT_R };
	int kf[6] = { Keyflag::SHIFT, Keyflag::SHIFT, Keyflag::CTRL, Keyflag::CTRL, Keyflag::ALT, Keyflag::ALT };

	int flags = 0;
	for(int i = 0; i < 6; ++i)
		if(myKeyState.test(kc[i])) flags |= kf[i];

	return flags;
}

Key::Code translateKeyCode(int vkCode, int flags)
{
	static const UINT lshift = MapVirtualKey(VK_LSHIFT, MAPVK_VK_TO_VSC);

	if(vkCode == VK_SHIFT)
		return (((flags & 0xFF0000) >> 16) == lshift) ? Key::SHIFT_L : Key::SHIFT_R;
	if(vkCode == VK_MENU)
		return ((HIWORD(flags) & KF_EXTENDED) ? Key::ALT_R : Key::ALT_L);
	if(vkCode == VK_CONTROL)
		return ((HIWORD(flags) & KF_EXTENDED) ? Key::CTRL_R : Key::CTRL_L);
	if(vkCode >= 0 && vkCode < 256)
		return myKeyMap[vkCode];

	return Key::NONE;
}

void handleKeyPress(Key::Code kc, bool repeated)
{
	bool handled = false;
	int kf = getKeyFlags();
	myEvents.addKeyPress(kc, kf, repeated);
	myKeyState.set(kc);
}

bool handleMsg(UINT msg, WPARAM wp, LPARAM lp, LRESULT& result)
{
	static const Mouse::Code mcodes[4] = {Mouse::NONE, Mouse::LMB, Mouse::MMB, Mouse::RMB};

	int mc = 0;
	switch(msg)
	{
	case WM_CLOSE:
	{
		gEditor->onExitProgram();
		result = 0;
		return true;
	}
	case WM_ACTIVATE:
	{
		int state = LOWORD(wp), minimized = HIWORD(wp);
		if(state == WA_ACTIVE && minimized) break; // Ignore minimize messages.
		myIsActive = (state != WA_INACTIVE);
		if(!myIsActive) myEvents.addWindowInactive();
		myMouseState.reset();
		myKeyState.reset();
		break;
	}
	case WM_GETMINMAXINFO:
	{
		vec2i minSize = { 256, 256 }, maxSize = { 0, 0 };
		MINMAXINFO* mm = (MINMAXINFO*)lp;
		if(minSize.x > 0 && minSize.y > 0)
		{
			RECT r = { 0, 0, minSize.x, minSize.y };
			AdjustWindowRectEx(&r, myStyle, FALSE, myExStyle);
			mm->ptMinTrackSize.x = r.right - r.left;
			mm->ptMinTrackSize.y = r.bottom - r.top;
		}
		if(maxSize.x > 0 && maxSize.y > 0)
		{
			RECT r = { 0, 0, maxSize.x, maxSize.y };
			AdjustWindowRectEx(&r, myStyle, FALSE, myExStyle);
			mm->ptMaxTrackSize.x = r.right - r.left;
			mm->ptMaxTrackSize.y = r.bottom - r.top;
		}
		break;
	}
	case WM_SIZE:
	{
		vec2i next = { LOWORD(lp), HIWORD(lp) };
		if(next.x > 0 && next.y > 0) mySize = next;
		break;
	}
	case WM_MOUSEMOVE:
	{
		if(myIsInsideMessageLoop)
		{
			myMousePos.x = GET_LPX(lp);
			myMousePos.y = GET_LPY(lp);
			myEvents.addMouseMove(myMousePos.x, myMousePos.y);
		}
		break;
	}
	case WM_MOUSEWHEEL:
	{
		if(myIsInsideMessageLoop)
		{
			POINT pos = {GET_LPX(lp), GET_LPY(lp)};
			ScreenToClient(myHWND, &pos);
			bool up = GET_WHEEL_DELTA_WPARAM(wp) > 0;
			myEvents.addMouseScroll(up, pos.x, pos.y, getKeyFlags());
		}
		break;
	}
	case WM_SYSKEYDOWN:
	case WM_KEYDOWN:
	{
		if(myIsInsideMessageLoop)
		{
			int prev = lp & (1 << 30);
			Key::Code kc = translateKeyCode(wp, lp);
			handleKeyPress(kc, prev != 0);
			if(kc == Key::ALT_L || kc == Key::ALT_R)
			{
				result = 0;
				return true;
			}
		}
		break;
	}
	case WM_SYSKEYUP:
	case WM_KEYUP:
	{
		if(myIsInsideMessageLoop)
		{
			Key::Code kc = translateKeyCode(wp, lp);
			myEvents.addKeyRelease(kc, getKeyFlags());
			myKeyState.reset(kc);
			break;
		}
	}
	case WM_RBUTTONDOWN: ++mc;
	case WM_MBUTTONDOWN: ++mc;
	case WM_LBUTTONDOWN: ++mc;
	{
		SetCapture(myHWND);
		if(myIsInsideMessageLoop)
		{
			int x = GET_LPX(lp), y = GET_LPY(lp);
			myEvents.addMousePress(mcodes[mc], x, y, getKeyFlags(), false);
			myMouseState.set(mcodes[mc]);
		}
		break;
	}
	case WM_RBUTTONDBLCLK: ++mc;
	case WM_MBUTTONDBLCLK: ++mc;
	case WM_LBUTTONDBLCLK: ++mc;
	{
		SetCapture(myHWND);
		if(myIsInsideMessageLoop)
		{
			int x = GET_LPX(lp), y = GET_LPY(lp);
			myEvents.addMousePress(mcodes[mc], x, y, getKeyFlags(), true);
			myMouseState.set(mcodes[mc]);
		}
		break;
	}
	case WM_RBUTTONUP: ++mc;
	case WM_MBUTTONUP: ++mc;
	case WM_LBUTTONUP: ++mc;
	{
		ReleaseCapture();
		if(myIsInsideMessageLoop)
		{
			int x = GET_LPX(lp), y = GET_LPY(lp);
			myEvents.addMouseRelease(mcodes[mc], x, y, getKeyFlags());
			myMouseState.reset(mcodes[mc]);
		}
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
		if(wp >= 32) myInput.push_back(wp);
		else if(wp == '\r') myInput.push_back('\n');
		break;
	}
	case WM_DROPFILES:
	{
		if(myIsInsideMessageLoop)
		{
			POINT pos;
			DragQueryPoint((HDROP)wp, &pos);

			// Get the number of files dropped.
			UINT numFiles = DragQueryFileW((HDROP)wp, 0xFFFFFFFF, nullptr, 0);
			std::vector<String> files(numFiles);

			for (UINT i = 0; i < numFiles; ++i)
			{
				// Get the length of the file path and retrieve it.
				// Giving 0 for the stringbuffer returns path size without nullbyte.
				UINT pathLen = DragQueryFileW((HDROP)wp, i, nullptr, 0);
				WideString wstr(pathLen, 0);
				DragQueryFileW((HDROP)wp, i, wstr.begin(), pathLen + 1);
				files[i] = Narrow(wstr);
			}

			DragFinish((HDROP)wp);

			// Pass the file drop event to the input handler.
			std::vector<const char*> filePtrs;
			for (const auto& file : files)
			{
				filePtrs.push_back(file.str());
			}
			myEvents.addFileDrop(filePtrs.data(), static_cast<int>(filePtrs.size()), pos.x, pos.y);
		}
		break;
	}
	case WM_SETCURSOR:
	{
		if(LOWORD(lp) == HTCLIENT)
		{
			HCURSOR cursor = LoadCursorW(nullptr, getCursorResource());
			if(cursor) SetCursor(cursor);
			result = TRUE;
			return true;
		}
		break;
	}
	case WM_COMMAND:
	{
		if(myIsInsideMessageLoop)
		{
			gEditor->onMenuAction(LOWORD(wp));
		}
		break;
	}
	}; // end of message switch.

	return false;
}

static LRESULT CALLBACK GlobalProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	LRESULT res = 0;
	bool handled = false;
	if(msg == WM_CREATE)
	{
		void* app = ((LPCREATESTRUCT)lp)->lpCreateParams;
		SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)app);
	}
	else
	{
		SystemImpl* app = (SystemImpl*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
		if (app)
			handled = app->handleMsg(msg, wp, lp, res);
	}
	return handled ? res : DefWindowProcW(hwnd, msg, wp, lp);
}

// ================================================================================================
// SystemImpl :: dialog boxes.

Result showMessageDlg(StringRef title, StringRef text, Buttons b, Icon i)
{
	WideString wtitle = Widen(title), wtext = Widen(text);
	int flags = sDlgType[b] | sDlgIcon[i], result = R_OK;
	switch(MessageBoxW((HWND)gSystem->getHWND(), wtext.str(), wtitle.str(), flags))
	{
	case IDOK: return R_OK;
	case IDYES: return R_YES;
	case IDNO: return R_NO;
	};
	return R_CANCEL;
}

String openFileDlg(StringRef title, StringRef filename, StringRef filters)
{
	return ShowFileDialog(title, filename, filters, nullptr, false);
}

String saveFileDlg(StringRef title, StringRef filename, StringRef filters, int* index)
{
	return ShowFileDialog(title, filename, filters, index, true);
}

// ================================================================================================
// SystemImpl :: misc/get/set functions.

bool runSystemCommand(StringRef cmd)
{
	return runSystemCommand(cmd, nullptr, nullptr);
}

bool runSystemCommand(StringRef cmd, CommandPipe* pipe, void* buffer)
{
	bool result = false;

	// Copy the command to a Vector because CreateProcessW requires a modifiable buffer, urgh.
	WideString wcommand = Widen(cmd);
	Vector<wchar_t> wbuffer;
	wbuffer.resize(wcommand.length() + 1);
	memcpy(wbuffer.begin(), wcommand.begin(), sizeof(wchar_t) * (wcommand.length() + 1));

	// Create a pipe for the process's stdin.
	STARTUPINFOW startupInfo;
	startupInfo.cb = sizeof(startupInfo);
	ZeroMemory(&startupInfo, sizeof(startupInfo));

	HANDLE readPipe, writePipe;
	if(pipe)
	{
		// Set the bInheritHandle flag so pipe handles are inherited. 
		SECURITY_ATTRIBUTES attr;
		attr.nLength = sizeof(SECURITY_ATTRIBUTES);
		attr.bInheritHandle = TRUE;
		attr.lpSecurityDescriptor = NULL;

		// Create a pipe to send data to stdin of the child process.
		CreatePipe(&readPipe, &writePipe, &attr, 0);
		SetHandleInformation(writePipe, HANDLE_FLAG_INHERIT, 0);

		startupInfo.hStdInput = readPipe;
		startupInfo.dwFlags |= STARTF_USESTDHANDLES;
	}

	// Fire off the process.
	int flags = CREATE_NO_WINDOW;
	PROCESS_INFORMATION processInfo;
	ZeroMemory(&processInfo, sizeof(processInfo));
	if(CreateProcessW(NULL, wbuffer.data(), NULL, NULL,
		TRUE, flags, NULL, NULL, &startupInfo, &processInfo))
	{
		if(pipe)
		{
			DWORD bytesWritten;
			int bytesRead = pipe->read();
			while(bytesRead > 0)
			{
				WriteFile(writePipe, buffer, bytesRead, &bytesWritten, NULL);
				bytesRead = pipe->read();
			}
			CloseHandle(writePipe);
		}
		WaitForSingleObject(processInfo.hProcess, INFINITE);
		CloseHandle(processInfo.hProcess);
		CloseHandle(processInfo.hThread);
		result = true;
	}

	return result;
}

void openWebpage(StringRef link)
{
	ShellExecuteW(0, 0, Widen(link).str(), 0, 0, SW_SHOW);
}

void setWorkingDir(StringRef path)
{
	SetCurrentDirectoryW(Widen(path).str());
}

void setCursor(Cursor::Icon c)
{
	myCursor = c;
}

void disableVsync()
{
	if(wglSwapInterval)
	{
		Debug::log("[NOTE] turning off v-sync\n");
		wglSwapInterval(0);
	}
}

double getElapsedTime() const
{
	return Debug::getElapsedTime(myApplicationStartTime);
}

void* getHWND() const
{
	return myHWND;
}

String getExeDir() const
{
	return Narrow(sExeDir);
}

String getRunDir() const
{
	return Narrow(sRunDir);
}

Cursor::Icon getCursor() const
{
	return myCursor;
}

bool isKeyDown(Key::Code key) const
{
	return myKeyState.test(key);
}

bool isMouseDown(Mouse::Code button) const
{
	return myMouseState.test(button);
}

vec2i getMousePos() const
{
	return myMousePos;
}

void setWindowTitle(StringRef text)
{
	if(!(myTitle == text))
	{
		SetWindowTextW(myHWND, Widen(text).str());
		myTitle = text;
	}
}

vec2i getWindowSize() const
{
	return mySize;
}

StringRef getWindowTitle() const
{
	return myTitle;
}

InputEvents& getEvents()
{
	return myEvents;
}

bool isActive() const
{
	return myIsActive;
}

void terminate()
{
	myIsTerminated = true;
}

}; // SystemImpl.
}; // anonymous namespace.

System* gSystem = nullptr;

}; // namespace Vortex
using namespace Vortex;

String System::getLocalTime()
{
	time_t t = time(0);
	tm* now = localtime(&t);
	String time = asctime(localtime(&t));
	if(time.back() == '\n') Str::pop_back(time);
	return time;
}

String System::getBuildData()
{
	String date(__DATE__);
	if(date[4] == ' ') date.begin()[4] = '0';
	return date;
}

static void ApplicationStart()
{
	// Set the executable directory as the working dir.
	GetCurrentDirectoryW(MAX_PATH, sRunDir);
	GetModuleFileNameW(NULL, sExeDir, MAX_PATH);
	wchar_t* finalSlash = wcsrchr(sExeDir, L'\\');
	if(finalSlash) finalSlash[1] = 0;
	SetCurrentDirectoryW(sExeDir);

	// Log the application start-up time.
	Debug::openLogFile();
	Debug::log("Starting ArrowVortex :: %s\n", System::getLocalTime().str());
	Debug::log("Build: %s\n", System::getBuildData().str());
	Debug::logBlankLine();
}

static void ApplicationEnd()
{
	// Log the application termination time.
	time_t t = time(0);
	tm* now = localtime(&t);
	Debug::logBlankLine();
	Debug::log("Closing ArrowVortex :: %s", System::getLocalTime().str());
}

int APIENTRY WinMain(HINSTANCE, HINSTANCE, char*, int)
{
	//_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_ALWAYS_DF);

	ApplicationStart();
#ifndef NDEBUG
	Debug::openConsole();
#endif
	gSystem = new SystemImpl;
	((SystemImpl*)gSystem)->messageLoop();
	delete (SystemImpl*)gSystem;
	ApplicationEnd();

#ifdef CRTDBG_MAP_ALLOC
	_CrtDumpMemoryLeaks();
#endif

	return 0;
}
