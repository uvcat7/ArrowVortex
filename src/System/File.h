// Utility functions for the ITG toolset.
// Copyright 2015-2016 Bram "Fietsemaker" van de Wetering.

#pragma once

#include <Core/Vector.h>

#include <stdio.h>

namespace Vortex {

/// Path to a file or directory.
struct Path
{
	Path();
	Path(const Path& other);
	Path(const std::string& path);
	Path(const std::string& dir, const std::string& file);
	Path(const std::string& dir, const std::string& name, const std::string& ext);

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

	/// Returns the attributes of the file/directory.
	int attributes() const;

	/// Returns true if the path has the given extension, false otherwise (case-insensitive).
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
	Path operator + (const std::string& items) const;

	/// String representation of the entire path.
	std::string str;
};

/// Reads data from a file.
struct FileReader
{
	FileReader();
	~FileReader();

	bool open(const std::string& path);
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

	bool open(const std::string& path);
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
	extern long getSize(const std::string& path, bool* success);

	/// Returns a string with the contents of a file.
	extern std::string getText(const std::string& path, bool* success);

	/// Returns a vector with the contents of a file, split into lines.
	extern Vector<std::string> getLines(const std::string& path, bool* success);

	/// Moves or renames a file.
	extern bool moveFile(const std::string& path, const std::string& newPath, bool replace);

	/// Creates a folder.
	extern bool createFolder(const std::string& path);

	/// Deletes a file.
	extern bool deleteFile(const std::string& path);

	/// Deletes a folder.
	extern bool deleteFolder(const std::string& path);

	/// Returns a list of files if path is a directory, or a single file if path is a file.
	/// Filters is a string of acceptable extensions seperated by semicolons (e.g. "sm;ssc").
	extern Vector<Path> findFiles(const std::string& path, bool recursive = true, const char* filters = nullptr);

	/// Returns a list of all subdirectories in a directory.
	extern Vector<Path> findDirs(const std::string& path, bool recursive = true);

}; // namespace File.

}; // namespace Vortex
