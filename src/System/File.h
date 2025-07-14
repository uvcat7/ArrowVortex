// Utility functions for the ITG toolset.
// Copyright 2015-2016 Bram "Fietsemaker" van de Wetering.

#pragma once

#include <Core/String.h>
#include <Core/Vector.h>

#include <filesystem>

namespace fs = std::filesystem;

namespace Vortex {

/// Path to a file or directory.
struct Path
{
	Path();
	Path(const Path& other);
	Path(StringRef path);
	Path(StringRef dir, StringRef file);
	Path(StringRef dir, StringRef name, StringRef ext);

	/// Appends one or more items at the end of the path.
	void push(String items, bool endWithSlash);

	/// Appends one or more items at the end of the path.
	void push(String items);

	/// Removes the top-most item from the past.
	void pop();

	/// Empties the path String.
	void clear();

	/// Removes the extension portion of the path.
	void dropExt();

	/// Removes the filename portion of the path.
	void dropFile();

	/// Returns true if the path has the given extension, false otherwise (case-insensitive).
	bool hasExt(const char* ext) const;

	/// Returns the name of the file, without extension.
	String name() const;

	/// Returns the name of the file, including extension.
	String filename() const;

	/// Returns the extension of the file at the end of the path.
	String ext() const;

	/// Returns the directory portion of the path.
	String dir() const;

	/// Returns the directory portion of the path without a final slash.
	String dirWithoutSlash() const;

	/// Returns the name of the top-most directory in the path.
	String topdir() const;

	/// Returns the name of the top-most item in the path.
	String top() const;

	/// Returns the name of the top-most item, shortened to 20 characters.
	String brief() const;

	/// Returns the string representation of the entire path.
	operator StringRef() const { return str; }

	/// Returns the path that would result from push(items);
	Path operator + (StringRef items) const;

	/// String representation of the entire path.
	String str;
};

namespace File
{
	/// Returns a string with the contents of a file.
	String getText(StringRef path, bool* success);

	/// Returns a vector with the contents of a file, split into lines.
	Vector<String> getLines(StringRef path, bool* success);

	/// Moves or renames a file.
	bool moveFile(StringRef path, StringRef newPath, bool replace);

	/// Returns a list of files if path is a directory, or a single file if path is a file.
	/// Filters is a string of acceptable extensions seperated by semicolons (e.g. "sm;ssc").
	Vector<Path> findFiles(StringRef path, bool recursive = true, const char* filters = nullptr);

	/// Returns a list of all subdirectories in a directory.
	Vector<Path> findDirs(StringRef path, bool recursive = true);

}; // namespace File.

}; // namespace Vortex
