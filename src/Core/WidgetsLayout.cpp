#include <Core/WidgetsLayout.h>
#include <Core/Utils.h>
#include <Core/VectorUtils.h>

#include <math.h>

namespace Vortex {

// ================================================================================================
// RowLayout.

struct RowLayout::Col
{
	uint width : 30;
	uint adjust : 1;
	uint expand : 1;
};

struct RowLayout::Row
{
	Row() : expand(0) {}

	uint expand : 1;

	Vector<RowLayout::Col> cols;
	Vector<GuiWidget*> widgets;
};

RowLayout::~RowLayout()
{
	for(auto widget : widgetList_)
	{
		delete widget;
	}
}

RowLayout::RowLayout(GuiContext* gui, int spacing)
	: GuiWidget(gui)
	, rowSpacing_(spacing)
{
	Row row;
	row.cols.push_back({0, 1, 0});
	rowLayouts_.push_back(row);
}

void RowLayout::onUpdateSize()
{
	int y = 0;

	width_ = 0;
	height_ = 0;

	for(auto& row : rowLayouts_)
	{
		GuiWidget** widget = row.widgets.begin();
		int numWidgets = row.widgets.size();
		int numCols = row.cols.size();
		Col* cols = row.cols.begin();

		for(int c = 0; c < numCols; ++c)
		{
			if(cols[c].adjust)
			{
				cols[c].width = 0;
			}
		}

		int h = 0, x = 0, c = 0;
		for(int i = 0; i < numWidgets; ++i, ++c, ++widget)
		{
			if(c == numCols)
			{
				y += h;
				height_ = y;
				y += rowSpacing_;
				x = 0, c = 0, h = 0;
			}

			if(*widget)
			{
				(*widget)->updateSize();
				vec2i size = (*widget)->getSize();
				h = max(h, size.y);
				if(cols[c].adjust)
				{
					cols[c].width = max(cols[c].width, (uint)size.x);
				}
			}

			x += cols[c].width;
			width_ = max(width_, x);
			x += rowSpacing_;
		}

		y += h;
		height_ = y;
		y += rowSpacing_;
	}
}

void RowLayout::onArrange(recti r)
{
	int y = r.y;

	int extraW = max(0, r.w - width_);

	for(auto& row : rowLayouts_)
	{
		bool expanded = false;

		GuiWidget** widget = row.widgets.begin();
		int numWidgets = row.widgets.size();
		int numCols = row.cols.size();
		Col* cols = row.cols.begin();

		int h = 0, x = r.x;
		for(int i = 0, c = 0; i < numWidgets; ++i, ++c, ++widget)
		{
			if(c == numCols)
			{
				y += h + rowSpacing_;
				x = r.x, c = 0, h = 0;
				expanded = false;
			}

			int colW = cols[c].width;
			if(cols[c].expand && !expanded)
			{
				colW += extraW;
				expanded = true;
			}

			if(*widget)
			{
				vec2i size = (*widget)->getSize();
				(*widget)->arrange({x, y, colW, size.y});
				h = max(h, size.y);
			}

			x += cols[c].width + rowSpacing_;
		}

		y += h + rowSpacing_;
	}
}

void RowLayout::onTick()
{
	FOR_VECTOR_REVERSE(widgetList_, i)
	{
		widgetList_[i]->tick();
	}
}

void RowLayout::onDraw()
{
	FOR_VECTOR_FORWARD(widgetList_, i)
	{
		widgetList_[i]->draw();
	}
}

void RowLayout::add(GuiWidget* widget)
{
	widgetList_.push_back(widget);
	rowLayouts_.back().widgets.push_back(widget);
}

void RowLayout::addBlank()
{
	rowLayouts_.back().widgets.push_back(nullptr);
}

RowLayout& RowLayout::row(bool expand)
{
	if(rowLayouts_.back().widgets.empty()) rowLayouts_.pop_back();

	Row& row = rowLayouts_.append();
	row.expand = (expand == true);

	return *this;
}

RowLayout& RowLayout::col(bool expand)
{
	return col(INT_MAX, expand);
}

RowLayout& RowLayout::col(int w, bool expand)
{
	Col& col = rowLayouts_.back().cols.append();
	col.width = (w == INT_MAX) ? 0 : w;
	col.adjust = (w == INT_MAX);
	col.expand = (expand == true);

	return *this;
}

GuiWidget** RowLayout::begin()
{
	return widgetList_.begin();
}

GuiWidget** RowLayout::end()
{
	return widgetList_.end();
}

}; // namespace Vortex
