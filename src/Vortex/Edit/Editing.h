#pragma once

#include <Precomp.h>

namespace AV {
namespace Editing {

enum class Mirror
{
	Horizontal,
	Vertical,
	Both,
};

void initialize(const XmrDoc& settings);
void deinitialize();

void drawGhostNotes();

void deleteSelection();

void changeHoldsToSteps();
void changeNotesToMines();
void changeNotesToFakes();
void changeNotesToLifts();
void changeHoldsToRolls();
void changePlayerNumber();

void mirrorNotes(Mirror type);
void scaleNotes(int numerator, int denominator);

void insertRows(Row row, int numRows, bool curChartOnly);

} // namespace Editing
} // namespace AV
