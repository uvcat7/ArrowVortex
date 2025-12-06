#include <Core/GuiManager.h>

#include <Core/MapUtils.h>

namespace Vortex {

struct GuiManagerData {
    GuiWidget* mouseOver;
    GuiWidget* mouseCapture;
    GuiWidget* textCapture;

    Vector<GuiWidget*> mouseBlockers;

    std::map<GuiWidget*, std::string> widgetToIdMap;
    std::multimap<std::string, GuiWidget*> idToWidgetMap;
    std::map<std::string, GuiMain::CreateWidgetFunction> widgetFactory;
    std::map<const GuiWidget*, std::string> tooltipMap;
};

GuiManagerData* GM = nullptr;

template <typename T>
void Register(const char* id) {
    GuiManager::registerWidgetClass(id, [](GuiContext* gui) {
        return static_cast<GuiWidget*>(new T(gui));
    });
}

static void RegisterCommonWidgetClasses() {
    Register<WgLabel>("WgLabel");
    Register<WgButton>("WgButton");
    Register<WgCheckbox>("WgCheckbox");
    Register<WgSelectList>("WgSelectList");
    Register<WgDroplist>("WgDroplist");
    Register<WgLineEdit>("WgLineEdit");
    Register<WgSpinner>("WgSpinner");
    Register<WgSlider>("WgSlider");
    Register<WgScrollbarH>("WgScrollbarH");
    Register<WgScrollbarV>("WgScrollbarV");
}

void GuiManager::destroy() {
    delete GM;
    GM = nullptr;
}

void GuiManager::create() {
    GM = new GuiManagerData;

    GM->mouseOver = nullptr;
    GM->mouseCapture = nullptr;
    GM->textCapture = nullptr;

    RegisterCommonWidgetClasses();
}

void GuiManager::registerWidgetClass(const std::string& name,
                                     GuiMain::CreateWidgetFunction fun) {
    GM->widgetFactory[name] = fun;
}

GuiWidget* GuiManager::createWidget(const std::string& name, GuiContext* gui) {
    auto it = Map::findVal(GM->widgetFactory, name);
    return (it) ? (*it)(gui) : nullptr;
}

void GuiManager::removeWidget(GuiWidget* w) {
    if (GM->mouseOver == w) GM->mouseOver = nullptr;
    if (GM->mouseCapture == w) GM->mouseCapture = nullptr;
    if (GM->textCapture == w) GM->textCapture = nullptr;

    GM->tooltipMap.erase(w);
    Map::eraseVals(GM->idToWidgetMap, w);
    GM->widgetToIdMap.erase(w);
    GM->mouseBlockers.erase_values(w);
}

void GuiManager::setWidgetId(GuiWidget* w, const std::string& id) {
    Map::eraseVals(GM->idToWidgetMap, w);
    GM->widgetToIdMap.erase(w);
    if (id.length()) {
        GM->idToWidgetMap.insert({id, w});
        GM->widgetToIdMap.insert({w, id});
    }
}

GuiWidget* GuiManager::findWidget(const std::string& id, GuiContext* gui) {
    auto& map = GM->idToWidgetMap;
    auto r = map.equal_range(id);
    if (gui) {
        while (r.first != r.second && r.first->second->getGui() != gui)
            ++r.first;
    }
    return (r.first != r.second) ? r.first->second : nullptr;
}

void GuiManager::setTooltip(const GuiWidget* w, const std::string& str) {
    GM->tooltipMap[w] = str;
}

std::string GuiManager::getTooltip(const GuiWidget* w) {
    auto it = Map::findVal(GM->tooltipMap, w);
    return it ? *it : std::string();
}

void GuiManager::blockMouseOver(GuiWidget* w) {
    GM->mouseBlockers.push_back(w);
}

void GuiManager::unblockMouseOver(GuiWidget* w) {
    GM->mouseBlockers.erase_values(w);
}

void GuiManager::makeMouseOver(GuiWidget* w) {
    if (GM->mouseBlockers.empty() && !GM->mouseOver) {
        GM->mouseOver = w;
    }
}

void GuiManager::startCapturingMouse(GuiWidget* w) {
    if (GM->mouseCapture != w) {
        if (GM->textCapture && GM->textCapture != w) {
            GM->textCapture->onTextCaptureLost();
            GM->textCapture = nullptr;
        }
        if (GM->mouseCapture) {
            GM->mouseCapture->onMouseCaptureLost();
        }
        GM->mouseCapture = w;
    }
}

void GuiManager::captureText(GuiWidget* w) {
    if (GM->textCapture != w) {
        if (GM->textCapture) {
            GM->textCapture->onTextCaptureLost();
        }
        GM->textCapture = w;
    }
}

void GuiManager::stopCapturingMouse(GuiWidget* w) {
    if (GM->mouseCapture == w) {
        if (GM->mouseCapture) {
            GM->mouseCapture->onMouseCaptureLost();
        }
        GM->mouseCapture = nullptr;
    }
}

void GuiManager::stopCapturingText(GuiWidget* w) {
    if (GM->textCapture == w) {
        if (GM->textCapture) {
            GM->textCapture->onTextCaptureLost();
        }
        GM->textCapture = nullptr;
    }
}

void GuiManager::startFrame(float dt) {
    GM->mouseBlockers.clear();

    GM->mouseOver = GM->mouseCapture;
}

GuiWidget* GuiManager::getMouseOver() { return GM->mouseOver; }

GuiWidget* GuiManager::getMouseCapture() { return GM->mouseCapture; }

GuiWidget* GuiManager::getTextCapture() { return GM->textCapture; }

bool GuiManager::isCapturingMouse() {
    return (GM->mouseCapture != nullptr) || (GM->mouseOver != nullptr);
}

bool GuiManager::isCapturingText() { return GM->textCapture != nullptr; }

};  // namespace Vortex
