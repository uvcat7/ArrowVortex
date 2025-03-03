#pragma once

#include <Core/Types/Vec2.h>
#include <Vortex/Common.h>

namespace AV {

struct StreamGenerator
{
	StreamGenerator();

	int maxColRep;
	int maxBoxRep;
	bool allowBoxes;
	bool startWithRight;
	float patternDifficulty;

	void generate(int startRow, Row endRow, Vec2 feetCols, SnapType spacing);
};

} // namespace AV
