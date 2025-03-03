#include <Precomp.h>

#include <cmath>

#include <Core/Utils/Util.h>
#include <Core/Utils/vector.h>

#include <Core/System/Debug.h>

#include <Core/Graphics/Draw.h>

#include <Core/Interface/Widgets/Grid.h>

namespace AV {

using namespace std;
using namespace Util;

struct Span
{
	int offset;
	int stretch;
	int margin;
	int size;
	int minSize;
	int actualSize;
	shared_ptr<Widget> autosizeWidget;
};

struct Cell
{
	Row row, col;
	Row rowSpan;
	int colSpan;
	shared_ptr<Widget> widget;
};

struct WGrid::GridData
{
	vector<Span> rows;
	vector<Span> cols;
	vector<Cell> cells;
};

WGrid::WGrid()
{
	myGrid = new GridData;
}

WGrid::~WGrid()
{
	delete myGrid;
}

void WGrid::addRow(int height, int bottomMargin, int stretch)
{
	myGrid->rows.emplace_back();
	auto& row = myGrid->rows.back();
	row.size = row.minSize = height;
	row.autosizeWidget = nullptr;
	row.margin = bottomMargin;
	row.stretch = stretch;
}

void WGrid::addRow(shared_ptr<Widget> autosize, int bottomMargin, int stretch)
{
	myGrid->rows.emplace_back();
	auto& row = myGrid->rows.back();
	row.size = row.minSize = 0;
	row.autosizeWidget = autosize;
	row.margin = bottomMargin;
	row.stretch = stretch;
}

void WGrid::addColumn(int width, int rightMargin, int stretch)
{
	myGrid->cols.emplace_back();
	auto& col = myGrid->cols.back();
	col.size = col.minSize = width;
	col.autosizeWidget = nullptr;
	col.margin = rightMargin;
	col.stretch = stretch;
}

void WGrid::addColumn(shared_ptr<Widget> autosize, int rightMargin, int stretch)
{
	myGrid->cols.emplace_back();
	auto& col = myGrid->cols.back();
	col.size = col.minSize = 0;
	col.autosizeWidget = autosize;
	col.margin = rightMargin;
	col.stretch = stretch;
}

void WGrid::addWidget(shared_ptr<Widget> widget, Row row, int column, Row rowSpan, int columnSpan)
{
	myGrid->cells.emplace_back();
	auto& cell = myGrid->cells.back();
	cell.col = column;
	cell.row = row;
	cell.colSpan = columnSpan;
	cell.rowSpan = rowSpan;
	cell.widget = widget;
}

void WGrid::setRowMinSize(Row row, int minSize)
{
	VortexAssert(row >= 0 && row < (int)myGrid->rows.size());
	myGrid->rows[row].minSize = minSize;
}

void WGrid::setColMinSize(int col, int minSize)
{
	VortexAssert(col >= 0 && col < (int)myGrid->cols.size());
	myGrid->cols[col].minSize = minSize;
}

static void ArrangeSpan(vector<Span>& spans, int offset, int availableSize, bool horizontal)
{
	// Calculate combined Size, minimum Size and stretch.
	int combinedSize = 0;
	int combinedMinSize = 0;
	int combinedStretch = 0;
	for (auto& span : spans)
	{
		combinedSize += span.size + span.margin;
		combinedMinSize += span.minSize + span.margin;
		combinedStretch += span.stretch;
	}

	// Check if the combined Size requirement can be met.
	if (availableSize >= combinedSize)
	{
		double scalar = (double)(availableSize - combinedSize) / (double)max(1, combinedStretch);
		int offset = 0;
		for (auto& span : spans)
		{
			span.offset = offset;
			span.actualSize = span.size + int(span.stretch * scalar);
			offset += span.actualSize + span.margin;
		}
	}
	// If not, check if the available Size is between Size and minimum Size.
	else if (availableSize >= combinedMinSize)
	{
		int sizeDiff = max(1, combinedSize - combinedMinSize);
		double t = (double)(availableSize - combinedMinSize) / (double)sizeDiff;
		int offset = 0;
		for (auto& span : spans)
		{
			span.offset = offset;
			span.actualSize = (int)lerp<double>(span.minSize, span.size, t);
			offset += span.actualSize + span.margin;
		}
	}
	else // If not, assign the minimum required space.
	{
		int offset = 0;
		for (auto& span : spans)
		{
			span.offset = offset;
			span.actualSize = span.minSize;
			offset += span.actualSize + span.margin;
		}
	}
}

void WGrid::onMeasure()
{
	for (auto& cell : myGrid->cells)
	{
		if (cell.widget) cell.widget->measure();
	}

	myWidth = 0;
	myMinWidth = 0;
	for (auto& col : myGrid->cols)
	{
		if (col.autosizeWidget)
		{
			col.minSize = col.autosizeWidget->getMinWidth();
			col.size = col.autosizeWidget->getWidth();
		}
		myWidth += col.size + col.margin;
		myMinWidth += col.minSize + col.margin;
	}

	myHeight = 0;
	myMinHeight = 0;
	for (auto& row : myGrid->rows)
	{
		if (row.autosizeWidget)
		{
			row.minSize = row.autosizeWidget->getMinWidth();
			row.size = row.autosizeWidget->getWidth();
		}
		myHeight += row.size + row.margin;
		myMinHeight += row.minSize + row.margin;
	}
}

void WGrid::onArrange(Rect r)
{
	myRect = r;

	ArrangeSpan(myGrid->cols, r.l, r.w(), true);
	ArrangeSpan(myGrid->rows, r.t, r.h(), false);

	for (auto& cell : myGrid->cells)
	{
		if (cell.widget)
		{
			auto& startCol = myGrid->cols[cell.col];
			auto& startRow = myGrid->rows[cell.row];
			auto& endCol = myGrid->cols[cell.col + cell.colSpan - 1];
			auto& endRow = myGrid->rows[cell.row + cell.rowSpan - 1];

			cell.widget->arrange(Rect(
				r.l + startCol.offset,
				r.t + startRow.offset,
				r.l + endCol.offset + endCol.actualSize,
				r.t + endRow.offset + endRow.actualSize));
		}
	}
}

UiElement* WGrid::findMouseOver(int x, int y)
{
	for (auto& cell : myGrid->cells)
	{
		if (cell.widget)
		{
			if (auto mouseOver = cell.widget->findMouseOver(x, y))
				return mouseOver;
		}
	}
	return Widget::findMouseOver(x, y);
}

void WGrid::handleEvent(UiEvent& uiEvent)
{
	for (auto& cell : myGrid->cells)
	{
		if (cell.widget)
			cell.widget->handleEvent(uiEvent);
	}
	Widget::handleEvent(uiEvent);
}

void WGrid::tick(bool enabled)
{
	for (auto& cell : myGrid->cells)
	{
		if (cell.widget)
			cell.widget->tick(enabled);
	}
}

void WGrid::draw(bool enabled)
{
	for (auto& cell : myGrid->cells)
	{
		if (cell.widget)
		{
			cell.widget->draw(enabled);
		}
		else
		{
			Rect r;

			auto& startCol = myGrid->cols[cell.col];
			auto& startRow = myGrid->rows[cell.row];
			auto& endCol = myGrid->cols[cell.col + cell.colSpan - 1];
			auto& endRow = myGrid->rows[cell.row + cell.rowSpan - 1];

			r.l = startCol.offset;
			r.t = startRow.offset;
			r.r = endCol.offset + endCol.size;
			r.b = endRow.offset + endRow.size;

			Draw::fill(r, Color(255, 64));
		}
	}
}

} // namespace AV
