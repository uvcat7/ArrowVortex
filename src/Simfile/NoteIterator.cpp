#include <Precomp.h>

#include <limits.h>

#include <Simfile/Common.h>
#include <Simfile/NoteSet.h>
#include <Simfile/NoteIterator.h>

namespace AV {

/*

NoteIterator::~NoteIterator()
{
	delete[] myCur;
}

NoteIterator::NoteIterator(const NoteSet& target)
{
	myNumColumns = target.numColumns();
	myCur = new const Note*[myNumColumns*2];
	myEnd = myCur + myNumColumns;
	for (int i = 0; i < myNumColumns; ++i)
	{
		auto& column = target.column(i);
		myCur[i] = column.begin();
		myEnd[i] = column.end();
	}
}

const Note* NoteIterator::next(int& outColumn)
{
	int col = -1;
	Row row = INT_MAX;
	const Note* result = nullptr;
	for (int i = 0; i < myNumColumns; ++i)
	{
		auto cur = myCur[i];
		if (cur == myEnd[i]) continue;
		if (cur->row >= row) continue;
		row = cur->row, col = i;
		result = cur;
	}
	if (col >= 0) ++myCur[col];
	outColumn = col;
	return result;
}

*/

} // namespace AV