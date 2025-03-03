#pragma once

#include <Precomp.h>

namespace AV {

class History;

class ChartHistoryEntry
{
public:
	virtual ~ChartHistoryEntry() {}
	virtual void apply(Chart* history) const = 0;
	virtual void undo(Chart* history) const = 0;
	virtual std::string description() const = 0;
};

class ChartHistory
{
public:
	~ChartHistory();
	ChartHistory(Chart* chart);

	// Undo the most recently applied edit.
	void undo();

	// Redo the most recent undone edit.
	void redo();

	// Clears the entire editing history.
	void clear();

	// Adds an edit to the history and applies it immediately using the ApplyEdit function that is
	// associated with the ID.
	void add(unique_ptr<ChartHistoryEntry>&& entry);

	// Tells the history that the current history state is now the saved state.
	void onFileSaved();

	// Returns true if the current history state is not the same as the most recent saved state.
	bool hasUnsavedChanges() const;

private:
	void myDiscardUnappliedEntries();

	vector<unique_ptr<ChartHistoryEntry>> myEntries;

	int myAppliedEntries;
	int mySavedEntries;

	Chart* myChart;
};

} // namespace AV
