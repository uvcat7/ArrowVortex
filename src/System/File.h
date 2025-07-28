// Utility functions for the ITG toolset.
// Copyright 2015-2016 Bram "Fietsemaker" van de Wetering.

#pragma once

#include <Core/Vector.h>

#include <filesystem>

namespace fs = std::filesystem;

namespace Vortex {

/// Path to a file or directory.
struct Path {
    Path();
    Path(const Path& other);
    Path(const std::string& path);
    Path(const std::string& dir, const std::string& file);
    Path(const std::string& dir, const std::string& name,
         const std::string& ext);

    /// Appends one or more items at the end of the path.
    void push(std::string items, bool endWithSlash);

    /// Appends one or more items at the end of the path.
    void push(std::string items);

    /// Removes the top-most item from the past.
    void pop();

    /// Empties the path String.
    void clear();

    /// Removes the extension portion of the path.
    void dropExt();

    /// Removes the filename portion of the path.
    void dropFile();

    /// Returns true if the path has the given extension, false otherwise
    /// (case-insensitive).
    bool hasExt(const char* ext) const;

    /// Returns the name of the file, without extension.
    std::string name() const;

    /// Returns the name of the file, including extension.
    std::string filename() const;

    /// Returns the extension of the file at the end of the path.
    std::string ext() const;

    /// Returns the directory portion of the path.
    std::string dir() const;

    /// Returns the directory portion of the path without a final slash.
    std::string dirWithoutSlash() const;

    /// Returns the name of the top-most directory in the path.
    std::string topdir() const;

    /// Returns the name of the top-most item in the path.
    std::string top() const;

    /// Returns the name of the top-most item, shortened to 20 characters.
    std::string brief() const;

    /// Returns the string representation of the entire path.
    operator std::string() const { return str; }

    /// Returns the path that would result from push(items);
    Path operator+(const std::string& items) const;

    /// String representation of the entire path.
    std::string str;
};

namespace File {
/// Returns a string with the contents of a file.
extern std::string getText(const std::string& path, bool* success);

/// Returns a vector with the contents of a file, split into lines.
extern Vector<std::string> getLines(const std::string& path, bool* success);

/// Moves or renames a file.
extern bool moveFile(const std::string& path, const std::string& newPath,
                     bool replace);

/// Returns a list of files if path is a directory, or a single file if path is
/// a file. Filters is a string of acceptable extensions seperated by semicolons
/// (e.g. "sm;ssc").
extern Vector<Path> findFiles(const std::string& path, bool recursive = true,
                              const char* filters = nullptr);

/// Returns a list of all subdirectories in a directory.
extern Vector<Path> findDirs(const std::string& path, bool recursive = true);

};  // namespace File.

};  // namespace Vortex
