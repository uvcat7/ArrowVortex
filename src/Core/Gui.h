#pragma once

#include <Core/Input.h>
#include <Core/Slot.h>

namespace Vortex {

/// A context for gui objects that can contains widgets, layouts and dialogs.
class GuiContext {
   public:
    virtual ~GuiContext();

    static GuiContext* create();

    // Application functions.
    virtual void tick(recti view, float deltaTime, InputEvents& events) = 0;
    virtual void draw() = 0;

    // Functions used by widgets.
    virtual recti getView() = 0;
    virtual vec2i getMousePos() = 0;
    virtual float getDeltaTime() = 0;
    virtual InputEvents& getEvents() = 0;

    // Functions that bind a read-only value to a slot.
    virtual bool bind(const std::string& slot, const int* v) = 0;
    virtual bool bind(const std::string& slot, const uint32_t* v) = 0;
    virtual bool bind(const std::string& slot, const long* v) = 0;
    virtual bool bind(const std::string& slot, const uint64_t* v) = 0;
    virtual bool bind(const std::string& slot, const float* v) = 0;
    virtual bool bind(const std::string& slot, const double* v) = 0;
    virtual bool bind(const std::string& slot, const bool* v) = 0;
    virtual bool bind(const std::string& slot, const char* str) = 0;
    virtual bool bind(const std::string& slot, const std::string* str) = 0;

    // Functions that bind a read-write value to a slot.
    virtual bool bind(const std::string& slot, int* v) = 0;
    virtual bool bind(const std::string& slot, uint32_t* v) = 0;
    virtual bool bind(const std::string& slot, long* v) = 0;
    virtual bool bind(const std::string& slot, uint64_t* v) = 0;
    virtual bool bind(const std::string& slot, float* v) = 0;
    virtual bool bind(const std::string& slot, double* v) = 0;
    virtual bool bind(const std::string& slot, bool* v) = 0;
    virtual bool bind(const std::string& slot, std::string* str) = 0;

    /// Binds a functor to a call slot, and takes ownership of it.
    /// On failure, false is returned and the functor is deleted.
    virtual bool bind(const std::string& slot, Functor::Generic* f) = 0;

    /// Binds a static function without arguments.
    template <typename Result>
    bool bind(const std::string& slot, Result (*function)()) {
        return bind(slot, new Functor::Static<Result>(function));
    }

    /// Binds a static function with one argument.
    template <typename Result, typename Arg>
    bool bind(const std::string& slot, Result (*function)(Arg), Arg arg) {
        return bind(slot,
                    new Functor::StaticWithArg<Result, Arg>(function, arg));
    }

    /// Binds an object with a member function without arguments.
    template <typename Result, typename Object>
    bool bind(const std::string& slot, Object* obj,
              Result (Object::*member)()) {
        return bind(slot, new Functor::Member<Result, Object>(obj, member));
    }

    /// Binds an object with a member function with one argument.
    template <typename Result, typename Object, typename Arg>
    bool bind(const std::string& slot, Object* obj,
              Result (Object::*member)(Arg), Arg arg) {
        return bind(slot, new Functor::MemberWithArg<Result, Object, Arg>(
                              obj, member, arg));
    }
};

/// Base class for widget objects.
class GuiWidget : public InputHandler {
   public:
    friend struct GuiManager;

    virtual ~GuiWidget();

    GuiWidget(GuiContext* gui);

    // Captures mouse over for the current frame.
    void captureMouseOver();
    void blockMouseOver();
    void unblockMouseOver();
    bool isMouseOver() const;

    // Captures mouse input and mouse over for a duration of time.
    void startCapturingMouse();
    void stopCapturingMouse();
    bool isCapturingMouse() const;

    // Captures text input for a duration of time.
    void startCapturingText();
    void stopCapturingText();
    bool isCapturingText() const;

    // Captures focus for a duration of time.
    void startCapturingFocus();
    void stopCapturingFocus();
    bool isCapturingFocus() const;

    // Update functions.
    void updateSize();
    void arrange(recti r);
    void tick();
    void draw();

    // Set functions.
    void setId(const std::string& id);
    void setWidth(int w);
    void setHeight(int h);
    void setSize(int w, int h);
    void setEnabled(bool enabled);
    void setTooltip(const std::string& text);

    // Get functions.
    bool isEnabled() const;
    int getWidth() const;
    int getHeight() const;
    vec2i getSize() const;
    recti getRect() const;
    std::string getTooltip() const;
    GuiContext* getGui() const;

   protected:
    friend class GuiContextImpl;

    virtual void onMouseCaptureLost();
    virtual void onTextCaptureLost();

    virtual void onUpdateSize();
    virtual void onArrange(recti r);
    virtual void onTick();
    virtual void onDraw();

   protected:
    GuiContext* gui_;
    recti rect_;
    int width_;
    int height_;
    uint32_t flags_;
};

// Base class for dialog objects.
class GuiDialog {
   public:
    virtual ~GuiDialog();

    GuiDialog(GuiContext* gui);

    virtual void onUpdateSize();
    virtual void onArrange(recti r);
    virtual void onTick();
    virtual void onDraw();

    void requestClose();
    void requestPin();

    void setPosition(int x, int y);
    void setTitle(const std::string& title);

    void setWidth(int w);
    void setHeight(int h);

    void setMinimumWidth(int w);
    void setMinimumHeight(int h);

    void setMaximumWidth(int w);
    void setMaximumHeight(int h);

    void setCloseable(bool enable);
    void setMinimizable(bool enable);
    void setResizeable(bool horizontal, bool vertical);

    GuiContext* getGui() const;

    recti getOuterRect() const;
    recti getInnerRect() const;

    bool isPinned() const;

   protected:
    void* data_;  // TODO: replace with a more descriptive variable name.
};

/// Global gui functions.
struct GuiMain {
    /// Prototype for functions that create new widgets.
    typedef GuiWidget* (*CreateWidgetFunction)(GuiContext*);

    /// Prototype for functions that read text from the clipboard.
    typedef std::string (*ClipboardGetFunction)();

    /// Prototype for functions that write text to the clipboard.
    typedef void (*ClipboardSetFunction)(std::string);

    /// Call once to initialize the gui system.
    static void init();

    /// Call once destroy the gui system when finished.
    static void shutdown();

    /// Call each frame before you start drawing/updating Goo objects.
    static void frameStart(float dt, InputEvents& events);

    /// Call each frame after you are done drawing/updating Goo objects.
    static void frameEnd();

    /// Assigns a widget creation function to the given class name.
    static void registerWidgetClass(const char* className,
                                    CreateWidgetFunction func);

    /// Uses the widget creation function assigned to the given class name to
    /// create a widget.
    static GuiWidget* createWidget(const char* className, GuiContext* gui);

    /// Sets clipboard functions, which can be used by widgets to manipulate the
    /// clipboard.
    static void setClipboardFunctions(ClipboardGetFunction get,
                                      ClipboardSetFunction set);

    /// Sets the clipboard text by using the assigned ClipboardSetFunction.
    static void setClipboardText(std::string text);

    /// Returns the clipboard text obtained from the assigned
    /// ClipboardGetFunction.
    static std::string getClipboardText();

    /// Sets the mouse cursor icon, which is reset to CURSOR_ARROW at the start
    /// of each frame.
    static void setCursorIcon(Cursor::Icon icon);

    /// Returns the current mouse cursor icon.
    static Cursor::Icon getCursorIcon();

    /// Sets the current tooltip text, which is reset to an empty string at the
    /// start of each frame.
    static void setTooltip(std::string text);

    /// Returns the current tooltip text.
    static std::string getTooltip();

    /// Returns true if the mouse is hovering over a gui object or if a gui
    /// object is capturing mouse input.
    static bool isCapturingMouse();

    /// Returns true if an object is capturing text input.
    static bool isCapturingText();

    /// Sets the view size.
    static void setViewSize(int width, int height);

    /// Returns the view size.
    static vec2i getViewSize();
};

};  // namespace Vortex
