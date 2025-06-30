#include <Simfile/Notes.h>

#include <Core/ByteStream.h>

namespace Vortex {

// ================================================================================================
// Regular note encoding.

void EncodeNote(WriteStream& out, const Note& in)
{
	if(in.row == in.endrow && in.player == 0 && in.type == 0)
	{
		out.write<uchar>(in.col);
		out.writeNum(in.row);
		out.write<uchar>(in.quant);
	}
	else
	{
		out.write<uchar>(in.col | 0x80);
		out.writeNum(in.row);
		out.writeNum(in.endrow);
		out.write<uchar>((in.player << 4) | in.type);
		out.write<uchar>(in.quant);
	}
}

void DecodeNote(ReadStream& in, Note& out)
{
	uchar col = in.read<uchar>();
	if((col & 0x80) == 0)
	{
		int row = in.readNum();
		out = {row, row, col, 0, 0, 0};
		out.quant = in.read<uchar>();
	}
	else
	{
		out.col = col & 0x7F;
		out.row = in.readNum();
		out.endrow = in.readNum();
		uint v = in.read<uchar>();
		out.player = v >> 4;
		out.type = v & 0xF;
		out.quant = in.read<uchar>();
	}
}

// ================================================================================================
// Time-based note encoding.

void EncodeNoteWithTime(WriteStream& out, const ExpandedNote& in)
{
	if(in.time == in.endtime && in.player == 0 && in.type == 0)
	{
		out.write<uchar>(in.col);
		out.write(in.time);
		out.write<uchar>(in.quant);
	}
	else
	{
		out.write<uchar>(in.col | 0x80);
		out.write(in.time);
		out.write(in.endtime);
		out.write<uchar>((in.player << 4) | in.type);
		out.write<uchar>(in.quant);
	}
}

void DecodeNoteWithTime(ReadStream& in, ExpandedNote& out)
{
	uchar col = in.read<uchar>();
	if((col & 0x80) == 0)
	{
		double time = in.read<double>();
		out.quant = in.read<uchar>();
	}
	else
	{
		out.col = col & 0x7F;
		in.read(out.time);
		in.read(out.endtime);
		uint v = in.read<uchar>();
		out.player = v >> 4;
		out.type = v & 0xF;
		out.quant = in.read<uchar>();
	}
}

// ================================================================================================
// Batch note encoding.

void EncodeNotes(WriteStream& out, const Note* in, int num)
{
	out.writeNum(num);
	for(int i = 0; i < num; ++i)
	{
		EncodeNote(out, in[i]);
	}
}

}; // namespace Vortex