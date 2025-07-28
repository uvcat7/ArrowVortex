#include <Core/Gui.h>

#include <Core/Utils.h>
#include <Core/StringUtils.h>
#include <Core/Texture.h>
#include <Core/Renderer.h>
#include <Core/FontManager.h>
#include <Core/TextLayout.h>
#include <Core/TextDraw.h>
#include <Core/Shader.h>
#include <Core/GuiContext.h>
#include <Core/Widgets.h>
#include <Core/GuiContext.h>
#include <Core/GuiManager.h>
#include <Core/GuiDraw.h>

#include <System/Debug.h>

namespace Vortex {
namespace {

// ================================================================================================
// Goo data.

struct GuiMainData {
    vec2i viewSize;
    vec2i mousePos;

    GuiMain::ClipboardSetFunction clipboardSet;
    GuiMain::ClipboardGetFunction clipboardGet;
    std::string clipboardText;

    std::string tooltipPreviousText;
    std::string tooltipText;
    vec2i tooltipPos;
    float tooltipTimer;

    Cursor::Icon cursorIcon;
};

static GuiMainData* GUI = nullptr;

};  // anonymous namespace

// ================================================================================================
// GuiMain :: initialization / shutdown.

void GuiMain::init() {
    if (GUI) return;

    GUI = new GuiMainData;

    GUI->viewSize = {640, 480};
    GUI->mousePos = {0, 0};

    TextureManager::create();
    FontManager::create();
    Renderer::create();
    TextLayout::create();
    TextDraw::create();
    GuiManager::create();
    GuiDraw::create();
}

void GuiMain::shutdown() {
    if (!GUI) return;

    GuiDraw::destroy();
    GuiManager::destroy();
    TextDraw::destroy();
    TextLayout::destroy();
    FontManager::destroy();
    TextureManager::destroy();
    Renderer::destroy();

    delete GUI;
    GUI = nullptr;
}

// ================================================================================================
// GuiMain :: Tooltip.

static void handleTooltip() {
    if (GUI->tooltipPreviousText == GUI->tooltipText) {
        if (GUI->tooltipText.length() && GUI->tooltipTimer > 1.0f) {
            TextStyle style;

            int alpha = clamp((int)(GUI->tooltipTimer * 1000 - 1000), 0, 255);

            style.textColor = Color32(0, alpha);
            style.shadowColor = Colors::blank;
            style.fontSize = 11;

            Text::arrange(Text::TL, style, 256, GUI->tooltipText.c_str());
            vec2i textSize = Text::getSize();

            vec2i& pos = GUI->tooltipPos;
            if (pos.x == INT_MAX) {
                pos.x = GUI->mousePos.x + 2;
                if (GUI->mousePos.y > GUI->viewSize.y - 24) {
                    pos.y = GUI->mousePos.y - 18;
                } else {
                    pos.y = GUI->mousePos.y + 24;
                }
            }

            pos.x = clamp(pos.x, 4, GUI->viewSize.x - textSize.x - 4);
            pos.y = clamp(pos.y, 4, GUI->viewSize.y - textSize.y - 4);

            recti textBox = {pos.x, pos.y, textSize.x, textSize.y};
            recti box = Expand(textBox, 3);

            Draw::fill(box, Color32(191, alpha));
            Draw::outline(box, Color32(230, alpha));

            Text::draw(textBox);
        }
    } else if (GUI->tooltipText.length()) {
        GUI->tooltipPos = {INT_MAX, INT_MAX};
        GUI->tooltipTimer = 0.0f;
    }
}

// ================================================================================================
// GuiMain :: frame start / frame end.

void GuiMain::frameStart(float dt, InputEvents& events) {
    if (!GUI) return;

    for (MouseMove* move = nullptr; events.next(move);) {
        GUI->mousePos = {move->x, move->y};
    }

    GUI->cursorIcon = Cursor::ARROW;

    GUI->tooltipPreviousText = GUI->tooltipText;
    GUI->tooltipText = std::string();
    GUI->tooltipTimer += dt;

    Renderer::startFrame();
    FontManager::startFrame(dt);
    GuiManager::startFrame(dt);
}

void GuiMain::frameEnd() {
    if (!GUI) return;

    handleTooltip();

    TextLayout::endFrame();
    Renderer::endFrame();
}

// ================================================================================================
// Goo :: API functions.

void GuiMain::registerWidgetClass(const char* id, CreateWidgetFunction fun) {
    return GuiManager::registerWidgetClass(id, fun);
}

GuiWidget* GuiMain::createWidget(const char* id, GuiContext* gui) {
    return GuiManager::createWidget(id, gui);
}

void GuiMain::setClipboardFunctions(ClipboardGetFunction get,
                                    ClipboardSetFunction set) {
    GUI->clipboardGet = get;
    GUI->clipboardSet = set;
}

void GuiMain::setClipboardText(std::string text) {
    GUI->clipboardText = text;
    if (GUI->clipboardSet) (*GUI->clipboardSet)(GUI->clipboardText);
}

std::string GuiMain::getClipboardText() {
    if (GUI->clipboardGet) GUI->clipboardText = (*GUI->clipboardGet)();
    return GUI->clipboardText;
}

void GuiMain::setCursorIcon(Cursor::Icon icon) { GUI->cursorIcon = icon; }

Cursor::Icon GuiMain::getCursorIcon() { return GUI->cursorIcon; }

void GuiMain::setTooltip(std::string text) { GUI->tooltipText = text; }

std::string GuiMain::getTooltip() { return GUI->tooltipText; }

bool GuiMain::isCapturingMouse() { return GuiManager::isCapturingMouse(); }

bool GuiMain::isCapturingText() { return GuiManager::isCapturingText(); }

void GuiMain::setViewSize(int width, int height) {
    GUI->viewSize = {width, height};
}

vec2i GuiMain::getViewSize() { return GUI->viewSize; }

};  // namespace Vortex
