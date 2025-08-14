#pragma once

#include <Editor/Common.h>

namespace Vortex {

static const int sRowSnapTypes[NUM_SNAP_TYPES] =
{
	1, 48, 24, 16, 12, 8, 6, 4, 3, 2, 1, 0
};

// Represents a time stamp in time-based mode, and a row in row-based mode.
typedef double ChartOffset;

struct View
{
	enum SnapDir { SNAP_UP, SNAP_DOWN, SNAP_CLOSEST };

	struct Coords { int xl, xc, xr, y; };

	static void create(XmrNode& settings);
	static void destroy();

	virtual void saveSettings(XmrNode& settings) = 0;

	virtual void onChanges(int changes) = 0;

	virtual void tick() = 0;

	virtual void toggleReverseScroll() = 0;
	virtual void toggleChartPreview() = 0;

	virtual void setTimeBased(bool enabled) = 0;

	virtual void setZoomLevel(double level) = 0;
	virtual void setScaleLevel(double level) = 0;

	virtual void setCursorTime(double time) = 0;
	virtual void setCursorRow(int row) = 0;
	virtual void setCursorOffset(ChartOffset ofs) = 0;
	virtual void setCursorToNextInterval(int rows) = 0;
	virtual void setCursorToStream(bool top) = 0;
	virtual void setCursorToSelection(bool top) = 0;

	virtual void setSnapType(int type) = 0;
	virtual void setCustomSnap(int snap) = 0;
	virtual int  snapRow(int row, SnapDir direction) = 0;
	virtual bool isAlignedToSnap(int row) = 0;

	virtual void adjustForPreview(bool enabled) = 0;
	virtual int getPreviewOffset() const = 0;
	virtual double getZoomLevel() const = 0;
	virtual double getScaleLevel() const = 0;
	virtual int applyZoom(int v) const = 0;

	virtual Coords getReceptorCoords() const = 0;
	virtual Coords getNotefieldCoords() const = 0;

	virtual int columnToX(int column) const = 0;

	virtual int rowToY(int row) const = 0;
	virtual int timeToY(double time) const = 0;
	virtual int offsetToY(ChartOffset ofs) = 0;

	virtual ChartOffset yToOffset(int y) const = 0;
	virtual ChartOffset rowToOffset(int row) const = 0;
	virtual ChartOffset timeToOffset(double time) const = 0;

	virtual int offsetToRow(ChartOffset ofs) const = 0;
	virtual double offsetToTime(ChartOffset ofs) const = 0;

	virtual SnapType getSnapType() const = 0;
	virtual int getSnapQuant() = 0;
	virtual int getCustomSnap() const = 0;
	virtual bool isTimeBased() const = 0;
	virtual bool hasReverseScroll() const = 0;
	virtual bool hasChartPreview() const = 0;
	virtual int getNoteScale() const = 0;

	virtual double getPixPerSec() const = 0;
	virtual double getPixPerRow() const = 0;
	virtual double getPixPerOfs() const = 0;

	virtual int getCursorRow() const = 0;
	virtual double getCursorTime() const = 0;
	virtual double getCursorBeat() const = 0;
	virtual ChartOffset getCursorOffset() const = 0;

	virtual int getWidth() const = 0;
	virtual int getHeight() const = 0;
	virtual const recti& getRect() const = 0;
};

extern View* gView;

}; // namespace Vortex
