#ifndef NDEBUG
#include <stdlib.h>
#ifdef _WIN32
#define CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#include <System/Resources.h>
#endif
#endif

#include <System/System.h>
#include <System/File.h>
#include <System/Debug.h>

#include <Core/WideString.h>
#include <Core/StringUtils.h>
#include <Core/Shader.h>

#include <Editor/Editor.h>
#include <Editor/Menubar.h>

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_video.h>
#include <System/OpenGL.h>

#ifdef __APPLE__
#include <OpenGL/gl.h>
#endif

#include <chrono>
#include <thread>
#include <algorithm>
#include <cmath>
#include <numeric>
#include <ctime>
#include <bitset>
#include <list>
#include <vector>
#include <map>
#include <mutex>
#include <locale>
#include <condition_variable>

#undef DELETE

// Various statics had to be pulled out of the System singleton since
// SDL needs non-member callback functions for event handling.
bool myInitSuccesful = false;
Vortex::InputEvents myEvents;
Vortex::vec2i myMousePos = {0, 0};
Vortex::vec2i mySize = {0, 0};
SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
Vortex::Cursor::Icon myCursor = Vortex::Cursor::ARROW;
std::map<Vortex::Cursor::Icon, SDL_SystemCursor> myCursorMap;
bool myIsActive = false;
bool myIsTerminated = false;
bool myIsInsideMessageLoop = false;
std::vector<std::string> droppedFiles;
std::bitset<Vortex::Key::MAX_VALUE> myKeyState;
std::bitset<Vortex::Mouse::MAX_VALUE> myMouseState;
float myScale = 1.0f;

namespace Vortex {

std::chrono::duration<double> deltaTime;  // Defined in <Core/Core.h>

namespace {

static int wglSwapInterval;

static std::string outPath;
std::mutex fileDialogMutex;
std::condition_variable fileDialogCv;
fs::path fileDialogPath;
int file_extension_index = 0;
bool isDialogClosed = false;

// Mapping of windows virtual keys to vortex key codes.
static const int VKtoKCmap[] = {SDLK_GRAVE,        Key::ACCENT,
                                SDLK_MINUS,        Key::DASH,
                                SDLK_EQUALS,       Key::EQUAL,
                                SDLK_LEFTBRACKET,  Key::BRACKET_L,
                                SDLK_RIGHTBRACKET, Key::BRACKET_R,
                                SDLK_SEMICOLON,    Key::SEMICOLON,
                                SDLK_APOSTROPHE,   Key::QUOTE,
                                SDLK_BACKSLASH,    Key::BACKSLASH,
                                SDLK_COMMA,        Key::COMMA,
                                SDLK_PERIOD,       Key::PERIOD,
                                SDLK_SLASH,        Key::SLASH,
                                SDLK_SPACE,        Key::SPACE,
                                SDLK_ESCAPE,       Key::ESCAPE,
                                SDLK_LGUI,         Key::SYSTEM_L,
                                SDLK_RGUI,         Key::SYSTEM_R,
                                SDLK_TAB,          Key::TAB,
                                SDLK_CAPSLOCK,     Key::CAPS,
                                SDLK_RETURN,       Key::RETURN,
                                SDLK_BACKSPACE,    Key::BACKSPACE,
                                SDLK_PAGEUP,       Key::PAGE_UP,
                                SDLK_PAGEDOWN,     Key::PAGE_DOWN,
                                SDLK_HOME,         Key::HOME,
                                SDLK_END,          Key::END,
                                SDLK_INSERT,       Key::INSERT,
                                SDLK_DELETE,       Key::DELETE,
                                SDLK_PRINTSCREEN,  Key::PRINT_SCREEN,
                                SDLK_SCROLLLOCK,   Key::SCROLL_LOCK,
                                SDLK_PAUSE,        Key::PAUSE,
                                SDLK_LEFT,         Key::LEFT,
                                SDLK_RIGHT,        Key::RIGHT,
                                SDLK_UP,           Key::UP,
                                SDLK_DOWN,         Key::DOWN,
                                SDLK_NUMLOCKCLEAR, Key::NUM_LOCK,
                                SDLK_KP_DIVIDE,    Key::NUMPAD_DIVIDE,
                                SDLK_KP_MULTIPLY,  Key::NUMPAD_MULTIPLY,
                                SDLK_KP_MINUS,     Key::NUMPAD_SUBTRACT,
                                SDLK_KP_PLUS,      Key::NUMPAD_ADD,
                                SDLK_SEPARATOR,    Key::NUMPAD_SEPERATOR,
                                SDLK_LSHIFT,       Key::SHIFT_L,
                                SDLK_RSHIFT,       Key::SHIFT_R,
                                SDLK_LALT,         Key::ALT_L,
                                SDLK_RALT,         Key::ALT_R,
                                SDLK_LCTRL,        Key::CTRL_L,
                                SDLK_RCTRL,        Key::CTRL_R};

static void SDLCALL FileDialogOpenCallback(void* userdata,
                                           const char* const* filelist,
                                           int filter) {
    if (!filelist) {
        HudError("Failed to open the file dialog: \"%s\".", SDL_GetError());
        return;
    }
    if (filelist[0]) {
        fileDialogPath = utf8ToPath(filelist[0]);
    }
    isDialogClosed = true;
    fileDialogCv.notify_all();
    return;
}

static void SDLCALL FileDialogSaveCallback(void* userdata,
                                           const char* const* filelist,
                                           int filter) {
    if (!filelist) {
        HudError("Failed to open the file dialog: \"%s\".", SDL_GetError());
        return;
    }
    if (filelist[0]) {
        fileDialogPath = utf8ToPath(filelist[0]);
    }
    file_extension_index = filter;
    isDialogClosed = true;
    fileDialogCv.notify_all();
    return;
}

// Shows an open/save message box and returns the path selected by the user.
fs::path ShowFileDialog(std::string title, fs::path path,
                        SDL_DialogFileFilter filters[], int num_filters,
                        int* index, bool save) {
    fileDialogPath.clear();
    isDialogClosed = false;
    std::unique_lock<std::mutex> lock(fileDialogMutex);
    if (save) {
        SDL_ShowSaveFileDialog(FileDialogSaveCallback, nullptr, nullptr,
                               filters, num_filters, pathToUtf8(path).c_str());
    } else {
        SDL_ShowOpenFileDialog(FileDialogOpenCallback, nullptr, nullptr,
                               filters, num_filters, pathToUtf8(path).c_str(),
                               false);
    }

#ifdef __linux__
    /* On Fedora, SDL won't run the callback when the dialog is closed since the
       action triggers a DBus event SDL needs to process first.
       Only the main thread can pump events so we regularly signal it to do so.
       Yes, it's silly. */
    std::jthread signal([] {
        while (!isDialogClosed) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            fileDialogCv.notify_all();
        }
    });
#endif  // __linux__
    fileDialogCv.wait(lock, [] {
        SDL_PumpEvents();
        return isDialogClosed || !fileDialogPath.empty();
    });
    if (save) *index = file_extension_index;
    return fileDialogPath;
}

// ================================================================================================
// SystemImpl :: Debug logging.

static bool LogCheckpoint(bool result, const char* description) {
    if (result) {
        Debug::log("%s :: OK\n", description);
    } else {
        Debug::blockBegin(Debug::ERROR, description);
        Debug::log("SDL error message: %s", SDL_GetError());
        Debug::blockEnd();
    }
    return !result;
}

// ================================================================================================
// SystemImpl :: menu item.

};  // anonymous namespace

namespace {

// ================================================================================================
// SystemImpl :: member data.

struct SystemImpl : public System {
    std::chrono::steady_clock::time_point myApplicationStartTime;
    std::map<SDL_Keycode, Key::Code> myKeyMap;
    std::string myTitle;
    SDL_GLContext myHRC = nullptr;
    std::string workingDirectory;

    // ================================================================================================
    // SystemImpl :: constructor and destructor.

    ~SystemImpl() {
        // Destroy the rendering context.
        if (myHRC) SDL_GL_DestroyContext(myHRC);

        // Destroy the window.
        if (window) SDL_DestroyWindow(window);
    }

    SystemImpl() : myTitle("ArrowVortex") {
        myApplicationStartTime = Debug::getElapsedTime();

        if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
            SDL_Log("Couldn't initialize SDL subsystems: %s", SDL_GetError());
        }

        // Initialize the keymap, which maps windows virtual keys to vortex key
        // codes.
        int k = sizeof(VKtoKCmap) / sizeof(VKtoKCmap[0]);
        for (int i = 0; i < k; i += 2)
            myKeyMap.insert({static_cast<SDL_Keycode>(VKtoKCmap[i]),
                             static_cast<Key::Code>(VKtoKCmap[i + 1])});
        for (int i = 0; i < 26; ++i)
            myKeyMap.insert({static_cast<SDL_Keycode>(SDLK_A + i),
                             static_cast<Key::Code>(Key::A + i)});
        for (int i = 0; i < 10; ++i)
            myKeyMap.insert({static_cast<SDL_Keycode>(SDLK_0 + i),
                             static_cast<Key::Code>(Key::DIGIT_0 + i)});
        for (int i = 0; i < 15; ++i)
            myKeyMap.insert({static_cast<SDL_Keycode>(SDLK_F1 + i),
                             static_cast<Key::Code>(Key::F1 + i)});
        for (int i = 0; i < 9; ++i)
            myKeyMap.insert({static_cast<SDL_Keycode>(SDLK_KP_0 + i),
                             static_cast<Key::Code>(Key::NUMPAD_0 + i)});

        // Initialize the cursor map
        myCursorMap.insert({Cursor::ARROW, SDL_SYSTEM_CURSOR_DEFAULT});
        myCursorMap.insert({Cursor::HAND, SDL_SYSTEM_CURSOR_POINTER});
        myCursorMap.insert({Cursor::IBEAM, SDL_SYSTEM_CURSOR_TEXT});
        myCursorMap.insert({Cursor::SIZE_ALL, SDL_SYSTEM_CURSOR_MOVE});
        myCursorMap.insert({Cursor::SIZE_WE, SDL_SYSTEM_CURSOR_EW_RESIZE});
        myCursorMap.insert({Cursor::SIZE_NS, SDL_SYSTEM_CURSOR_NS_RESIZE});
        myCursorMap.insert({Cursor::SIZE_NESW, SDL_SYSTEM_CURSOR_NESW_RESIZE});
        myCursorMap.insert({Cursor::SIZE_NWSE, SDL_SYSTEM_CURSOR_NWSE_RESIZE});

#if defined(__APPLE__)
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                            SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
#endif

        // Avoid creating both a Metal and SDL context on macOS
        window =
            SDL_CreateWindow("ArrowVortex", 800, 600,
                             SDL_WINDOW_OPENGL | SDL_WINDOW_HIGH_PIXEL_DENSITY |
                                 SDL_WINDOW_RESIZABLE);
        if (!window) {
            SDL_Log("Couldn't create window: %s", SDL_GetError());
        }

        if (LogCheckpoint(window != nullptr, "creating window")) return;

        // Create the OpenGL rendering context.
        myHRC = SDL_GL_CreateContext(window);
        if (LogCheckpoint(myHRC != nullptr, "creating OpenGL context")) return;

        bool mc = SDL_GL_MakeCurrent(window, myHRC);
        if (LogCheckpoint(mc != 0, "activating OpenGL context")) return;

        VortexCheckGlError();

        // Initialize the OpenGL settings.
        glClearColor(0, 0, 0, 1);
        glEnable(GL_BLEND);
        glEnable(GL_TEXTURE_2D);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnableClientState(GL_VERTEX_ARRAY);

        VortexCheckGlError();

        // Enable vsync for now, we will disable it later if the settings
        // require it.
        bool interval_supported = SDL_GL_GetSwapInterval(&wglSwapInterval);
        Debug::log("swap interval support :: %s\n",
                   interval_supported ? "OK" : "MISSING");
        if (interval_supported) {
            if (!SDL_GL_SetSwapInterval(1))
                Debug::log("Failed to set V-sync state: %s\n", SDL_GetError());
        }

        // Check for shader support.
        Shader::initExtension();
        Debug::logBlankLine();

        // Make sure the window is centered on the desktop.
        myScale = SDL_GetWindowDisplayScale(window);
        setWindowSize({static_cast<int>(1200 * myScale),
                       static_cast<int>(900 * myScale)});

        // Show the window.
        myIsActive = true;
        myInitSuccesful = true;
    }

    // ================================================================================================
    // SystemImpl :: message loop.

    void createMenu() override {
#ifndef GL_MENU_BAR
        HMENU menu = CreateMenu();
        gMenubar->init(reinterpret_cast<MenuItem*>(menu));
        SetMenu(GetActiveWindow(), menu);
        SDL_SetWindowsMessageHook(
            [](void* userdata, tagMSG* msg) {
                if (msg->message == WM_COMMAND) {
                    gEditor->onMenuAction(LOWORD(msg->wParam));
                    return false;
                }
                return true;
            },
            nullptr);
#else
        gMenubar->init(new MenuItem);
#endif
    }

    // ================================================================================================
    // SystemImpl :: clipboard functions.

    bool setClipboardText(const std::string& text) override {
        return SDL_SetClipboardText(text.c_str());
    }

    std::string getClipboardText() const override {
        return std::string(SDL_GetClipboardText());
    }

    // ================================================================================================
    // SystemImpl :: message handling.

    SDL_SystemCursor getCursorResource() const override {
        return myCursorMap[myCursor];
    }

    int getKeyFlags() const override {
        int kc[6] = {Key::SHIFT_L, Key::SHIFT_R, Key::CTRL_L,
                     Key::CTRL_R,  Key::ALT_L,   Key::ALT_R};
        int kf[6] = {Keyflag::SHIFT, Keyflag::SHIFT, Keyflag::CTRL,
                     Keyflag::CTRL,  Keyflag::ALT,   Keyflag::ALT};

        int flags = 0;
        for (int i = 0; i < 6; ++i)
            if (myKeyState.test(kc[i])) flags |= kf[i];

        return flags;
    }

    Key::Code translateKeyCode(SDL_Keycode vkCode) override {
        if (myKeyMap.contains(vkCode))
            return myKeyMap[vkCode];
        else
            return Key::NONE;
    }

    void handleKeyPress(Key::Code kc, bool repeated) override {
        bool handled = false;
        int kf = getKeyFlags();
        myEvents.addKeyPress(kc, kf, repeated);
        myKeyState.set(kc);
    }

    // ================================================================================================
    // SystemImpl :: dialog boxes.

    Result showMessageDlg(const std::string& title, const std::string& text,
                          Buttons b, Icon i) override {
        SDL_MessageBoxData box;
        SDL_MessageBoxButtonData buttons[3];
        box.flags = SDL_MESSAGEBOX_BUTTONS_LEFT_TO_RIGHT;
        box.window = nullptr;
        box.title = title.c_str();
        box.message = text.c_str();
        box.colorScheme = nullptr;
        switch (b) {
            case (T_OK):
                box.numbuttons = 1;
                buttons[0].buttonID = R_OK;
                buttons[0].text = "OK";
                buttons[0].flags = SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT;
                break;
            case (T_OK_CANCEL):
                box.numbuttons = 2;
                buttons[0].buttonID = R_OK;
                buttons[0].text = "OK";
                buttons[0].flags = SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT;
                buttons[1].buttonID = R_CANCEL;
                buttons[1].text = "CANCEL";
                buttons[1].flags = SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT;
                break;
            case (T_YES_NO):
                box.numbuttons = 2;
                buttons[0].buttonID = R_YES;
                buttons[0].text = "YES";
                buttons[0].flags = SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT;
                buttons[1].buttonID = R_NO;
                buttons[1].text = "NO";
                buttons[1].flags = SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT;
                break;
            case (T_YES_NO_CANCEL):
                box.numbuttons = 3;
                buttons[0].buttonID = R_YES;
                buttons[0].text = "YES";
                buttons[0].flags = SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT;
                buttons[1].buttonID = R_NO;
                buttons[1].text = "NO";
                buttons[1].flags = 0;
                buttons[2].buttonID = R_CANCEL;
                buttons[2].text = "CANCEL";
                buttons[2].flags = SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT;
                break;
            default:
                return R_CANCEL;
        }
        box.buttons = buttons;
        int result = 0;
        if (!SDL_ShowMessageBox(&box, &result)) {
            HudError("Failed to open message box with error %s",
                     SDL_GetError());
            return R_CANCEL;
        }
        return static_cast<Result>(result);
    }

    fs::path openFileDlg(const std::string& title,
                         SDL_DialogFileFilter filters[], int num_filters,
                         fs::path filename) override {
        return ShowFileDialog(title, filename, filters, num_filters, nullptr,
                              false);
    }

    fs::path saveFileDlg(const std::string& title,
                         SDL_DialogFileFilter filters[], int num_filters,
                         int* index, fs::path filename) override {
        return ShowFileDialog(title, filename, filters, num_filters, index,
                              true);
    }

    // ================================================================================================
    // SystemImpl :: misc/get/set functions.

    void openWebpage(const std::string& link) override {
        SDL_OpenURL(link.c_str());
    }

    void setCursor(Cursor::Icon c) override {
        myCursor = c;
        SDL_SetCursor(SDL_CreateSystemCursor(getCursorResource()));
    }

    void disableVsync() override {
        if (!SDL_GL_SetSwapInterval(0))
            HudError("Failed to disable V-sync: %s", SDL_GetError());
    }

    double getElapsedTime() const override {
        return Debug::getElapsedTime(myApplicationStartTime);
    }

    std::string getRunDir() const override {
        return std::string(SDL_GetBasePath());
    }

    Cursor::Icon getCursor() const override { return myCursor; }

    bool isKeyDown(Key::Code key) const override {
        return myKeyState.test(key);
    }

    bool isMouseDown(Mouse::Code button) const override {
        return myMouseState.test(button);
    }

    vec2i getMousePos() const override { return myMousePos; }

    const std::string& getWindowTitle() const override { return myTitle; }

    void setWindowTitle(const std::string& text) override {
        SDL_SetWindowTitle(window, text.c_str());
    }

    vec2i getWindowSize() const override {
        vec2i size = {mySize.x, mySize.y};
        if (!gMenubar)
            return size;
        else
            return {size.x, size.y - gMenubar->getMenubarHeight()};
    }

    int applyScaleFactor(int size) const override {
        return static_cast<int>(static_cast<float>(size) * myScale);
    }

    void setWindowSize(vec2i size) override {
        mySize = {std::clamp(size.x, 100, 32768),
                  std::clamp(size.y, 100, 32768)};
        SDL_SetWindowSize(window, mySize.x, mySize.y);
        SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED);
    }

    bool getWindowState() const override {
        auto temp = SDL_GetWindowFlags(window);
        return (SDL_GetWindowFlags(window) & SDL_WINDOW_MAXIMIZED);
    }

    void setWindowState(bool isMaximized) const override {
        if (isMaximized) {
            if (!SDL_MaximizeWindow(window))
                Debug::log("Couldn't maximize window with error: %s\n",
                           SDL_GetError());
        } else if (!SDL_RestoreWindow(window))
            Debug::log("Couldn't restore window with error: %s\n",
                       SDL_GetError());
    }

    InputEvents& getEvents() override { return myEvents; }

    bool isActive() const override { return myIsActive; }

    void terminate() override { myIsTerminated = true; }

};  // SystemImpl.
};  // anonymous namespace.

System* gSystem = nullptr;

};  // namespace Vortex
using namespace Vortex;

std::string System::getLocalTime() {
    time_t t = time(nullptr);
    tm* now = localtime(&t);
    std::string time = asctime(localtime(&t));
    if (time.back() == '\n') Str::pop_back(time);
    return time;
}

std::string System::getBuildData() {
    std::string date(__DATE__);
    if (date[4] == ' ') date.begin()[4] = '0';
    return date;
}

static void ApplicationStart() {
    // Log the application start-up time.
    Debug::openLogFile();
    Debug::log("Starting ArrowVortex :: %s\n", System::getLocalTime().c_str());
    Debug::log("Build: %s\n", System::getBuildData().c_str());
    Debug::logBlankLine();
}

static void ApplicationEnd() {
    // Log the application termination time.
    time_t t = time(nullptr);
    tm* now = localtime(&t);
    Debug::logBlankLine();
    Debug::log("Closing ArrowVortex :: %s", System::getLocalTime().c_str());
}

SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv) {
#ifdef WIN32
    std::wstring exe_folder = Widen(SDL_GetBasePath());
    SetCurrentDirectoryW(exe_folder.c_str());
#endif
    ApplicationStart();
#ifndef NDEBUG
    Debug::openConsole();
#endif
    gSystem = new SystemImpl;
    Editor::create();
    SDL_StartTextInput(window);
    SDL_SetWindowResizable(window, true);

    if (argc > 1) {
        gEditor->openSimfile(fs::path(argv[1]));
    }

    myIsInsideMessageLoop = true;

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate) {
    using namespace std::chrono;

    if (!myInitSuccesful) return SDL_APP_FAILURE;
    if (myIsTerminated) {
        Editor::destroy();
        return SDL_APP_SUCCESS;
    }

#ifndef NDEBUG
    static long long frames = 0;
    static auto lowcounts = 0;
    std::list<double> fpsList, sleepList, frameList, inputList, waitList;
    // Adjust frameGuess to your VSync target if you are testing with VSync
    // enabled
    auto frameGuess = 960;
#endif

    // Non-vsync FPS max target
    auto frameTarget = duration<double>(1.0 / 960.0);

    // Enter the message loop.
    auto prevTime = Debug::getElapsedTime();
    auto startTime = Debug::getElapsedTime();

    int x = mySize.x;
    int y = mySize.y;
    // Set up the OpenGL view.
    glViewport(0, 0, x, y);
    glLoadIdentity();
    glOrtho(0, x, y, 0, -1, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Reset the mouse cursor.
    myCursor = Cursor::ARROW;

#ifndef NDEBUG
    auto inputTime = Debug::getElapsedTime();

    VortexCheckGlError();
#endif

    gEditor->tick();
    // Tick processes the events, so clear them out after.
    myEvents.clear();

    // Display.
    SDL_GL_SwapWindow(window);

#ifndef NDEBUG
    auto renderTime = Debug::getElapsedTime();
#endif
    // Tick function.
    duration<double> frameTime = Debug::getElapsedTime() - prevTime;
    auto waitTime = frameTarget.count() - frameTime.count();

    if (wglSwapInterval) {
        while (Debug::getElapsedTime() - prevTime < frameTarget) {
            std::this_thread::yield();
        }
    }

    // End of frame
    auto curTime = Debug::getElapsedTime();
    deltaTime = duration<double>(static_cast<float>(std::min(
        std::max(0.0, duration<double>(curTime - prevTime).count()), 0.25)));
    prevTime = curTime;

#ifndef NDEBUG
    // Do frame statistics
    // Note that these will be wrong with VSync enabled.
    fpsList.push_front(deltaTime.count());
    waitList.push_front(duration<double>(curTime - renderTime).count());
    frameList.push_front(duration<double>(renderTime - inputTime).count());
    inputList.push_front(duration<double>(inputTime - startTime).count());

    if (abs(deltaTime.count() - 1.0 / static_cast<double>(frameGuess)) /
            (1.0 / static_cast<double>(frameGuess)) >
        0.01) {
        lowcounts++;
    }
    if (fpsList.size() >= frameGuess * 2) {
        fpsList.pop_back();
        frameList.pop_back();
        inputList.pop_back();
        waitList.pop_back();
    }
    auto min = *std::min_element(fpsList.begin(), fpsList.end());
    auto max = *std::max_element(fpsList.begin(), fpsList.end());
    auto maxIndex = std::distance(
        fpsList.begin(), std::max_element(fpsList.begin(), fpsList.end()));
    auto siz = fpsList.size();
    auto avg = std::accumulate(fpsList.begin(), fpsList.end(), 0.0) / siz;
    auto varianceFunc = [&avg, &siz](double accumulator, double val) {
        return accumulator + (val - avg) * (val - avg);
    };
    auto std = std::sqrt(
        std::accumulate(fpsList.begin(), fpsList.end(), 0.0, varianceFunc) /
        siz);
    auto frameAvg = std::accumulate(frameList.begin(), frameList.end(), 0.0) /
                    frameList.size();
    auto frameMax = frameList.begin();
    std::advance(frameMax, maxIndex);
    auto inputMax = inputList.begin();
    std::advance(inputMax, maxIndex);
    auto waitMax = waitList.begin();
    std::advance(waitMax, maxIndex);
    if (frames % (frameGuess * 2) == 0) {
        Debug::log(
            "frame total average: %f, frame render average %f, std dev "
            "%f, lowest FPS %f, highest FPS %f, highest FPS render "
            "time %f, highest FPS input time %f, highest FPS wait time "
            "%f, lag frames %d\n",
            avg, frameAvg, std, 1.0 / max, 1.0 / min, *frameMax, *inputMax,
            *waitMax, lowcounts);
        lowcounts = 0;
    }
    frames++;
#endif
    return SDL_APP_CONTINUE;
}

// Convert SDL window mouse coordinates to the app's OpenGL coordinate space.
static vec2i windowMouseToApp(float wx, float wy) {
    int menu_h = gMenubar ? gMenubar->getMenubarHeight() : 0;
    float rx = wx;
    float ry = wy;
    if (renderer) {
        if (!SDL_RenderCoordinatesFromWindow(renderer, wx, wy, &rx, &ry))
            HudError("Failed to get render coordinates with error: %s",
                     SDL_GetError());
    }
    return {static_cast<int>(rx), static_cast<int>(ry - menu_h)};
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
    static const Mouse::Code mcodes[4] = {Mouse::NONE, Mouse::LMB, Mouse::MMB,
                                          Mouse::RMB};
    int mc = 0;
    int menu_h = gMenubar ? gMenubar->getMenubarHeight() : 0;
    switch (event->type) {
        case SDL_EVENT_QUIT: {
            gEditor->onExitProgram();
            // myIsTerminated is set to true which actually stops the program
            return SDL_APP_CONTINUE;
        }
        case SDL_EVENT_WINDOW_MOUSE_ENTER:
        case SDL_EVENT_WINDOW_FOCUS_GAINED: {
            myMouseState.reset();
            myKeyState.reset();
            break;
        }
        case SDL_EVENT_WINDOW_MOUSE_LEAVE:
        case SDL_EVENT_WINDOW_FOCUS_LOST: {
            myEvents.addWindowInactive();
            myMouseState.reset();
            myKeyState.reset();
            break;
        }
        case SDL_EVENT_WINDOW_RESIZED:
        case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED: {
            myScale = SDL_GetWindowDisplayScale(window);
            vec2i next = {event->window.data1, event->window.data2};
            if (!SDL_GetWindowSizeInPixels(window, &next.x, &next.y))
                HudError("Failed to get SDL window size with error %s",
                         SDL_GetError());
            if (next.x > 0 && next.y > 0) mySize = next;
            break;
        }
        case SDL_EVENT_MOUSE_MOTION: {
            if (myIsInsideMessageLoop) {
                myMousePos = windowMouseToApp(event->motion.x, event->motion.y);
                myEvents.addMouseMove(myMousePos.x, myMousePos.y);
            }
            break;
        }
        case SDL_EVENT_MOUSE_WHEEL: {
            if (myIsInsideMessageLoop) {
                bool up = (event->wheel.y > 0) !=
                          (event->wheel.direction != SDL_MOUSEWHEEL_NORMAL);
                myEvents.addMouseScroll(up, event->wheel.x, event->wheel.y,
                                        gSystem->getKeyFlags());
            }
            break;
        }
        case SDL_EVENT_KEY_DOWN: {
            if (myIsInsideMessageLoop) {
                Key::Code kc = gSystem->translateKeyCode(event->key.key);
                gSystem->handleKeyPress(kc, event->key.repeat);
            }
            break;
        }
        case SDL_EVENT_KEY_UP: {
            if (myIsInsideMessageLoop) {
                Key::Code kc = gSystem->translateKeyCode(event->key.key);
                myEvents.addKeyRelease(kc, gSystem->getKeyFlags());
                myKeyState.reset(kc);
            }
            break;
        }
        case SDL_EVENT_MOUSE_BUTTON_DOWN: {
            mc = event->button.button;
            if (mc >= SDL_BUTTON_LEFT && mc <= SDL_BUTTON_RIGHT) {
                SDL_CaptureMouse(true);
                if (myIsInsideMessageLoop) {
                    auto mdc = mcodes[mc];
                    float x, y;
                    SDL_GetMouseState(&x, &y);
                    myMousePos = windowMouseToApp(x, y);
                    myEvents.addMousePress(mcodes[mc], myMousePos.x,
                                           myMousePos.y, gSystem->getKeyFlags(),
                                           event->button.clicks >= 2);
                    myMouseState.set(mcodes[mc]);
                }
            }
            break;
        }
        case SDL_EVENT_MOUSE_BUTTON_UP: {
            mc = event->button.button;
            if (mc >= SDL_BUTTON_LEFT && mc <= SDL_BUTTON_RIGHT) {
                SDL_CaptureMouse(false);
                if (myIsInsideMessageLoop) {
                    float x, y;
                    SDL_GetMouseState(&x, &y);
                    myMousePos = windowMouseToApp(x, y);
                    myEvents.addMouseRelease(mcodes[mc], myMousePos.x,
                                             myMousePos.y,
                                             gSystem->getKeyFlags());
                    myMouseState.reset(mcodes[mc]);
                }
            }
            break;
        }
        case SDL_EVENT_TEXT_INPUT: {
            const char* wp = event->text.text;
            const char n = '\n';
            if (!std::iscntrl(*reinterpret_cast<const char8_t*>(wp)))
                myEvents.addTextInput(wp);
            else if (*wp == '\r')
                myEvents.addTextInput(&n);
            break;
        }
        case SDL_EVENT_DROP_BEGIN: {
            if (myIsInsideMessageLoop) {
                droppedFiles.clear();
            }
            break;
        }
        case SDL_EVENT_DROP_FILE: {
            if (myIsInsideMessageLoop) {
                droppedFiles.emplace_back(event->drop.data);
            }
            break;
        }
        case SDL_EVENT_DROP_COMPLETE: {
            if (myIsInsideMessageLoop && !droppedFiles.empty()) {
                std::vector<const char*> filePtrs;
                for (const auto& file : droppedFiles) {
                    filePtrs.emplace_back(file.c_str());
                }
                vec2i drop_pos = windowMouseToApp(event->drop.x, event->drop.y);
                myEvents.addFileDrop(filePtrs.data(),
                                     static_cast<int>(filePtrs.size()),
                                     drop_pos.x, drop_pos.y);
            }
            break;
        }
    };  // end of message switch.
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
    SDL_StopTextInput(window);
    delete static_cast<SystemImpl*>(gSystem);
    ApplicationEnd();

#ifdef CRTDBG_MAP_ALLOC
    _CrtDumpMemoryLeaks();
#endif
}