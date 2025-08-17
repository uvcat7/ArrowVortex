#include <Dialogs/ChartList.h>

#include <Core/StringUtils.h>
#include <Core/Gui.h>
#include <Core/Draw.h>
#include <Core/WidgetsLayout.h>
#include <Core/GuiDraw.h>
#include <Core/Canvas.h>

#include <Managers/MetadataMan.h>
#include <Managers/StyleMan.h>
#include <Managers/ChartMan.h>
#include <Managers/SimfileMan.h>

#include <Editor/Common.h>

namespace Vortex {

// ================================================================================================
// ChartButton

struct DialogChartList::ChartButton : public GuiWidget {
    ChartButton(GuiContext* gui, TileRect2* bar, int index)
        : GuiWidget(gui), myBar(bar), myChartIndex(index) {}

    void onMousePress(MousePress& evt) override {
        if (isMouseOver()) {
            if (isEnabled() && evt.button == Mouse::LMB && evt.unhandled()) {
                startCapturingMouse();
                gSimfile->openChart(myChartIndex);
            }
            evt.setHandled();
        }
    }

    void onMouseRelease(MouseRelease& evt) override {
        if (isCapturingMouse() && evt.button == Mouse::LMB) {
            stopCapturingMouse();
        }
    }

    void onTick() override {
        GuiWidget::onTick();
        if (isMouseOver()) {
            GuiMain::setTooltip("Open this chart in the editor");
        }
    }

    void onDraw() override {
        recti r = rect_;

        TextStyle textStyle;
        textStyle.textFlags = Text::ELLIPSES;

        // Draw the button graphic.
        auto& button = GuiDraw::getButton();
        button.base.draw(r, 0);

        const Chart* chart = gSimfile->getChart(myChartIndex);
        if (chart) {
            // Draw the left-side colored bar with difficulty and meter.
            recti left = {rect_.x, rect_.y, min(r.w, 128), 20};
            uint32_t color = ToColor(chart->difficulty);
            myBar->draw(left, 0, color);

            Text::arrange(Text::ML, textStyle,
                          GetDifficultyName(chart->difficulty));
            Text::draw(vec2i{left.x + 4, left.y + 10});

            Text::arrange(Text::MR, textStyle, Str::val(chart->meter).c_str());
            Text::draw(vec2i{left.x + left.w - 6, left.y + 10});

            // Draw the right-side step artist and note count.
            if (r.w > left.w) {
                std::string stepCount = Str::val(chart->stepCount());

                int maxW = r.w - left.w - 8;
                recti right = {rect_.x + left.w, rect_.y, rect_.w - left.w, 20};
                Text::arrange(Text::MR, textStyle, maxW, stepCount.c_str());
                Text::draw(vec2i{right.x + right.w - 6, right.y + 10});

                maxW = right.w - Text::getSize().x - 16;
                Text::arrange(Text::ML, textStyle, maxW, chart->artist.c_str());
                Text::draw(vec2i{right.x + 6, right.y + 10});
            }
        }

        // Interaction effects.
        if (isCapturingMouse()) {
            button.pressed.draw(r, 0);
        } else if (isMouseOver()) {
            button.hover.draw(r, 0);
        }
    }

    int myChartIndex;
    TileRect2* myBar;
};

// ================================================================================================
// ChartList

static int GetChartListH() {
    int h = 0;
    const Style* style = nullptr;
    for (int i = 0; i < gSimfile->getNumCharts(); ++i) {
        auto chart = gSimfile->getChart(i);
        if (style != chart->style) {
            style = chart->style;
            h += 24;
        }
        h += 21;
    }
    return max(24, h);
}

struct DialogChartList::ChartList : public WgScrollRegion {
    Vector<ChartButton*> myButtons;
    TileRect2 myButtonTex;

    ~ChartList() override {
        for (auto button : myButtons) {
            delete button;
        }
    }

    explicit ChartList(GuiContext* gui) : WgScrollRegion(gui) {
        setScrollType(SCROLL_NEVER, SCROLL_WHEN_NEEDED);

        Canvas c(32, 16);

        c.setColor(ToColor(0.8f, 0.8f));
        c.box(1, 1, 15, 15, 0.0f);
        c.box(17, 1, 31, 15, 3.5f);
        c.setColor(ToColor(0.6f, 0.8f));
        c.box(2, 2, 14, 14, 0.0f);
        c.box(18, 2, 30, 14, 2.5f);

        myButtonTex.texture = c.createTexture();
        myButtonTex.border = 4;

        updateButtons();
    }

    void onUpdateSize() override {
        scroll_height_ = GetChartListH();
        ClampScrollPositions();
    }

    void onTick() override {
        PreTick();

        int viewW = getViewWidth() - 2 * is_vertical_scrollbar_active_;

        // Update the properties of each button.
        int y = rect_.y - scroll_position_y_;
        const Style* style = nullptr;
        for (int i = 0; i < myButtons.size(); ++i) {
            auto button = myButtons[i];
            auto chart = gSimfile->getChart(i);
            if (chart && style != chart->style) {
                style = chart->style;
                y += 24;
            }
            button->arrange({rect_.x, y, viewW, 20});
            button->tick();
            y += 21;
        }

        PostTick();
    }

    void onDraw() override {
        TextStyle textStyle;
        int w = getViewWidth() - 2 * is_vertical_scrollbar_active_;
        int h = getViewHeight();
        int x = rect_.x;
        int y = rect_.y - scroll_position_y_;
        const Style* style = nullptr;
        int chartIndex = 0;

        Renderer::pushScissorRect({rect_.x, rect_.y, w, h});
        if (myButtons.empty()) {
            Text::arrange(Text::MC, textStyle, "- no charts -");
            Text::draw(vec2i{x + w / 2, y + rect_.h / 2});
        } else
            for (auto button : myButtons) {
                auto chart = gSimfile->getChart(chartIndex++);
                if (chart && style != chart->style) {
                    style = chart->style;
                    Text::arrange(Text::MC, textStyle, style->name.c_str());
                    Text::draw(vec2i{x + w / 2, y + 10});
                    textStyle.textColor = Colors::white;
                    y += 24;
                }
                button->draw();
                y += 21;
            }
        Renderer::popScissorRect();

        WgScrollRegion::onDraw();
    }

    void updateButtons() {
        int numCharts = gSimfile->getNumCharts();
        while (myButtons.size() < numCharts) {
            myButtons.push_back(
                new ChartButton(getGui(), &myButtonTex, myButtons.size()));
        }
        while (myButtons.size() > numCharts) {
            delete myButtons.back();
            myButtons.pop_back();
        }
    }
};

// ================================================================================================
// DialogChartList

DialogChartList::~DialogChartList() { delete myList; }

DialogChartList::DialogChartList() {
    setTitle("LIST OF CHARTS");

    setWidth(320);

    setMinimumWidth(128);
    setMaximumWidth(1024);
    setMinimumHeight(16);

    setResizeable(true, true);

    myList = new ChartList(getGui());
}

void DialogChartList::onChanges(int changes) {
    if (changes & VCM_CHART_LIST_CHANGED) {
        myList->updateButtons();
        int h = GetChartListH();
        h = min(h, getGui()->getView().h - 128);
        h = max(h, 32);
        setHeight(h);
    }
}

void DialogChartList::onUpdateSize() {
    myList->updateSize();
    int h = myList->getScrollHeight();
    setMinimumHeight(min(64, h));
    setMaximumHeight(min(1024, h));
}

void DialogChartList::onTick() {
    myList->arrange(getInnerRect());
    myList->tick();
}

void DialogChartList::onDraw() { myList->draw(); }

};  // namespace Vortex
