#include <Dialogs/LabelBreakdown.h>

#include <System/System.h>

#include <Core/StringUtils.h>
#include <Core/Canvas.h>

#include <Managers/TempoMan.h>
#include <Managers/SimfileMan.h>

#include <Editor/Common.h>
#include <Editor/View.h>

#include <Simfile/SegmentGroup.h>

#define ITEM_H static_cast<int>(20 * gSystem->getScaleFactor())
#define ITEM_W static_cast<int>(74 * gSystem->getScaleFactor())

namespace Vortex {

// ================================================================================================
// LabelButton
enum DisplayType { TIMESTAMP, BEAT, ROW };

struct DialogLabelBreakdown::LabelButton : public GuiWidget {
    LabelButton(GuiContext* gui, TileRect2* bar, int row, std::string time,
                std::string text)
        : GuiWidget(gui),
          myBar(bar),
          myRow(row),
          myDisplayTime(time),
          myDisplayText(text) {}

    void onMousePress(MousePress& evt) override {
        if (isMouseOver()) {
            if (isEnabled() && evt.button == Mouse::LMB && evt.unhandled()) {
                startCapturingMouse();
                gView->setCursorRow(myRow);
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
            GuiMain::setTooltip("Jump to label");
        }
    }

    void onDraw() override {
        recti r = rect_;

        TextStyle textStyle;
        textStyle.textFlags = Text::ELLIPSES;

        // Draw the button graphic.
        auto& button = GuiDraw::getButton();
        button.base.draw(r, 0);

        // Draw the button text.
        recti left = {rect_.x, rect_.y, ITEM_W, ITEM_H};

        uint32_t color = RGBAtoColor32(139, 148, 148, 255);
        myBar->draw(left, 0, color);

        Text::arrange(Text::MR, myDisplayTime.c_str());
        Text::draw(vec2i{left.x + left.w - 6, left.y + ITEM_H / 2});

        Text::arrange(Text::ML, textStyle, myDisplayText.c_str());
        Text::draw(vec2i{left.x + left.w + 6, left.y + ITEM_H / 2});

        // Interaction effects.
        if (isCapturingMouse()) {
            button.pressed.draw(r, 0);
        } else if (isMouseOver()) {
            button.hover.draw(r, 0);
        }
    }

    int myRow;
    std::string myDisplayTime;
    std::string myDisplayText;
    TileRect2* myBar;
};

// ================================================================================================
// LabelList

struct DialogLabelBreakdown::LabelList : public WgScrollRegion {
    Vector<LabelButton*> myButtons;
    TileRect2 myButtonTex;
    int myDisplayType;

    ~LabelList() override {
        for (auto button : myButtons) {
            delete button;
        }
    }

    explicit LabelList(GuiContext* gui) : WgScrollRegion(gui) {
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
        scroll_height_ = max(static_cast<int>(24 * gSystem->getScaleFactor()),
                             myButtons.size() * (ITEM_H + 1));
        ClampScrollPositions();
    }

    void onTick() override {
        PreTick();

        int viewW = getViewWidth() - 2 * is_vertical_scrollbar_active_;

        // Update the properties of each button.
        int y = rect_.y - scroll_position_y_;
        for (auto button : myButtons) {
            button->arrange({rect_.x, y, viewW, ITEM_H});
            button->tick();
            y += ITEM_H + 1;
        }

        PostTick();
    }

    void onDraw() override {
        if (gSimfile->isClosed()) return;

        TextStyle textStyle;
        int w = getViewWidth() - 2 * is_vertical_scrollbar_active_;
        int h = getViewHeight();
        int x = rect_.x;
        int y = rect_.y - scroll_position_y_;
        const Style* style = nullptr;
        int chartIndex = 0;

        Renderer::pushScissorRect({rect_.x, rect_.y, w, h});
        if (myButtons.empty()) {
            Text::arrange(Text::MC, textStyle, "- no labels -");
            Text::draw(vec2i{x + w / 2, y + 16});
        } else
            for (auto button : myButtons) {
                button->draw();
                y += ITEM_H + 1;
            }
        Renderer::popScissorRect();

        WgScrollRegion::onDraw();
    }

    void updateButtons() {
        if (gSimfile->isClosed()) return;

        while (myButtons.size() > 0) {
            delete myButtons.back();
            myButtons.pop_back();
        }

        auto segs = gTempo->getSegments();
        auto& list = segs->getList<Label>();
        auto seg = list.begin(), segEnd = list.end();
        while (seg != segEnd) {
            std::string time = displayTime(seg->row);
            std::string text = segs->getRow<Label>(seg->row).str;
            myButtons.push_back(
                new LabelButton(getGui(), &myButtonTex, seg->row, time, text));
            ++seg;
        }
    }

    void setDisplayType(int type) {
        myDisplayType = type;
        updateButtons();
    }

    std::string displayTime(int row) const {
        if (myDisplayType == BEAT) {
            return Str::val(static_cast<double>(row) / ROWS_PER_BEAT, 3, 3);
        } else if (myDisplayType == ROW) {
            return Str::val(row);
        } else {
            double time = gTempo->rowToTime(row);
            return Str::formatTime(time);
        }
    }

    void copyLabels() {
        std::string out;
        for (auto button : myButtons) {
            out += button->myDisplayTime;
            out += ": ";
            out += button->myDisplayText;
            out += "\n";
        }
        if (out.empty()) {
            HudInfo("%s", "There are no labels to copy...");
        } else {
            gSystem->setClipboardText(out);
            HudInfo("%s", "Label list copied to clipboard.");
        }
    }
};

// ================================================================================================
// DialogLabelList

DialogLabelBreakdown::~DialogLabelBreakdown() = default;

DialogLabelBreakdown::DialogLabelBreakdown() {
    setTitle("LABELS");
    float scale = gSystem->getScaleFactor();
    setWidth(static_cast<int>(scale * 298));
    setMinimumWidth(static_cast<int>(scale * 298));
    setMinimumHeight(static_cast<int>(scale * 152));
    setMaximumWidth(static_cast<int>(scale * 1024));
    setMaximumHeight(static_cast<int>(scale * 1024));
    setResizeable(true, true);

    myDisplayType = TIMESTAMP;
    myCreateWidgets();
    onChanges(VCM_ALL_CHANGES);
    clear();
}

void DialogLabelBreakdown::clear() { myLabelText.clear(); }

void DialogLabelBreakdown::myCreateWidgets() {
    myLayout.row().col(44).col(234, true);

    WgLineEdit* text = myLayout.add<WgLineEdit>("Label");
    text->text.bind(&myLabelText);
    text->setMaxLength(1000);
    text->onChange.bind(this, &DialogLabelBreakdown::onChange);
    text->setTooltip("Label text");

    myLayout.row().col(284, true);
    myList = myLayout.add<LabelList>();

    myLayout.row().col(74).col(210, true);
    myDisplayTypeList = myLayout.add<WgCycleButton>();
    myDisplayTypeList->addItem("Time");
    myDisplayTypeList->addItem("Beat");
    myDisplayTypeList->addItem("Row");
    myDisplayTypeList->value.bind(&myDisplayType);
    myDisplayTypeList->onChange.bind(this,
                                     &DialogLabelBreakdown::mySetDisplayType);
    myDisplayTypeList->setTooltip("Change the unit used for timestamps.");

    WgButton* copy = myLayout.add<WgButton>();
    copy->text.set("{g:copy}");
    copy->setTooltip("Copy labels to clipboard");
    copy->onPress.bind(this, &DialogLabelBreakdown::myCopyLabels);
}

void DialogLabelBreakdown::onChanges(int changes) {
    if (changes & VCM_FILE_CHANGED) {
        clear();
        if (gSimfile->isOpen()) {
            for (auto w : myLayout) w->setEnabled(true);
        } else {
            for (auto w : myLayout) w->setEnabled(false);
        }
    }
    if (changes & VCM_TEMPO_CHANGED) {
        myList->updateButtons();
    }
}

void DialogLabelBreakdown::onUpdateSize() { myLayout.onUpdateSize(); }

void DialogLabelBreakdown::onTick() {
    recti bounds = getInnerRect();
    myList->setHeight(bounds.h -
                      static_cast<int>(58 * gSystem->getScaleFactor()));

    if (gSimfile->isOpen()) {
        int row = gView->getCursorRow();
        auto segs = gTempo->getSegments();

        myLabelText = segs->getRow<Label>(row).str;
    }

    EditorDialog::onTick();
}

void DialogLabelBreakdown::onChange() {
    if (gSimfile->isClosed()) return;

    int row = gView->getCursorRow();
    if (strpbrk(myLabelText.c_str(), ";,=") != nullptr) {
        HudWarning(
            "A Label cannot contain commas, semicolons, or equal signs; they "
            "will be replaced with underscores.");
        Str::replace(myLabelText, ",", "_");
        Str::replace(myLabelText, ";", "_");
        Str::replace(myLabelText, "=", "_");
    }
    gTempo->addSegment(Label(row, myLabelText));
}

void DialogLabelBreakdown::mySetDisplayType() {
    myList->setDisplayType(myDisplayType);
}

void DialogLabelBreakdown::myCopyLabels() { myList->copyLabels(); }
};  // namespace Vortex
