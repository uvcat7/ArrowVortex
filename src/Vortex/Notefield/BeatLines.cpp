#include <Precomp.h>

#include <Core/Utils/Util.h>
#include <Core/Utils/String.h>

#include <Core/Graphics/Text.h>
#include <Core/Graphics/Draw.h>

#include <Core/Interface/UiStyle.h>

#include <Simfile/Simfile.h>
#include <Simfile/Tempo.h>

#include <Vortex/Editor.h>

#include <Vortex/View/View.h>
#include <Vortex/Notefield/BeatLines.h>
#include <Vortex/Notefield/NotefieldPosTracker.h>

namespace AV {

using namespace std;
using namespace Util;

struct MeasureLabel
{
	MeasureLabel(int measure, int y)
		: measure(measure), y(y) {}

	int measure;
	int y;
};

void BeatLines::drawBeats(float x, float w)
{
	auto sim = Editor::currentSimfile();
	auto tempo = Editor::currentTempo();
	if (!tempo) return;

	bool zoomedIn = View::isZoomedIn();

	// Keep track of the measure labels to render them afterwards.
	vector<MeasureLabel> labels;
	labels.reserve(8);

	// Determine the first and last startTime/row visible on screen.
	auto visibleRows = View::getVisibleRows(8, 8);
	int drawBeginRow = (int)lround(visibleRows.first);
	int drawEndRow = min((int)lround(visibleRows.second), View::getEndRow()) + 1;

	// Skip over startTime signatures that are not drawn.
	auto& signatures = tempo->timing.signatures();
	auto sig = signatures.begin();
	auto end = signatures.end();
	auto next = sig + 1;
	while (next < end && next->row <= drawBeginRow) sig = next, ++next;

	// Skip over measures at the start of the startTime signature that are not drawn.
	int measure = sig->measure, row = sig->row;
	if (drawBeginRow > row + sig->rowsPerMeasure)
	{
		int skip = (drawBeginRow - sig->row - sig->rowsPerMeasure) / sig->rowsPerMeasure;
		measure += skip;
		row += skip * sig->rowsPerMeasure;
	}

	// Start drawing measures and beat lines.
	Renderer::startBatch(Renderer::FillType::ColoredQuads, Renderer::VertexType::Float);
	Renderer::bindShader(Renderer::Shader::Color);
	Renderer::resetColor();

	Color constexpr halfColor = Color(255, 255, 255, 45);
	Color constexpr fullColor = Color(255, 255, 255, 130);

	NotefieldPosTracker drawPos;
	while (sig != end && row < drawEndRow)
	{
		Row endRow = drawEndRow;
		if (next != end) endRow = min(endRow, next->row);
		while (row < endRow)
		{
			// Measure line and measure label.
			float y = float(drawPos.advance(row));
			Renderer::pushQuad(x, y, x + w, y + 1, fullColor);
			labels.emplace_back(measure, (int)y);

			// Beat lines.
			if (zoomedIn)
			{
				int beatRow = row + int(RowsPerBeat);
				int measureEnd = min(row + sig->rowsPerMeasure, drawEndRow);
				while (beatRow < measureEnd)
				{
					float y = float(drawPos.advance(beatRow));
					Renderer::pushQuad(x, y, x + w, y + 1, halfColor);
					beatRow += int(RowsPerBeat);
				}
			}

			++measure;
			row += sig->rowsPerMeasure;
		}
		sig = next;
		if (next != end)
			++next;
	}
	Renderer::flushBatch();

	// Draw the measure labels.
	float dist = float(View::getZoomScale() * 10 + 2);
	Text::setStyle(UiText::regular);
	if (!zoomedIn) Text::setFontSize(9);
	for (const auto& label : labels)
	{
		string info = String::fromInt(label.measure);
		Text::format(TextAlign::MR, info.data());
		Text::draw(int(x - dist), label.y);
	}
}

} // namespace AV
