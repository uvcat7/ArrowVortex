#pragma once

#include <Core/Graphics/Draw.h>

#include <Simfile/Tempo/SegmentSet.h>

#include <Vortex/Edit/Selection.h>

namespace AV {

struct SegmentBoxes
{
	static void drawBoxes(float leftSideX, float rightSideX, const Texture* boxImage);

	static SelectedSegments getSegmentsInRegion(float leftSideX, float rightSideX, int beginRow, Row endRow, int beginX, int endX);
};

} // namespace AV
