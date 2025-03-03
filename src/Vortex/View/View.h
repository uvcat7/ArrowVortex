#pragma once

#include <Core/Common/Event.h>
#include <Core/Common/Setting.h>

#include <Core/Types/Rect.h>

#include <Vortex/Common.h>
#include <Vortex/Commands.h>
#include <Vortex/View/Minimap.h>

namespace AV {

// Represents a startTime in startTime-based mode, and a row in row-based mode.
typedef double ViewOffset;

enum class SnapDir
{
	Up,
	Down,
	Closest,
};

namespace View{

struct Coords { float xl, xc, xr, y; };

void initialize(const XmrDoc& settings);
void deinitialize();

void startZooming(bool zoomIn);
void finishZooming();
void setZoomLevel(double level);

void startAdjustingNoteSpacing(bool increaseSpacing);
void finishAdjustingNoteSpacing();
void setNoteSpacing(double level);

void setCursorTime(double time);
void setCursorRow(double row);
void setSnapType(SnapType type);

void setCursorOffset(ViewOffset ofs);

void jumpToCursorRow(Row row);

Row snapRow(Row row, SnapDir direction);
Row snapToNextMeasure(bool isUp);
Row snapToStream(bool top);
Row snapToSelection(bool top);

bool isAlignedToSnap(Row row);

double getZoomLevel();
double getNoteSpacing();

Coords getReceptorCoords();
Coords getNotefieldCoords();

float getZoomScale();
bool isZoomedIn();
float columnToX(int column);
float rowToY(Row row);
float timeToY(double time);
float offsetToY(ViewOffset ofs);

ViewOffset yToOffset(int y);
ViewOffset rowToOffset(Row row);
ViewOffset timeToOffset(double time);

double offsetToRow(ViewOffset ofs);
double offsetToTime(ViewOffset ofs);

std::pair<double, double> getVisibleRows(int pixelMarginTop, int pixelMarginBottom);

SnapType getSnapType();

double getPixPerSec();
double getPixPerRow();
double getPixPerOfs();

Row getCursorRowI();
double getCursorRow();
double getCursorTime();
ViewOffset getCursorOffset();

int getWidth();
int getHeight();
const Rect& rect();

int getEndRow();
bool updateEndRow();

struct EndRowChanged : Event {};

} // namespace View
} // namespace AV
