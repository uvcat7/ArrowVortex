#include <Precomp.h>

#include <Core/Common/Serialize.h>

#include <Core/Utils/Util.h>
#include <Core/Utils/String.h>

#include <Core/System/System.h>

#include <Simfile/NoteSet.h>
#include <Simfile/Tempo/TimingData.h>
#include <Simfile/NoteSet.h>
#include <Simfile/Chart.h>
#include <Simfile/GameMode.h>

#include <Vortex/Editor.h>
#include <Vortex/Settings.h>

#include <Vortex/Edit/TempoTweaker.h>

#include <Vortex/View/View.h>
#include <Vortex/View/Hud.h>

#include <Vortex/Notefield/SegmentBoxes.h>

#include <Vortex/Edit/Selection.h>
#include <Vortex/Edit/Editing.h>
#include <Vortex/Edit/Clipboard.h>

namespace AV {
	
using namespace std;
using namespace Util;

static const char* ArrowVortexTag = "AV:";
static const char* NotesTag = "NOTES:";
static const char* TempoTag = "TEMPO:";

// =====================================================================================================================
// Clipboard :: utility functions.

static const int A85Shift[4] = {24, 16, 8, 0};
static const uint A85Div[5] = {85 * 85 * 85 * 85, 85 * 85 * 85, 85 * 85, 85, 1};

static void Ascii85enc(string& out, const uchar* in, size_t size)
{
	// Convert to 32-bit integers (big endian) and pad with zeroes.
	vector<uint> blocks((size + 3) / 4, 0U);
	for (size_t i = 0; i < size; ++i)
	{
		blocks[i / 4] += in[i] << A85Shift[i & 3];
	}

	// Encode 32-bit integers to base 85.
	for (size_t i = 0; i != blocks.size(); ++i)
	{
		if (blocks[i] == 0 && i + 1 != blocks.size())
		{
			out.push_back('z');
		}
		else for (int j = 0; j != 5; ++j)
		{
			out.push_back(33 + (blocks[i] / A85Div[j]) % 85);
		}
	}

	// Remove padded bytes from output.
	auto padding = blocks.size() * 4 - size;
	String::truncate(out, out.length() - padding);
}

static void Ascii85dec(vector<uchar>& out, const char* in)
{
	// Convert from base 85 to 32-bit integers.
	int padding = 0;
	while (*in)
	{
		if (*in == 'z')
		{
			out.resize(out.size() + 4);
			++in;
		}
		else
		{
			int v = 0, i = 0;
			for (; i != 5 && *in; ++i)
			{
				v = v * 85 + (*in++) - 33;
			}
			for (; i != 5; ++i, ++padding)
			{
				v = v * 85 + 'u' - 33;
			}
			for (i = 0; i != 4; ++i)
			{
				out.push_back((v >> A85Shift[i]) & 0xFF);
			}
		}
	}

	// Remove padded bytes from output.
	out.resize(out.size() - padding);
}

void SetClipboardData(const char* tag, const uchar* data, size_t size)
{
	string out(ArrowVortexTag);
	out.append(tag);
	Ascii85enc(out, data, size);
	System::setClipboardText(out);
}

// =====================================================================================================================
// Clipboard :: notes.

static void CopyNotes(bool cut)
{
	auto chart = Editor::currentChart();
	if (!chart) return;

	NoteSet notes(chart->gameMode->numCols);
	Selection::getSelectedNotes(notes);

	// Copy all the selected notes.
	if (notes.numNotes())
	{
		Serializer stream;
		bool timeBased = Settings::editor().timeBasedCopy;
		stream.write<uchar>(timeBased);
		if (timeBased)
		{
			auto& timing = Editor::currentTempo()->timing;
			notes.encodeTo(stream, timing, true);
		}
		else
		{
			notes.encodeTo(stream, true);
		}
		SetClipboardData(NotesTag, stream.data(), stream.size());
		Hud::note("Copied %i notes", notes.numNotes());
	}

	// If cutting, remove the selected notes.
	if (cut)
	{
		ChartModification mod(chart->gameMode->numCols);
		mod.notesToRemove.swap(notes);
		chart->modify(mod);
	}
}

static void PasteNotes(uchar* data, size_t size)
{
	auto chart = Editor::currentChart();
	if (!chart) return;

	ChartModification mod(chart->gameMode->numCols);

	// Decode the clipboard data.
	Deserializer stream(data, size);
	bool timeBased = (stream.read<uchar>() != 0);
	if (timeBased)
	{
		int offsetRow = View::getCursorRowI();
		auto& timing = Editor::currentTempo()->timing;
		mod.notesToAdd.decodeFrom(stream, timing, offsetRow);
	}
	else
	{
		int offsetRows = View::getCursorRowI();
		mod.notesToAdd.decodeFrom(stream, offsetRows);
	}

	if (stream.failed() || stream.bytesLeft() > 0)
	{
		Hud::error("Clipboard contains invalid note data");
		return;
	}

	// Insert the pasted notes.
	chart->modify(mod);
}

// =====================================================================================================================
// Clipboard :: tempo.

static void CopyTempo(bool cut)
{
	// TODO: fix
	/*
	SegmentSet segmentCollection;
	Selection::getSelectedSegments(segmentCollection);

	// Find out what the first row is.
	Row row = INT_MAX;
	for (auto& segments : segmentCollection)
	{
		if (segments.size())
		{
			row = min(row, segments.begin()->row);
		}
	}

	// Offset all segments to row zero.
	for (auto& segments : segmentCollection)
	{
		for (auto& seg : segments)
		{
			seg.row -= row;
		}
	}

	// Encode the segment data.
	if (segmentCollection.numSegments() > 0)
	{
		Serializer stream;
		segmentCollection.encodeTo(stream);
		SetClipboardData(TempoTag, stream.data(), stream.size());
		HudNote("Copied %s", segmentCollection.description().data());
	}

	// If cutting, remove the selected segments.
	if (cut)
	{
		TempoModification mod;
		mod.segmentsToRemove.swap(segmentCollection);
		Editor::currentTempo()->modify(mod);
	}
	*/
}

static void PasteTempo(uchar* data, size_t size)
{
	// TODO: fix
	/*
	TempoModification mod;

	// Decode the clipboard data.
	Deserializer stream(data, size);
	mod.segmentsToAdd.decodeFrom(stream);
	if (stream.failed() || stream.bytesLeft() > 0)
	{
		HudError("Clipboard contains invalid tempo data");
		return;
	}

	// Offset all segments to the cursor row.
	Row row = View::getCursorRowI();
	for (auto& segments : mod.segmentsToAdd)
	{
		for (auto& seg : segments)
		{
			seg.row += row;
		}
	}

	// Add the pasted segments to the current tempo.
	Editor::currentTempo()->modify(mod);
	*/
}

// =====================================================================================================================
// Clipboard :: API functions.

static void CopySelection(bool cut)
{
	if (Selection::hasSegmentSelection(Editor::currentTempo()))
	{
		CopyTempo(cut);
	}
	else
	{
		CopyNotes(cut);
	}
}

static void PasteClipboard()
{
	vector<uchar> buffer;
	string text = System::getClipboardText();
	if (String::startsWith(text, ArrowVortexTag))
	{
		const char* subTag = text.data() + strlen(ArrowVortexTag);
		if (String::startsWith(subTag, NotesTag))
		{
			Ascii85dec(buffer, subTag + strlen(NotesTag));
			PasteNotes(buffer.data(), buffer.size());
		}
		else if (String::startsWith(subTag, TempoTag))
		{
			Ascii85dec(buffer, subTag + strlen(TempoTag));
			PasteTempo(buffer.data(), buffer.size());
		}
	}
}

void Clipboard::copy()
{
	CopySelection(false);
}

void Clipboard::cut()
{
	CopySelection(true);
}

void Clipboard::paste()
{
	PasteClipboard();
}

} // namespace AV
