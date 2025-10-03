#include <Dialogs/TempoBreakdown.h>

#include <Core/StringUtils.h>
#include <Core/Draw.h>

#include <Managers/TempoMan.h>
#include <Managers/SimfileMan.h>

#include <Editor/View.h>

#include <Simfile/SegmentList.h>
#include <Simfile/SegmentGroup.h>

namespace Vortex {

// ================================================================================================
// ChartList

static int GetTempoListH() {
    auto segments = gTempo->getSegments();
    int h = 0;
    if (segments) {
        for (const auto& segment : *segments) {
            if (segment.size()) {
                h += 26 + segment.size() * 16;
            }
        }
    }
    return max(h, 16);
}

struct DialogTempoBreakdown::TempoList : public WgScrollRegion {
    ~TempoList() override = default;

    explicit TempoList(GuiContext* gui) : WgScrollRegion(gui) {
        setScrollType(SCROLL_NEVER, SCROLL_WHEN_NEEDED);
    }

    void onUpdateSize() override {
        scroll_height_ = GetTempoListH();
        ClampScrollPositions();
    }

    const Segment* getSegmentUnderMouse(vec2i position) {
        int my = position.y - 16;
        int y = rect_.y - scroll_position_y_;

        if (!isMouseOver() || my < y || position.x > rect_.x + getViewWidth())
            return nullptr;

        auto segments = gTempo->getSegments();
        for (const auto& segment : *segments) {
            if (segment.size()) {
                y += 22;
                auto seg = segment.begin(), segEnd = segment.end();
                while (seg != segEnd && y < my) {
                    y += 16, ++seg;
                }
                y += 4;
                if (y >= my) return seg.ptr;
            }
        }

        return nullptr;
    }

    void onMousePress(MousePress& evt) override {
        if (isMouseOver()) {
            if (isEnabled() && evt.button == Mouse::LMB && evt.unhandled()) {
                auto seg = getSegmentUnderMouse(gui_->getMousePos());
                if (seg) {
                    gView->setCursorRow(seg->row);
                    evt.setHandled();
                }
            }
        }
        WgScrollRegion::onMousePress(evt);
    }

    void onDraw() override {
        if (gSimfile->isClosed()) return;

        int x = rect_.x;
        int y = rect_.y - scroll_position_y_;
        recti view = {rect_.x, rect_.y, getViewWidth(), getViewHeight()};

        TextStyle style;
        style.textFlags = 0;

        Renderer::pushScissorRect(view);

        Draw::fill({view.x + view.w / 2, view.y, 1, view.h}, Color32(26));

        vec2i m = gui_->getMousePos();
        auto segments = gTempo->getSegments();
        for (const auto& segment : *segments) {
            if (segment.size()) {
                auto meta = Segment::meta[segment.type()];
                auto seg = segment.begin(), segEnd = segment.end();

                Draw::fill({x, y, view.w, 20}, Color32(26));
                Text::arrange(Text::MC, style, meta->plural);
                Text::draw({x, y, view.w, 20});
                y += 22;

                while (seg != segEnd && y < view.y - 20) {
                    y += 16, ++seg;
                }
                while (seg != segEnd && y < view.y + view.h + 20) {
                    if (isMouseOver() && m.y >= y && m.y < y + 16)
                        Draw::fill({x, y - 1, view.w, 20}, Color32(34));

                    std::string str = Str::val(seg->row * BEATS_PER_ROW, 3, 3);
                    Text::arrange(Text::MR, style, str.c_str());
                    Text::draw(vec2i{x + view.w / 2 - 6, y + 8});

                    str = meta->getDescription(seg.ptr);
                    Text::arrange(Text::ML, style, str.c_str());
                    Text::draw(vec2i{x + view.w / 2 + 6, y + 8});

                    y += 16, ++seg;
                }

                y += 4;
            }
        }

        Renderer::popScissorRect();

        WgScrollRegion::onDraw();
    }

};  // TimingData

// ================================================================================================
// DialogTempoBreakdown

DialogTempoBreakdown::~DialogTempoBreakdown() { delete myList; }

DialogTempoBreakdown::DialogTempoBreakdown() {
    setTitle("TEMPO BREAKDOWN");
    setWidth(200);

    setMinimumHeight(32);
    setResizeable(false, true);

    myList = new TempoList(getGui());
}

void DialogTempoBreakdown::onUpdateSize() {
    myList->updateSize();
    int h = myList->getScrollHeight();
    setMinimumHeight(min(64, h));
    setMaximumHeight(min(1024, h));
}

void DialogTempoBreakdown::onTick() {
    myList->arrange(getInnerRect());
    myList->tick();
}

void DialogTempoBreakdown::onDraw() { myList->draw(); }

};  // namespace Vortex