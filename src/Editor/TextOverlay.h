#pragma once

#include <Core/Input.h>

namespace Vortex {

struct InfoBox {
    InfoBox();
    virtual ~InfoBox();
    virtual void draw(recti r) = 0;
    virtual int height() = 0;
};

struct InfoBoxWithProgress : public InfoBox {
    void draw(recti r);
    int height();
    void setProgress(double rate);
    void setTime(double seconds);
    std::string left, right;
};

struct TextOverlay : public InputHandler {
    enum Mode { HUD, SHORTCUTS, MESSAGE_LOG, DEBUG_LOG, ABOUT };
    enum MessageType { NOTE, INFO, WARNING, ERROR };

    static void create();
    static void destroy();

    virtual void tick() = 0;
    virtual void draw() = 0;

    virtual void addMessage(const char* str, MessageType type) = 0;
    virtual void show(Mode mode) = 0;

    virtual bool isOpen() = 0;
};

extern TextOverlay* gTextOverlay;

};  // namespace Vortex
