#include <Precomp.h>

#include <Core/Utils/Util.h>
#include <Core/Utils/String.h>

#include <Core/System/System.h>
#include <Core/System/Window.h>

#include <Core/Graphics/Draw.h>
#include <Core/Graphics/Text.h>

#include <Core/Interface/UiStyle.h>

#include <Simfile/Simfile.h>
#include <Simfile/Tempo.h>

#include <Vortex/Editor.h>

#include <Vortex/View/View.h>
#include <Vortex/Notefield/SegmentBoxes.h>
#include <Vortex/Notefield/NotefieldPosTracker.h>
#include <Vortex/Notefield/Notefield.h>

#include <Vortex/Edit/Selection.h>

namespace AV {

using namespace std;
using namespace Util;

/*
struct BoxData
{
	Segment* segment;
	SegmentType type;
	string str;
	int x, y;
	int width;
};

static void GetVisibleBoxes(vector<BoxData>& out, int leftSideX, int rightSideX, int beginRow, Row endRow)
{
	auto tempo = Editor::currentTempo();
	if (!tempo) return;

	// Get the range of visible rows.
	auto visibleRows = View::getVisibleRows(16, 16);
	int drawBeginRow = visibleRows.x;
	int drawEndRow = visibleRows.y;

	// Make a temporary list of boxes for every visible segment.
	for (auto& segments : tempo->segments)
	{
		auto type = segments.type();
		auto meta = Segment::getTypeData(type);
		for (auto& seg : segments)
		{
			Row row = seg.row;
			if (row < beginRow) continue;
			if (row >= endRow) break;
			string str = meta->getDescription(&seg);
			out.emplace_back(BoxData{&seg, type, str, 0, 0, 0});
		}
	}

	// Sort by row, so boxes on the same row can be stacked.
	stable_sort(out.begin(), out.end(),
	[](const BoxData& a, const BoxData& b)
	{
		return (a.segment->row < b.segment->row);
	});

	// Calculate the x, y and width based on row.
	NotefieldPosTracker<float> tracker;
	TextStyle textStyle = UiText::regular;
	int previousRow = -1;
	int leftX = leftSideX;
	int rightX = rightSideX;
	for (auto& box : out)
	{
		Text::setStyle(textStyle);
		Text::format(TextAlign::MC, box.str.data());
		box.width = max(32, Text::getWidth() + 24);

		if (Segment::getTypeData(box.type)->side == Segment::TypeData::LEFT)
		{
			if (box.segment->row != previousRow) 
			{
				leftX = leftSideX;
			}
			else
			{
				previousRow = box.segment->row;
			}
			leftX -= box.width;
			box.x = leftX;
		}
		else
		{
			if (box.segment->row != previousRow)
			{
				rightX = rightSideX;
			}
			else
			{
				previousRow = box.segment->row;
			}
			box.x = rightX;
			rightX += box.width;
		}

		box.y = (int)tracker.advance(box.segment->row);
	}
}

static void DrawHelpIndicator(const BoxData& box)
{
	auto meta = Segment::getTypeData(box.type);

	int x = box.x + box.width / 2;
	int y = box.y + 24;
	int width = 0;
	int height = 0;

	Text::setStyle(UiText::regular);

	Text::setFontSize(12);
	Text::format(TextAlign::TC, meta->singular);
	width = max(width, Text::getWidth());
	int nameHeight = Text::getHeight();
	height += nameHeight;

	Text::setFontSize(10);
	Text::setTextColor(Color(192, 192, 192, 255));
	Text::format(TextAlign::TC, meta->help);
	width = max(width, Text::getWidth());
	height += Text::getHeight();

	width += 12;
	height += 8;
	Rect r(x - width / 2, y, x + width / 2, y + height);

	Draw::roundedBox(r, Color(128, 128, 128, 255));
	r = r.shrink(1);
	Draw::roundedBox(r, Color(26, 26, 26, 255));

	Text::draw(x, y + nameHeight + 4);

	Text::setFontSize(12);
	Text::setTextColor(Color::White);
	Text::format(TextAlign::MC, meta->singular);
	Text::draw(x, y + 10);
}
*/

void SegmentBoxes::drawBoxes(float leftSideX, float rightSideX, const Texture* boxImage)
{
	// TODO: fix
	/*
	if (!View::isZoomedIn())
		return;

	// Get the range of visible rows.
	auto visibleRows = View::getVisibleRows(16, 16);
	int drawBeginRow = visibleRows.x;
	int drawEndRow = visibleRows.y;

	// Get all visible boxes.
	vector<BoxData> boxes;
	boxes.reserve(16);
	GetVisibleBoxes(boxes, (int)leftSideX, (int)rightSideX, drawBeginRow, drawEndRow);

	// Create a tilebar from the box image.
	TileBar boxBar[2];
	boxBar[0].texture = boxBar[1].texture = boxImage;
	boxBar[0].border = boxBar[1].border = 8;
	boxBar[0].uvs = Rectf(0, 0, 1, 0.5f);
	boxBar[1].uvs = Rectf(0, 0.5f, 1, 1);

	// Draw the box sprites.
	Renderer::resetColor();
	Renderer::bindTextureAndShader(boxImage);
	Renderer::startBatch(Renderer::FillType::COLORED_TEXTURED_QUADS);

	for (auto& box : boxes)
	{
		int side = Segment::getTypeData(box.type)->side;
		int flags = side * TileBar::Flag::FLIP_H;
		Rect r(box.x, box.y - 16, box.x + box.width, box.y + 16);
		Color color = Segment::getTypeData(box.type)->color;
		boxBar[0].drawBatched(r, color, flags);
	}

	// Draw the selection highlights.
	auto tempo = Editor::currentTempo();
	if (Selection::hasSegmentSelection(tempo))
	{
		for (auto& box : boxes)
		{
			if (Selection::hasSelectedSegment(box.type, box.segment))
			{
				int x = (int)box.x, y = (int)box.y;
				int side = Segment::getTypeData(box.type)->side;
				int flags = side * TileBar::Flag::FLIP_H;
				Rect r(x, y - 16, x + box.width, y + 16);
				boxBar[1].drawBatched(r, Color::White, flags);
			}
		}
	}

	Renderer::flushBatch();

	// Draw the text labels on top of the box sprites.
	TextStyle textStyle = UiText::regular;
	for (auto& box : boxes)
	{
		int side = Segment::getTypeData(box.type)->side;
		int x = box.x + side * 4 - 2;

		Text::setStyle(textStyle);
		Text::format(TextAlign::MC, box.str.data());
		Text::draw(Rect(x, box.y - 17, x + box.width, box.y + 15));
	}

	// Draw a help indicator if the user is hovering over a box.
	Vec2 mousePos = Window::getMousePos();
	if (Notefield::hasShowTempoHelp() && !Window::isMouseDown(MouseButton::Lmb))
	{
		for (auto& box : boxes)
		{
			int side = Segment::getTypeData(box.type)->side;
			if (Rect(box.x, box.y - 16, box.x + box.width, box.y + 16).contains(mousePos))
			{
				DrawHelpIndicator(box);
				break;
			}
		}
	}
	*/
}

SelectedSegments SegmentBoxes::getSegmentsInRegion(float leftSideX, float rightSideX, int beginRow, Row endRow, int beginX, int endX)
{
	SelectedSegments outList;
	// TODO: fix
	/*
	auto sim = Editor::currentSimfile();
	auto tempo = Editor::currentTempo();
	if (!tempo) return SegmentSet();

	// Add some leeway to the x-range so the boxes do not have to be entirely inside.
	if (beginX >= endX) return SegmentSet();
	beginX -= 4;
	endX += 4;

	// Get all boxes inside the range of rows.
	vector<BoxData> boxes;
	boxes.reserve(16);
	GetVisibleBoxes(boxes, (int)leftSideX, (int)rightSideX, beginRow, endRow);

	// Check which of the boxes are also within the x-range.
	
	for (auto& box : boxes)
	{
		if (box.x >= beginX && box.x + box.width < endX)
		{
			outList.append(box.type, box.segment);
		}
	}
	*/
	return outList;
}

} // namespace AV
