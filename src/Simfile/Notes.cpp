#include <Simfile/Notes.h>

#include <Core/ByteStream.h>

namespace Vortex {

// ================================================================================================
// Regular note encoding.

void EncodeNote(WriteStream& out, const Note& in) {
    if (in.row == in.endrow && in.player == 0 && in.type == 0) {
        out.write<uint8_t>(in.col);
        out.writeNum(in.row);
        out.write<uint8_t>(in.quant);
    } else {
        out.write<uint8_t>(in.col | 0x80);
        out.writeNum(in.row);
        out.writeNum(in.endrow);
        out.write<uint8_t>((in.player << 4) | in.type);
        out.write<uint8_t>(in.quant);
    }
}

void DecodeNote(ReadStream& in, Note& out) {
    uint8_t col = in.read<uint8_t>();
    if ((col & 0x80) == 0) {
        int row = in.readNum();
        out = {row, row, col, 0, 0, 0};
        out.quant = in.read<uint8_t>();
    } else {
        out.col = col & 0x7F;
        out.row = in.readNum();
        out.endrow = in.readNum();
        uint32_t v = in.read<uint8_t>();
        out.player = v >> 4;
        out.type = v & 0xF;
        out.quant = in.read<uint8_t>();
    }
}

// ================================================================================================
// Time-based note encoding.

void EncodeNoteWithTime(WriteStream& out, const ExpandedNote& in) {
    if (in.time == in.endtime && in.player == 0 && in.type == 0) {
        out.write<uint8_t>(in.col);
        out.write(in.time);
        out.write<uint8_t>(in.quant);
    } else {
        out.write<uint8_t>(in.col | 0x80);
        out.write(in.time);
        out.write(in.endtime);
        out.write<uint8_t>((in.player << 4) | in.type);
        out.write<uint8_t>(in.quant);
    }
}

void DecodeNoteWithTime(ReadStream& in, ExpandedNote& out) {
    uint8_t col = in.read<uint8_t>();
    if ((col & 0x80) == 0) {
        double time = in.read<double>();
        out.quant = in.read<uint8_t>();
    } else {
        out.col = col & 0x7F;
        in.read(out.time);
        in.read(out.endtime);
        uint32_t v = in.read<uint8_t>();
        out.player = v >> 4;
        out.type = v & 0xF;
        out.quant = in.read<uint8_t>();
    }
}

// ================================================================================================
// Batch note encoding.

void EncodeNotes(WriteStream& out, const Note* in, int num) {
    out.writeNum(num);
    for (int i = 0; i < num; ++i) {
        EncodeNote(out, in[i]);
    }
}

};  // namespace Vortex