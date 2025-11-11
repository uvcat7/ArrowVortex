// Utility functions for the ITG toolset.
// Copyright 2015-2016 Bram "Fietsemaker" van de Wetering.

#pragma once

#include <Core/Vector.h>

#include <filesystem>

namespace fs = std::filesystem;

namespace Vortex {
namespace File {
/// Returns a string with the contents of a file.
extern std::string getText(fs::path path, bool* success);

/// Returns a vector with the contents of a file, split into lines.
extern Vector<std::string> getLines(fs::path path, bool* success);

/// Returns a list of files if path is a directory, or a single file if path is
/// a file. Filters is a string of acceptable extensions seperated by semicolons
/// (e.g. "sm;ssc").
extern Vector<fs::path> findFiles(fs::path path, bool recursive = true,
                                  const char* filters = nullptr);

/// Returns a list of all subdirectories in a directory.
extern Vector<fs::path> findDirs(fs::path path, bool recursive = true);

};  // namespace File.

};  // namespace Vortex
