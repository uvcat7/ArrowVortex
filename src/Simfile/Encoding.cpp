#include <Simfile/Encoding.h>

#include <Core/ByteStream.h>
#include <Core/Utils.h>
#include <Core/StringUtils.h>

#include <Managers/TempoMan.h>
#include <Managers/NoteMan.h>

#include <Editor/Common.h>

namespace Vortex {

// ================================================================================================
// Note encoding/decoding.

void EncodeNote(WriteStream& out, const Note& in)
{
	if(in.row == in.endrow && in.player == 0 && in.type == 0)
	{
		out.write<uchar>(in.col);
		out.writeNum(in.row);
	}
	else
	{
		out.write<uchar>(in.col | 0x80);
		out.writeNum(in.row);
		out.writeNum(in.endrow);
		out.write<uchar>((in.player << 4) | in.type);
	}
}

void DecodeNote(ReadStream& in, Note& out)
{
	uchar col = in.read<uchar>();
	if((col & 0x80) == 0)
	{
		int row = in.readNum();
		out = {row, row, col, 0, 0};
	}
	else
	{
		out.col = col & 0x7F;
		out.row = in.readNum();
		out.endrow = in.readNum();
		uint v = in.read<uchar>();
		out.player = v >> 4;
		out.type = v & 0xF;
	}
}

// ================================================================================================
// Note with time encoding/decoding.

void EncodeNoteWithTime(WriteStream& out, const ExpandedNote& in)
{
	if(in.time == in.endtime && in.player == 0 && in.type == 0)
	{
		out.write<uchar>(in.col);
		out.write(in.time);
	}
	else
	{
		out.write<uchar>(in.col | 0x80);
		out.write(in.time);
		out.write(in.endtime);
		out.write<uchar>((in.player << 4) | in.type);
	}
}

void DecodeNoteWithTime(ReadStream& in, ExpandedNote& out)
{
	uchar col = in.read<uchar>();
	if((col & 0x80) == 0)
	{
		double time = in.read<double>();
	}
	else
	{
		out.col = col & 0x7F;
		in.read(out.time);
		in.read(out.endtime);
		uint v = in.read<uchar>();
		out.player = v >> 4;
		out.type = v & 0xF;
	}
}

// ================================================================================================
// Notes encoding/decoding.

void EncodeNotes(WriteStream& out, const Note* in, int num)
{
	out.writeNum(num);
	for(int i = 0; i < num; ++i)
	{
		EncodeNote(out, in[i]);
	}
}

void DecodeNotes(ReadStream& in, Vector<Note>& out)
{
	Note note;
	uint num = in.readNum();
	for(uint i = 0; i < num; ++i)
	{
		DecodeNote(in, note);
		out.push_back(note);
	}
}

}; // namespace Vortex