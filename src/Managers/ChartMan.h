#pragma once

#include <Simfile/Chart.h>

#include <vector>

namespace Vortex {

/// Manages the active chart.
struct ChartMan
{
	static void create();
	static void destroy();

	/// Called when the active chart changes.
	virtual void update(Chart* chart) = 0;

	/// Returns true if a chart is open for editing, false otherwise.
	virtual bool isOpen() const = 0;

	/// Returns false if a chart is open for editing, true otherwise.
	virtual bool isClosed() const = 0;

	/// Sets the step artist of the current chart.
	virtual void setStepArtist(String stepArtist) = 0;

	/// Sets the difficulty of the current chart.
	virtual void setDifficulty(Difficulty dt) = 0;

	/// Sets the block rating of the current chart.
	virtual void setMeter(int meter) = 0;

	/// Returns the step artist of the current chart.
	virtual String getStepArtist() const = 0;

	/// Returns the difficulty type of the current chart (e.g. Challenge).
	virtual Difficulty getDifficulty() const = 0;

	/// Returns the block rating of the current chart.
	virtual int getMeter() const = 0;

	/// Returns a description of the difficulty and meter (e.g. "Challenge 12").
	virtual String getDescription() const = 0;

	/// Represents a single item in a stream breakdown.
	struct BreakdownItem { int row, endrow; String text; };

	/// Returns a text representation of the 16th stream breakdown of the current chart.
	virtual std::vector<BreakdownItem> getStreamBreakdown(int* totalMeasures = nullptr) const = 0;

	/// Returns an estimate block rating based on analyzing the current chart.
	virtual double getEstimatedMeter() const = 0;

	/// Returns the chart data of the active chart.
	virtual const Chart* get() const = 0;
};

extern ChartMan* gChart;

}; // namespace Vortex
