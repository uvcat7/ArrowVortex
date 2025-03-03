#include <Precomp.h>

#include <Vortex/View/Widgets/BreakdownList.h>

#include <Core/Utils/Util.h>
#include <Core/Utils/String.h>
#include <Core/Utils/vector.h>

#include <Core/System/System.h>
#include <Core/System/Window.h>

#include <Core/Graphics/Text.h>

#include <Core/Interface/UiStyle.h>

#include <Simfile/Chart.h>
#include <Simfile/NoteSet.h>
#include <Simfile/Tempo.h>

#include <Vortex/View/View.h>

#include <Vortex/Edit/Selection.h>

namespace AV {

using namespace std;
using namespace Util;

// =====================================================================================================================
// Helper functions.

struct StreamItem
{
	StreamItem()
		: row(0)
		, endRow(0)
		, isBreak(false)
	{
	}

	StreamItem(Row row, Row endRow, bool isBreak)
		: row(row)
		, endRow(endRow)
		, isBreak(isBreak)
	{
	}

	Row row;
	Row endRow;
	bool isBreak;
};

static void MergeItems(vector<StreamItem>& items)
{
	// Merge successive streams and breaks into a single item. 
	for (int i = (int)items.size() - 1; i > 0; --i)
	{
		if (items[i].isBreak == items[i - 1].isBreak)
		{
			items[i - 1].endRow = items[i].endRow;
			items.erase(items.begin() + i);
		}
	}

	// Remove breaks at the front and back of the list.
	if (items.size() && items.back().isBreak)
		items.pop_back();

	if (items.size() && items[0].isBreak)
		items.erase(items.begin());
}

static void RemoveItems(vector<StreamItem>& items, int minRows, bool breaks)
{
	for (int i = (int)items.size() - 1; i >= 0; --i)
	{
		if ((items[i].endRow - items[i].row) < minRows && items[i].isBreak == breaks)
		{
			items.erase(items.begin() + i);
		}
	}
	MergeItems(items);
}

static vector<StreamItem> ToStreams(const Chart* chart, int& totalMeasures, bool simplify)
{
	totalMeasures = 0;
	vector<StreamItem> items;

	// Reduce all notes to a set of rows.
	set<int> noteRows;
	for (auto& col : chart->notes)
	{
		for (auto& note : col)
		{
			if (note.type != (uint)NoteType::Mine)
			{
				noteRows.insert(note.row);
			}
		}
	}

	// If the chart is empty, quit early.
	if (!chart) return items;
	if (noteRows.begin() == noteRows.end()) return items;

	// Build a list of streams and breaks.
	auto first = noteRows.begin();
	auto last = noteRows.end(); --last;
	int prevStreamEnd = -1, streamBegin = -1;
	for (auto curRow = first; curRow != last; ++curRow)
	{
		auto nextRow = curRow; ++nextRow;
		bool isStreamNote = (*nextRow - *curRow <= 12);

		if (isStreamNote)
		{
			if (streamBegin < 0)
			{
				streamBegin = *curRow;
			}
		}

		if (nextRow == last || !isStreamNote)
		{
			auto streamEnd = *curRow;
			if (streamBegin && streamEnd >= streamBegin + (RowsPerBeat * 4))
			{
				if (prevStreamEnd >= 0 && streamBegin > prevStreamEnd)
				{
					items.emplace_back(prevStreamEnd, streamBegin, true);
				}
				items.emplace_back(streamBegin, streamEnd, false);
				prevStreamEnd = streamEnd;
			}
			streamBegin = -1;
		}
	}

	for (auto& i : items)
	{
		if (!i.isBreak)
		{
			totalMeasures += (i.endRow - i.row + 48) / int(RowsPerBeat * 4);
		}
	}

	// Merge streams with breaks shorter than half a beat.
	constexpr int rpb = int(RowsPerBeat);
	RemoveItems(items, rpb / 2, true);

	// Merge/remove more streams if the breakdown is too long.
	if (simplify)
	{
		if (items.size() > 32) RemoveItems(items, rpb * 8, false);
		if (items.size() > 32) RemoveItems(items, rpb * 2, true);
		if (items.size() > 32) RemoveItems(items, rpb * 16, false);
		if (items.size() > 32) RemoveItems(items, rpb * 4, true);
		if (items.size() > 32) RemoveItems(items, rpb * 24, false);
		if (items.size() > 32) RemoveItems(items, rpb * 6, true);
		if (items.size() > 32) RemoveItems(items, rpb * 32, false);
	}

	return items;
}

struct BreakdownItem { Row row, endRow; string text; };

static vector<BreakdownItem> ToBreakdown(const vector<StreamItem>& items)
{
	vector<BreakdownItem> out;
	BreakdownItem item = {-1, -1};
	for (auto& it : items)
	{
		if (it.isBreak)
		{
			if (it.endRow - it.row > int(RowsPerBeat * 4))
			{
				out.emplace_back(item);
				item.row = item.endRow = -1;
				item.text.clear();
			}
			else
			{
				item.text += '-';
			}
		}
		else
		{
			String::appendDouble(item.text, (it.endRow - it.row + 48) / (RowsPerBeat * 4));
			if (item.row == -1) item.row = it.row;
			item.endRow = it.endRow;
		}
	}
	if (item.row != -1)
		out.emplace_back(item);
	return out;
}

// =====================================================================================================================
// BreakdownButtons.

WBreakdownList::~WBreakdownList()
{
}

WBreakdownList::WBreakdownList()
	: myTotalMeasures(0)
	, myPressedButton(-1)
{
	myWidth = 128;
}

void WBreakdownList::update(const Chart* chart, bool simplify)
{
	myItems.clear();

	if (chart == nullptr) return;

	auto items = ToStreams(chart, myTotalMeasures, simplify);
	auto breakdown = ToBreakdown(items);

	Text::setStyle(UiText::regular);
	for (auto& it : breakdown)
	{
		Item item;

		item.text = it.text;
		Text::format(TextAlign::TL, item.text.data());
		item.w = max(16, Text::getWidth() + 8);
		item.startRow = it.row;
		item.endRow = it.endRow;

		myItems.emplace_back(item);
	}
}

string WBreakdownList::convertToText()
{
	string out;
	for (auto& item : myItems)
	{
		out += item.text;
		out += '/';
	}
	if (out.length())
	{
		out.pop_back();
	}
	return out;
}

int WBreakdownList::getItemAtPos(Vec2 pos)
{
	int index = 0;
	for (auto& item : myItems)
	{
		Rect r = {item.x, item.y, item.w, 20};
		if (r.contains(pos))
		{
			return index;
		}
		++index;
	}
	return -1;
}

void WBreakdownList::onMousePress(MousePress& input)
{
	if (isMouseOver())
	{
		if (isEnabled() && input.button == MouseButton::LMB && input.unhandled())
		{
			startCapturingMouse();
			myPressedButton = getItemAtPos(input.pos);
			if (myPressedButton >= 0)
			{
				auto& item = myItems[myPressedButton];
				Selection::selectRegion(item.startRow, item.endRow);
				View::setCursorRow(item.startRow);
			}
		}
		input.setHandled();
	}
}

void WBreakdownList::onMouseRelease(MouseRelease& input)
{
	stopCapturingMouse();
	myPressedButton = -1;
}

int WBreakdownList::getTotalMeasureCount() const
{
	return myTotalMeasures;
}

void WBreakdownList::onMeasure()
{
	if (myItems.size())
	{
		int x = 0, h = 20;
		int maxW = max(myRect.w(), 128);
		for (auto& item : myItems)
		{
			if (x + item.w > maxW)
			{
				x = 0, h += 22;
			}
			x += item.w + 2;
		}
		myHeight = h;
	}
	else
	{
		myHeight = 128;
	}
}

void WBreakdownList::onArrange(Rect r)
{
	myRect = r;
	int x = 0, y = 0;
	for (auto& item : myItems)
	{
		if (x + item.w > r.w())
		{
			x = 0;
			y += 22;
		}
		item.x = r.l + x;
		item.y = r.t + y;
		x += item.w + 2;
	}
}

void WBreakdownList::draw(bool enabled)
{
	int selectedItem = myPressedButton;

	if (selectedItem == -1)
		selectedItem = getItemAtPos(Window::getMousePos());

	int index = 0;
	for (auto& item : myItems)
	{
		Rect r = {item.x, item.y, item.w, 20};
		auto& button = UiStyle::getButton();

		// Draw the button graphic.
		if (isCapturingMouse() && index == selectedItem)
		{
			button.press.draw(r);
		}
		else if (isMouseOver() && index == selectedItem)
		{
			button.hover.draw(r);
		}
		else
		{
			button.base.draw(r);
		}

		// Draw the button text.
		TextStyle textStyle = UiText::regular;
		if (!isEnabled())
			textStyle.textColor = UiStyle::getMisc().colDisabled;
		
		Renderer::pushScissorRect(r.shrink(2));
		Text::setStyle(UiText::regular);
		Text::format(TextAlign::MC, item.text.data());
		Text::draw(r.shrinkX(2));
		Renderer::popScissorRect();

		++index;
	}
}

} // namespace AV
