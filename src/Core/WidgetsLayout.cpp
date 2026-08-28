#include <Core/WidgetsLayout.h>
#include <Core/Utils.h>
#include <Core/VectorUtils.h>

#include <System/System.h>

#include <climits>
#include <math.h>

namespace Vortex {

// ================================================================================================
// RowLayout.

struct RowLayout::Col {
    uint32_t width : 30;
    uint32_t adjust : 1;
    uint32_t expand : 1;
};

struct RowLayout::Row {
    Row() = default;

    uint32_t expand : 1 = 0;

    std::vector<RowLayout::Col> cols;
    std::vector<GuiWidget*> widgets;
};

RowLayout::~RowLayout() {
    for (auto widget : widget_list_) {
        delete widget;
    }
}

RowLayout::RowLayout(GuiContext* gui, int spacing)
    : GuiWidget(gui), row_spacing_(spacing) {
    Row row;
    row.cols.push_back({0, 1, 0});
    row_list_.emplace_back(row);
}

void RowLayout::onUpdateSize() {
    int y = 0;

    width_ = 0;
    height_ = 0;

    for (auto& row : row_list_) {
        auto widget = row.widgets.begin();
        int numWidgets = row.widgets.size();
        int numCols = row.cols.size();
        auto cols = row.cols.begin();

        for (int c = 0; c < numCols; ++c) {
            if (cols[c].adjust) {
                cols[c].width = 0;
            }
        }

        int h = 0, x = 0, c = 0;
        for (int i = 0; i < numWidgets; ++i, ++c, ++widget) {
            if (c == numCols) {
                y += h;
                height_ = y;
                y += row_spacing_;
                x = 0, c = 0, h = 0;
            }

            if (*widget) {
                (*widget)->updateSize();
                vec2i size = (*widget)->getSize();
                h = std::max(h, size.y);
                if (cols[c].adjust) {
                    cols[c].width =
                        std::max(cols[c].width, static_cast<uint32_t>(size.x));
                }
            }

            x += cols[c].width;
            width_ = std::max(width_, x);
            x += row_spacing_;
        }

        y += h;
        height_ = y;
        y += row_spacing_;
    }
}

void RowLayout::onArrange(recti r) {
    int y = r.y;

    int extraW = std::max(0, r.w - width_);

    for (auto& row : row_list_) {
        bool expanded = false;

        auto widget = row.widgets.begin();
        int numWidgets = row.widgets.size();
        int numCols = row.cols.size();
        auto cols = row.cols.begin();

        int h = 0, x = r.x;
        for (int i = 0, c = 0; i < numWidgets; ++i, ++c, ++widget) {
            if (c == numCols) {
                y += h + row_spacing_;
                x = r.x, c = 0, h = 0;
                expanded = false;
            }

            int colW = cols[c].width;
            if (cols[c].expand && !expanded) {
                colW += extraW;
                expanded = true;
            }

            if (*widget) {
                vec2i size = (*widget)->getSize();
                (*widget)->arrange({x, y, colW, size.y});
                h = std::max(h, size.y);
            }

            x += cols[c].width + row_spacing_;
        }

        y += h + row_spacing_;
    }
}

void RowLayout::onTick() {
    FOR_VECTOR_REVERSE(widget_list_, i) { widget_list_[i]->tick(); }
}

void RowLayout::onDraw() {
    for (GuiWidget* widget : widget_list_) {
        widget->draw();
    }
}

void RowLayout::add(GuiWidget* widget) {
    widget_list_.emplace_back(widget);
    row_list_.back().widgets.emplace_back(widget);
}

void RowLayout::addBlank() { row_list_.back().widgets.emplace_back(nullptr); }

RowLayout& RowLayout::row(bool expand) {
    if (row_list_.back().widgets.empty()) row_list_.pop_back();

    Row new_row = {};
    new_row.expand = expand;
    row_list_.emplace_back(new_row);

    return *this;
}

RowLayout& RowLayout::col(bool expand) { return col(INT_MAX, expand); }

RowLayout& RowLayout::col(int w, bool expand) {
    auto& cols = row_list_.back().cols;
    cols.push_back({(w == INT_MAX)
                        ? 0
                        : static_cast<uint32_t>(w * gSystem->getScaleFactor()),
                    w == INT_MAX, expand == true});
    Col& col = cols.back();
    return *this;
}

std::vector<GuiWidget*>::const_iterator RowLayout::begin() {
    return widget_list_.begin();
}

std::vector<GuiWidget*>::const_iterator RowLayout::end() {
    return widget_list_.end();
}

};  // namespace Vortex
