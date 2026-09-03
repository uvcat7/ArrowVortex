#pragma once

#include <string>

#include <Simfile/Simfile.h>

namespace Vortex {

/// The name the archive of that format is offered under: "Artist - Title"
/// for osu!, the name of the song folder for StepMania.
std::string SuggestedArchiveName(SimFormat format);

/// Packs the song into the archive its game reads: an .osz for osu!, a .zip
/// holding the song folder for StepMania. Both hold the chart files, the
/// music and the artwork the simfile points at.
///
/// Asks where the archive goes, and reports what it did in the message area.
void ExportArchive(SimFormat format, const std::string& name);

};  // namespace Vortex
