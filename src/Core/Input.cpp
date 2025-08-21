#include <Core/Input.h>

#include <System/Debug.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

namespace Vortex {
namespace {

enum EventTypes {
    ET_NONE,

    ET_MOUSE_MOVE,
    ET_MOUSE_PRESS,
    ET_MOUSE_RELEASE,
    ET_MOUSE_SCROLL,
    ET_KEY_PRESS,
    ET_KEY_RELEASE,
    ET_TEXT_INPUT,
    ET_FILE_DROP,
    ET_WINDOW_INACTIVE,

    NUM_EVENT_TYPES
};

struct EventHeader {
    EventHeader* next;
    unsigned int type : 8;
    unsigned int size : 24;
};

static void SetTextInputPointers(TextInput& event) {
    event.text = reinterpret_cast<char*>(&event + 1);
}

static void SetFileDropPointers(FileDrop& event) {
    char** files = reinterpret_cast<char**>(&event + 1);
    char* str = reinterpret_cast<char*>(files + event.count);
    for (int i = 0; i < event.count; ++i) {
        files[i] = str;
        str += strlen(str) + 1;
    }
    event.files = files;
}

static EventHeader* CopyEvent(EventHeader* event) {
    EventHeader* out = static_cast<EventHeader*>(malloc(event->size));
    memcpy(out, event, event->size);
    if (out->type == ET_TEXT_INPUT) {
        SetTextInputPointers(*reinterpret_cast<TextInput*>(out + 1));
    } else if (out->type == ET_FILE_DROP) {
        SetFileDropPointers(*reinterpret_cast<FileDrop*>(out + 1));
    }
    return out;
}

static char* AddEvent(void*& data, int type, int eventSize) {
    int numBytes = sizeof(EventHeader) + eventSize;
    EventHeader* header = static_cast<EventHeader*>(malloc(numBytes));
    if (data) {
        EventHeader* last = static_cast<EventHeader*>(data);
        while (last->next) last = last->next;
        last->next = header;
    } else {
        data = header;
    }
    header->type = type;
    header->next = nullptr;
    header->size = numBytes;
    return reinterpret_cast<char*>(header + 1);
}

template <typename T>
static void AddEventT(void*& data, int type, const T& eventData) {
    char* out = AddEvent(data, type, sizeof(eventData));
    memcpy(out, &eventData, sizeof(eventData));
}

static char* WriteString(void* buffer, const char* str) {
    size_t size = strlen(str) + 1;
    memcpy(buffer, str, size);
    return static_cast<char*>(buffer) + size;
}

template <typename T>
static char* WriteData(char* buffer, T& data) {
    memcpy(buffer, &data, sizeof(data));
    return buffer + sizeof(data);
}

static void ValidateEvents(const void* data, const void* it = nullptr) {
    bool itFound = false;
    for (EventHeader* event =
             static_cast<EventHeader*>(const_cast<void*>(data));
         event; event = event->next) {
        VortexAssert(event->type >= 1 && event->type < NUM_EVENT_TYPES);
        if (it && (event + 1) == it) itFound = true;
    }
    if (it) {
        VortexAssert(itFound);
    }
}

static char* ReadNextEvent(void* data, int type, char* it) {
    EventHeader* cur;
    if (it) {
        cur = reinterpret_cast<EventHeader*>(it);
        cur = (cur - 1)->next;
    } else {
        cur = static_cast<EventHeader*>(data);
    }
    while (cur && cur->type != type) {
        cur = cur->next;
    }
    return cur ? reinterpret_cast<char*>(cur + 1) : nullptr;
}

template <typename T>
bool ReadNext(void* data, int type, T*& it) {
    it = reinterpret_cast<T*>(
        ReadNextEvent(data, type, reinterpret_cast<char*>(it)));
    return it != nullptr;
}

};  // namespace

// ================================================================================================
// Reading functions.

InputEvents::~InputEvents() { clear(); }

InputEvents::InputEvents() : data_(nullptr) {}

InputEvents::InputEvents(const InputEvents& other) : data_(nullptr) {
    *this = other;
}

void InputEvents::clear() {
    EventHeader* header = static_cast<EventHeader*>(data_);
    while (header) {
        EventHeader* next = header->next;
        free(header);
        header = next;
    }
    data_ = nullptr;
}

void InputEvents::operator=(const InputEvents& other) {
    if (data_ != other.data_) {
        clear();
        EventHeader* last = nullptr;
        for (EventHeader* it = static_cast<EventHeader*>(other.data_); it;
             it = it->next) {
            EventHeader* event = CopyEvent(it);
            if (last) {
                last->next = event;
            } else {
                data_ = event;
            }
            last = event;
        }
    }
}

// ================================================================================================
// InputEvents :: adding events.

void InputEvents::addMouseMove(int x, int y) {
    ValidateEvents(data_);
    AddEventT(data_, ET_MOUSE_MOVE, MouseMove{x, y});
    ValidateEvents(data_);
}

void InputEvents::addMousePress(Mouse::Code button, int x, int y, int keyflags,
                                bool doubleClick) {
    ValidateEvents(data_);
    AddEventT(data_, ET_MOUSE_PRESS,
              MousePress{button, x, y, keyflags, doubleClick, false});
    ValidateEvents(data_);
}

void InputEvents::addMouseRelease(Mouse::Code button, int x, int y,
                                  int keyflags) {
    ValidateEvents(data_);
    AddEventT(data_, ET_MOUSE_RELEASE,
              MouseRelease{button, x, y, keyflags, false});
    ValidateEvents(data_);
}

void InputEvents::addMouseScroll(bool up, int x, int y, int keyflags) {
    ValidateEvents(data_);
    AddEventT(data_, ET_MOUSE_SCROLL, MouseScroll{up, x, y, keyflags, false});
    ValidateEvents(data_);
}

void InputEvents::addKeyPress(Key::Code key, int keyflags, bool repeated) {
    ValidateEvents(data_);
    AddEventT(data_, ET_KEY_PRESS, KeyPress{key, keyflags, repeated, false});
    ValidateEvents(data_);
}

void InputEvents::addKeyRelease(Key::Code key, int keyflags) {
    ValidateEvents(data_);
    AddEventT(data_, ET_KEY_RELEASE, KeyRelease{key, keyflags, false});
    ValidateEvents(data_);
}

void InputEvents::addTextInput(const char* text) {
    ValidateEvents(data_);

    int numBytes = sizeof(TextInput) + strlen(text) + 1;

    TextInput* event =
        reinterpret_cast<TextInput*>(AddEvent(data_, ET_TEXT_INPUT, numBytes));
    event->handled = false;
    event->text = nullptr;
    WriteString(event + 1, text);

    SetTextInputPointers(*event);
    ValidateEvents(data_);
}

void InputEvents::addFileDrop(const char* const* files, int count, int x,
                              int y) {
    ValidateEvents(data_);

    int numBytes = sizeof(FileDrop) + count * sizeof(char*);
    for (int i = 0; i < count; ++i) {
        numBytes += strlen(files[i]) + 1;
    }
    FileDrop* event =
        reinterpret_cast<FileDrop*>(AddEvent(data_, ET_FILE_DROP, numBytes));
    event->handled = false;
    event->count = count;
    event->x = x;
    event->y = y;

    char* p = reinterpret_cast<char*>(event + 1);
    p += count * sizeof(char*);
    for (int i = 0; i < count; ++i) {
        p = WriteString(p, files[i]);
    }

    SetFileDropPointers(*event);
    ValidateEvents(data_);
}

void InputEvents::addWindowInactive() {
    ValidateEvents(data_);
    AddEvent(data_, ET_WINDOW_INACTIVE, 0);
    ValidateEvents(data_);
}

// ================================================================================================
// InputEvents :: iterating over events.

bool InputEvents::next(MouseMove*& it) const {
    ValidateEvents(data_, it);
    return ReadNext(data_, ET_MOUSE_MOVE, it);
}

bool InputEvents::next(MousePress*& it) const {
    ValidateEvents(data_, it);
    return ReadNext(data_, ET_MOUSE_PRESS, it);
}

bool InputEvents::next(MouseRelease*& it) const {
    ValidateEvents(data_, it);
    return ReadNext(data_, ET_MOUSE_RELEASE, it);
}

bool InputEvents::next(MouseScroll*& it) const {
    ValidateEvents(data_, it);
    return ReadNext(data_, ET_MOUSE_SCROLL, it);
}

bool InputEvents::next(KeyPress*& it) const {
    ValidateEvents(data_, it);
    return ReadNext(data_, ET_KEY_PRESS, it);
}

bool InputEvents::next(KeyRelease*& it) const {
    ValidateEvents(data_, it);
    return ReadNext(data_, ET_KEY_RELEASE, it);
}

bool InputEvents::next(TextInput*& it) const {
    ValidateEvents(data_, it);
    return ReadNext(data_, ET_TEXT_INPUT, it);
}

bool InputEvents::next(FileDrop*& it) const {
    ValidateEvents(data_, it);
    return ReadNext(data_, ET_FILE_DROP, it);
}

// ================================================================================================
// InputHandler :: handling events.

typedef void (*HandleEventFunction)(void*, InputHandler*);

static void HandleDummyFunction(void* p, InputHandler* h) {}

static void HandleMouseMove(void* p, InputHandler* h) {
    h->onMouseMove(*static_cast<MouseMove*>(p));
}

static void HandleMousePress(void* p, InputHandler* h) {
    h->onMousePress(*static_cast<MousePress*>(p));
}

static void HandleMouseRelease(void* p, InputHandler* h) {
    h->onMouseRelease(*static_cast<MouseRelease*>(p));
}

static void HandleMouseScroll(void* p, InputHandler* h) {
    h->onMouseScroll(*static_cast<MouseScroll*>(p));
}

static void HandleKeyPress(void* p, InputHandler* h) {
    h->onKeyPress(*static_cast<KeyPress*>(p));
}

static void HandleKeyRelease(void* p, InputHandler* h) {
    h->onKeyRelease(*static_cast<KeyRelease*>(p));
}

static void HandleTextInput(void* p, InputHandler* h) {
    h->onTextInput(*static_cast<TextInput*>(p));
}

static void HandleFileDrop(void* p, InputHandler* h) {
    h->onFileDrop(*static_cast<FileDrop*>(p));
}

static void HandleWindowInactive(void* p, InputHandler* h) {
    h->onWindowInactive();
}

static HandleEventFunction handleFunctions[NUM_EVENT_TYPES] = {
    HandleDummyFunction,  HandleMouseMove,   HandleMousePress,
    HandleMouseRelease,   HandleMouseScroll, HandleKeyPress,
    HandleKeyRelease,     HandleTextInput,   HandleFileDrop,
    HandleWindowInactive,
};

void InputHandler::handleInputs(InputEvents& events) {
    for (EventHeader* it = static_cast<EventHeader*>(events.data_); it;
         it = it->next) {
        handleFunctions[it->type](it + 1, this);
    }
}

// ================================================================================================
// InputHandler :: events structs.

void MousePress::setHandled() { handled = true; }

MousePress::MousePress(Mouse::Code code, int x, int y, int keyflags,
                       bool doubleClick, bool handled)
    : button(code),
      x(x),
      y(y),
      keyflags(keyflags),
      doubleClick(doubleClick),
      handled(handled) {}

}  // namespace Vortex
