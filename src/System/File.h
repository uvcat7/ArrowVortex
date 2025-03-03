// Utility functions for the ITG toolset.
// Copyright 2015-2016 Bram "Fietsemaker" van de Wetering.

#pragma once

#include <Core/String.h>
#include <Core/Vector.h>

#include <stdio.h>

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

	/// Returns the attributes of the file/directory.
	int attributes() const;

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

/// Reads data from a file.
struct FileReader
{
	FileReader();
	~FileReader();

	bool open(StringRef path);
	void close();

	size_t size() const;
	long tell() const;
	size_t read(void* ptr, size_t size, size_t count);
	int seek(long offset, int origin);
	void skip(size_t n);
	bool eof();	

	void* file;
};

/// Writes data to a file.
struct FileWriter
{
	FileWriter();
	~FileWriter();

	bool open(StringRef path);
	void close();

	size_t write(const void* ptr, size_t size, size_t count);
	void printf(const char* format, ...);

	void* file;
};

namespace File
{
	/// Enumeration of file/directory attributes.
	enum Attributes
	{
		ATR_EXISTS    = 0x1, ///< The file or directory could be found.
		ATR_DIR       = 0x2, ///< The path identifies a directory.
		ATR_HIDDEN    = 0x4, ///< The file or directory is hidden.
		ATR_READ_ONLY = 0x8, ///< The file is read-only.
	};

	/// Returns the size in bytes of a file.
	extern long getSize(StringRef path, bool* success);

	/// Returns a string with the contents of a file.
	extern String getText(StringRef path, bool* success);

	/// Returns a vector with the contents of a file, split into lines.
	extern Vector<String> getLines(StringRef path, bool* success);

	/// Moves or renames a file.
	extern bool moveFile(StringRef path, StringRef newPath, bool replace);

	/// Creates a folder.
	extern bool createFolder(StringRef path);

	/// Deletes a file.
	extern bool deleteFile(StringRef path);

	/// Deletes a folder.
	extern bool deleteFolder(StringRef path);

	/// Returns a list of files if path is a directory, or a single file if path is a file.
	/// Filters is a string of acceptable extensions seperated by semicolons (e.g. "sm;ssc").
	extern Vector<Path> findFiles(StringRef path, bool recursive = true, const char* filters = nullptr);

	/// Returns a list of all subdirectories in a directory.
	extern Vector<Path> findDirs(StringRef path, bool recursive = true);

}; // namespace File.

}; // namespace Vortex
