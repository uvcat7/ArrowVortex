#pragma once

#include <Core/Common/NonCopyable.h>

namespace AV {

// Represents a path to a directory.
struct DirectoryPath
{
	// Constructs an empty path.
	DirectoryPath();

	// Constructs a path from a string.
	DirectoryPath(stringref path);

	// Constructs a path from a string.
	DirectoryPath(const char* path);

	// Constructs a path by concatenating a base path and relative path.
	DirectoryPath(const DirectoryPath& base, stringref relative);

	// Returns the directory name.
	std::string name() const;

	// Returns the path of the parent directory.
	DirectoryPath parent() const;

	// The path string.
	std::string str;

	auto operator <=> (const DirectoryPath&) const = default;
};

// Represents a path to a file.
struct FilePath
{
	// Constructs an empty path.
	FilePath();

	// Constructs a path from a string.
	FilePath(stringref path);

	// Constructs a path from a string.
	FilePath(const char* path);

	// Constructs a path from a directory and filename.
	FilePath(const DirectoryPath& dir, stringref filename);

	// Constructs a path from a directory, filename stem and extension.
	FilePath(const DirectoryPath& dir, stringref stem, stringref extension);

	// The filename portion of the path, including extension.
	std::string filename() const;

	// The filename portion of the path without extension.
	std::string stem() const;

	// The file extension, in lowercase.
	std::string extension() const;

	// The directory containing the file.
	DirectoryPath directory() const;

	// The path string.
	std::string str;

	auto operator <=> (const FilePath&) const = default;
};

// File system utilities.
namespace FileSystem
{
	// Returns whether the given path points to a directory.
	bool isDirectory(stringref path);
	
	// Returns whether a file exists.
	bool exists(const FilePath& path);
	
	// Returns whether a directory exists.
	bool exists(const DirectoryPath& path);
	
	// Deletes a directory.
	bool destroy(const DirectoryPath& path);
	
	// Creates a directory.
	bool create(const DirectoryPath& path);
	
	// Deletes a file.
	bool destroy(const FilePath& path);
	
	// Moves or renames a file.
	bool move(const FilePath& from, const FilePath& to, bool replaceExisting);
	
	// Returns the contents of a file as a string.
	bool readText(const FilePath& path, std::string& out);
	
	// Returns the contents of a file, split into line strings.
	bool readLines(const FilePath& path, vector<std::string>& out);
	
	// Lists the filenames of all files in the given directory that match the provided extension.
	vector<std::string> listFiles(const DirectoryPath& dir, bool recursive = false, const char* ext = nullptr);
	
	// Lists the names of all subdirectories in the given directory.
	vector<std::string> listDirectories(const DirectoryPath& dir, bool recursive = false);
};

// Contains seek origins.
struct Seek
{
	static const int Set; // Offset from the start of the file.
	static const int Cur; // Offset from the current read/write position.
	static const int End; // Offset from the end of the file.
};

// Reads data from a file.
struct FileReader : NonCopyable
{
	FileReader();
	~FileReader();

	bool open(const FilePath& path);
	void close();

	size_t size() const;
	long tell() const;
	size_t read(void* ptr, size_t size, size_t count);
	int seek(long offset, int origin);
	void skip(long n);
	bool eof();

	FILE* file;
};

// Writes data to a file.
struct FileWriter : NonCopyable
{
	FileWriter();
	~FileWriter();

	bool open(const FilePath& path, bool append = false);
	void close();

	size_t write(const void* ptr, size_t size, size_t count);
	void printf(const char* format, ...);

	FILE* file;
};

} // namespace AV
