#include <Precomp.h>

#include <Core/Utils/Util.h>
#include <Core/Utils/Vector.h>
#include <Core/Common/Serialize.h>

#include <Core/System/System.h>
#include <Core/System/Debug.h>

#include <Core/Graphics/Draw.h>
#include <Core/Graphics/Text.h>

#include <Simfile/Chart.h>
#include <Simfile/Simfile.h>
#include <Simfile/History.h>

namespace AV {

using namespace std;
using namespace Util;

// Indicates none of the history entries match the state of the last save.
static constexpr int NoSavedEntries = -1;

ChartHistory::~ChartHistory()
{
	clear();
}

ChartHistory::ChartHistory(Chart* chart)
	: myAppliedEntries(0)
	, mySavedEntries(0)
	, myChart(chart)
{
}

void ChartHistory::undo()
{
	if (myAppliedEntries > 0)
		myEntries[--myAppliedEntries]->undo(myChart);
}

void ChartHistory::redo()
{
	if (myAppliedEntries < (int)myEntries.size())
		myEntries[myAppliedEntries++]->apply(myChart);
}

void ChartHistory::clear()
{
	while (!myEntries.empty())
		myEntries.pop_back();

	mySavedEntries = NoSavedEntries;
}

void ChartHistory::add(unique_ptr<ChartHistoryEntry>&& entry)
{
	myEntries.push_back(move(entry));
	myEntries.back()->apply(myChart);
}

void ChartHistory::onFileSaved()
{
	mySavedEntries = myAppliedEntries;
}

bool ChartHistory::hasUnsavedChanges() const
{
	return mySavedEntries != myAppliedEntries;
}

void ChartHistory::myDiscardUnappliedEntries()
{
	while (int(myEntries.size()) > myAppliedEntries)
		myEntries.pop_back();

	if (mySavedEntries > myAppliedEntries)
		mySavedEntries = NoSavedEntries;
}

} // namespace AV
