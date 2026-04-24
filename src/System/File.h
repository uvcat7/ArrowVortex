// Utility functions for the ITG toolset.
// Copyright 2015-2016 Bram "Fietsemaker" van de Wetering.

#pragma once

#include <Core/Vector.h>

#include <filesystem>

namespace fs = std::filesystem;

namespace Vortex {
/// Returns a UTF-8 formatted string representation of a path.
/// Always use this when converting paths to strings.
extern std::string pathToUtf8(fs::path path);

/// Returns a u8string representation of a string.
/// Use this for various fs::path functions so that fs::path converts the UTF-8
/// to UTF-16 on Windows.
extern std::u8string stringToUtf8(std::string str);

/// Returns a path from a UTF-8 formatted string.
/// Always use this when creating paths that could have UTF-8 characters.
extern fs::path utf8ToPath(std::string str);

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
