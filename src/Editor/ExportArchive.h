#pragma once

#include <Simfile/Simfile.h>

namespace Vortex {

/// Packs the song into an .osz, the archive osu! reads: a flat zip holding
/// the chart files, the music and the artwork.
///
/// Asks for the destination, and reports what it did in the message area.
void ExportArchive();

};  // namespace Vortex
