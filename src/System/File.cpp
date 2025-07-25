#include <System/File.h>
#include <System/Debug.h>

#include <Core/WideString.h>
#include <Core/StringUtils.h>

#include <vector>
#include <array>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#undef DeleteFile
#undef MoveFile
#undef ERROR

#include <errno.h>
#include <stdio.h>

namespace Vortex {
namespace {

// ================================================================================================
// Utility functions.

static FILE* OpenFile(StringRef path, bool write)
{
	FILE* file;
	WideString wpath = Widen(path);
	if(!(file = _wfopen(wpath.str(), write ? L"wb" : L"rb")))
	{
		const char* reason = "file not found";
		if(errno == EACCES) reason = "permission denied, file might be read only";
		else if(write) reason = "unable to create file";
		Debug::blockBegin(Debug::ERROR, "could not open file");
		Debug::log("file: %s\n", path.str());
		Debug::log("reason: %s\n", reason);
		Debug::blockEnd();
	}
	return (FILE*)file;
}

// ================================================================================================
// Path iteration functions.

// Returns a pointer to the first character past the prefix of path.
static const char* GetDirStart(StringRef path)
{
	auto p = path.str();
	if(p[0] == '\\' && p[1] == '\\')
	{
		p += 2;
		if((p[0] == '?' || p[0] == '.') && p[1] == '\\') p += 2;
	}
	if(p[0] && p[1] == ':')
	{
		p += 2;
		if(p[0] == '\\') ++p;
	}
	return p;
}

// Returns true if the path is an absolute path, false otherwise.
static bool IsAbsolutePath(StringRef path)
{
	return GetDirStart(path) != path.str();
}

// Returns a pointer to the end of the top-most directory in the path.
static const char* GetDirEnd(StringRef path)
{
	auto p = GetDirStart(path), out = p;
	for(; *p; ++p) { if(p[0] == '\\') out = p; }
	return out;
}

// Returns a pointer to the first character of the filename in path.
static const char* GetFileStart(StringRef path)
{
	auto p = GetDirStart(path), out = p;
	for(; *p; ++p) { if(p[0] == '\\') out = p + 1; }
	return out;
}

// Returns a pointer to the end of the file (the part before the extension).
static const char* GetFileEnd(const char* filename)
{
	auto p = filename, out = p;
	for(; *p; ++p) { if(*p == '.') out = p; }
	if(out == filename) out = p;
	return out;
}

// Returns a pointer to the start of the extension (after the period).
static const char* GetExtStart(const char* filename)
{
	auto out = filename, p = out;
	for(; *p; ++p) { if(*p == '.') out = p + 1; }
	if(out == filename) out = p;
	return out;
}

// Returns a pointer to the start of the top-most directory in the path.
static const char* GetTopDir(StringRef path)
{
	auto p = GetDirStart(path), out = p, tmp = p;
	for(; *p; ++p) { if(p[0] == '\\') out = tmp, tmp = p + 1; }
	return out;
}

// Returns a pointer to the start of the top-most item in the path.
static const char* GetTopItem(StringRef path)
{
	auto p = GetDirStart(path), out = p;
	for(; *p; ++p) { if(p[0] == '\\' && p[1]) out = p + 1; }
	return out;
}

// ================================================================================================
// Path manipulation functions.

enum EndSlash { SLASH_YES, SLASH_NO, SLASH_AS_IS };

struct PathItem
{
	inline bool dotdot() const { return (n == 2) && (p[0] == '.') && (p[1] == '.'); }
	const char* p;
	size_t n;
};

// Returns true if the given String ends with a slash character.
static bool EndsWithSlash(StringRef path)
{
	return path.len() && (path.back() == '\\' || path.back() == '/');
}

// Adds items from a path to the list.
static void AddItems(Vector<PathItem>& out, StringRef path)
{
	auto begin = GetDirStart(path), end = path.end();
	for(const char* a = begin, *b = begin; *a; ++b)
	{
		if(*b == '\\' || *b == '/' || *b == 0)
		{
			int size = static_cast<int>(b - a);
			if(size == 2 && a[0] == '.' && a[1] == '.')
			{
				if(out.size() && !out.back().dotdot())
					out.pop_back();
				else
					out.push_back({a, (size_t)(b - a)});
			}
			else if(size > 1 || (size == 1 && a[0] != '.'))
			{
				out.push_back({a, (size_t)(b - a)});
			}
			a = *b ? (b + 1) : b;
		}
	}
}

// Concatenates two paths, resolving navigation elements and turning slashes forward.
static String Concatenate(StringRef first, StringRef second, EndSlash slash = SLASH_AS_IS)
{
	// Split the path into a list of items.
	const String* pathBegin;
	Vector<PathItem> items;
	if(first.empty() || IsAbsolutePath(second))
	{
		pathBegin = &second;
		AddItems(items, second);
	}
	else
	{
		pathBegin = &first;
		AddItems(items, first);
		AddItems(items, second);
	}

	// First, copy the characters in the path prefix.
	const char* dirBegin = GetDirStart(*pathBegin);
	String out(pathBegin->begin(), static_cast<int>(dirBegin - pathBegin->begin()));

	// If the first character after the prefix was a slash, append a slash.
	if(*dirBegin == '\\' || *dirBegin == '/') Str::append(out, '\\');

	// If the path is empty, we're done.
	if(items.empty()) return out;

	// Reconstruct the rest of the path from the items.
	auto i = items.begin();
	Str::append(out, i->p, static_cast<int>(i->n));
	for(++i; i != items.end(); ++i)
	{
		Str::append(out, '\\');
		Str::append(out, i->p, static_cast<int>(i->n));
	}

	// End with a slash if requested.
	if(slash == SLASH_YES)
	{
		Str::append(out, '\\');
	}
	else if(slash == SLASH_AS_IS)
	{
		StringRef pathEnd = second.len() ? second : first;
		if(EndsWithSlash(pathEnd)) Str::append(out, '\\');
	}

	return out;
}

// Returns a pointer to the first character of the extension of filename p.
static const char* GetExtension(const char* p)
{
	const char* out = p;
	for(; *p; ++p) { if(*p == '.') out = p; }
	if(*out != '.') out = p;
	return out;
}

}; // anonymous namespace.

// ================================================================================================
// Path.

Path::Path()
{
}

Path::Path(const Path& other)
	: str(other.str)
{
}

Path::Path(StringRef path)
	: str(Concatenate(String(), path))
{
}

Path::Path(StringRef dir, StringRef file)
	: str(Concatenate(dir, file))
{
}

Path::Path(StringRef dir, StringRef file, StringRef ext)
{
	if(ext.empty())
	{
		str = Concatenate(dir, file);
	}
	else
	{
		str = Concatenate(dir, file + '.' + ext);
	}
}

void Path::push(String items, bool slash)
{
	str = Concatenate(str, items, slash ? SLASH_YES : SLASH_NO);
}

void Path::push(String items)
{
	str = Concatenate(str, items);
}

void Path::pop()
{
	str = Concatenate(str, "..", SLASH_YES);
}

void Path::clear()
{
	str.clear();
}

void Path::dropExt()
{
	auto file = GetFileStart(str);
	Str::erase(str, static_cast<int>(GetFileEnd(file) - str.begin()));
}

void Path::dropFile()
{
	auto file = GetFileStart(str);
	Str::erase(str, static_cast<int>(file - str.begin()));
}

int Path::attributes() const
{
	DWORD out = 0, a = GetFileAttributesW(Widen(str).str());
	if(a != INVALID_FILE_ATTRIBUTES)
	{
		out |= File::ATR_EXISTS;
		if(a & FILE_ATTRIBUTE_DIRECTORY) out |= File::ATR_DIR;
		if(a & FILE_ATTRIBUTE_HIDDEN)    out |= File::ATR_HIDDEN;
		if(a & FILE_ATTRIBUTE_READONLY)  out |= File::ATR_READ_ONLY;
	}
	return out;
}

bool Path::hasExt(const char* ext) const
{
	auto file = GetFileStart(str);
	return Str::iequal(GetExtStart(file), ext);
}

String Path::name() const
{
	auto file = GetFileStart(str);
	return Str::create(file, GetFileEnd(file));
}

String Path::filename() const
{
	auto file = GetFileStart(str);
	return Str::create(file, str.end());
}

String Path::ext() const
{
	auto file = GetFileStart(str), end = str.end();
	return Str::create(GetExtStart(file), end);
}

String Path::dir() const
{
	return Str::create(str.str(), GetFileStart(str));
}

String Path::dirWithoutSlash() const
{
	return Str::create(str.str(), GetDirEnd(str));
}

String Path::topdir() const
{
	return Str::create(GetTopDir(str), GetFileStart(str));
}

String Path::top() const
{
	return Str::create(GetTopItem(str), str.end());
}

String Path::brief() const
{
	String out = top();
	if(out.len() > 20)
	{
		Str::erase(out, 3, out.len() - 16);
		Str::insert(out, 3, '~');
	}
	return out;
}

Path Path::operator + (StringRef items) const
{
	Path out(*this);
	out.push(items);
	return out;
}

// ================================================================================================
// File reader.

FileReader::FileReader() : file(nullptr)
{
}

FileReader::~FileReader()
{
	close();
}

bool FileReader::open(StringRef path)
{
	close();
	file = OpenFile(path, false);
	return (file != nullptr);
}

void FileReader::close()
{
	if (file)
	{
		fclose(static_cast<FILE*>(file));
		file = nullptr;
	}
}

size_t FileReader::size() const
{
	if (!file) return 0;
	long pos = ftell(static_cast<FILE*>(file));
	fseek(static_cast<FILE*>(file), 0, SEEK_END);
	size_t size = ftell(static_cast<FILE*>(file));
	fseek(static_cast<FILE*>(file), pos, SEEK_SET);
	return size;
}

long FileReader::tell() const
{
	return file ? ftell(static_cast<FILE*>(file)) : -1;
}

size_t FileReader::read(void* ptr, size_t size, size_t count)
{
	return file ? fread(ptr, size, count, static_cast<FILE*>(file)) : 0;
}

int FileReader::seek(long offset, int origin)
{
	return file ? fseek(static_cast<FILE*>(file), offset, origin) : -1;
}

void FileReader::skip(size_t n)
{
	if (file) fseek(static_cast<FILE*>(file), static_cast<long>(n), SEEK_CUR);
}

bool FileReader::eof()
{
	return file ? feof(static_cast<FILE*>(file)) != 0 : true;
}

// ================================================================================================
// File writer.

FileWriter::FileWriter() : file(nullptr)
{
}

FileWriter::~FileWriter()
{
	close();
}

bool FileWriter::open(StringRef path)
{
	close();
	file = OpenFile(path, true);
	return (file != nullptr);
}

void FileWriter::close()
{
	if(file)
	{
		fclose(static_cast<FILE*>(file));
	}
	file = nullptr;
}

size_t FileWriter::write(const void* ptr, size_t size, size_t count)
{
	return fwrite(ptr, size, count, (FILE*)file);
}

void FileWriter::printf(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vfprintf(static_cast<FILE*>(file), fmt, args);
	va_end(args);
}

// ================================================================================================
// File utilities.

namespace File {

static bool isNewline(char c)
{
	return (c == '\n' || c == '\r');
}

long getSize(StringRef path)
{
	FILE* fp = OpenFile(path, false);
	if(!fp) return 0;
	fseek(fp, 0, SEEK_END);
	long size = ftell(fp);
	fclose(fp);
	return size;
}

String getText(StringRef path, bool* success)
{
	FILE* fp = OpenFile(path, false);
	if(!fp) { if(success) *success = false;  return String(); }
	fseek(fp, 0, SEEK_END);
	long size = ftell(fp);
	String out(size, 0);
	fseek(fp, 0, SEEK_SET);
	fread(out.begin(), 1, size, fp);
	fclose(fp);
	if(success) *success = true;
	return out;
}

Vector<String> getLines(StringRef path, bool* success)
{
	constexpr size_t kBufferSize = 256;
	constexpr size_t kNumberOne = 1;

	Vector<String> out;
	FILE* fp = OpenFile(path, false);
	if(!fp) { if(success) *success = false; return out; }
	out.append();
	std::array<char, kBufferSize> buffer;
	for (size_t bytesRead; bytesRead = fread(buffer.data(), kNumberOne, buffer.size(), fp);)
	{
		if(bytesRead > 0 && isNewline(buffer[0]) && out.back().len())
		{
			out.append();
		}
		for (size_t pos = 0, end = 0; pos < bytesRead;)
		{
			while(pos < bytesRead && isNewline(buffer[pos]))
			{
				++pos, ++end;
			}
			while(end < bytesRead && !isNewline(buffer[end]))
			{
				++end;
			}
			if(end > pos)
			{
				Str::append(out.back(), buffer.data() + pos, static_cast<int>(end - pos));
			}
			if(end < bytesRead && isNewline(buffer[end]))
			{
				out.append();
			}
			pos = end;
		}
	}
	if(out.back().empty())
	{
		out.pop_back();
	}
	fclose(fp);
	if(success) *success = true;
	return out;
}

static void LogMoveFileError(StringRef path, StringRef newPath)
{
	int code = GetLastError();
	Debug::blockBegin(Debug::ERROR, "could not move file");
	Debug::log("old path: %s\n", path.str());
	Debug::log("new path: %s\n", newPath.str());
	Debug::log("windows error code: %i\n", code);
	Debug::blockEnd();
}

bool moveFile(StringRef path, StringRef newPath, bool replace)
{
	WideString wpath = Widen(path), wnew = Widen(newPath);
	DWORD flags = replace ? MOVEFILE_REPLACE_EXISTING : 0;
	BOOL result = MoveFileExW(wpath.str(), wnew.str(), flags);
	if(result == FALSE) LogMoveFileError(path, newPath);
	return result != FALSE;
}

bool createFolder(StringRef path)
{
	WideString wpath = Widen(path);
	return (CreateDirectoryW(wpath.str(), nullptr) != 0);
}

bool deleteFile(StringRef path)
{
	WideString wpath = Widen(path);
	return (DeleteFileW(wpath.str()) != 0);
}

bool deleteFolder(StringRef path)
{
	WideString wpath = Widen(path);
	wpath.push_back(0);
	SHFILEOPSTRUCTW file_op =
	{
		NULL, FO_DELETE, wpath.str(), L"\0\0",
		FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT,
		false, 0, L""
	};
	return (SHFileOperationW(&file_op) == 0);
}

static bool HasValidExt(StringRef filename, const Vector<String>& filters)
{
	const char* ext = GetExtStart(filename.str());
	for(auto& filter : filters)
	{
		if(Str::iequal(filter, ext))
		{
			return true;
		}
	}
	return filters.empty();
}

static void AddFilesInDir(Vector<Path>& out, const WideString& path, bool recursive, bool findDirs, const Vector<String>& filters)
{
	WIN32_FIND_DATAW ffd;
	WideString searchpath = path;
	searchpath.append(L"\\*");
	HANDLE hFind = FindFirstFileW(searchpath.str(), &ffd);
	if(hFind != INVALID_HANDLE_VALUE)
	{
		do {
			if(wcscmp(ffd.cFileName, L".") != 0 && wcscmp(ffd.cFileName, L"..") != 0)
			{
				bool isSubDirectory = (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
				if(isSubDirectory)
				{
					WideString subpath(path);
					subpath.append(L"\\");
					subpath.append(ffd.cFileName);
					if(recursive)
					{
						AddFilesInDir(out, subpath, true, findDirs, filters);
					}
					if(findDirs)
					{
						out.push_back({Narrow(subpath), String(), String()});
					}
				}
				else
				{
					if(!findDirs)
					{
						String filename = Narrow(ffd.cFileName);
						if(HasValidExt(filename, filters))
						{
							out.push_back({Narrow(path), filename});
						}
					}
				}
			}
		} while(FindNextFileW(hFind, &ffd) != 0);
	}
	FindClose(hFind);
}

Vector<Path> findFiles(StringRef path, bool recursive, const char* filters)
{
	Vector<Path> out;

	if(path.empty()) return out;

	// Extract filters from the filter String.
	Vector<String> filterlist;
	if(filters)
	{
		for(const char* begin = filters, *end = begin; true; end = begin)
		{
			while(*end && *end != ';') ++end;
			if (end != begin) filterlist.push_back(String(begin, static_cast<int>(end - begin)));
			if(*end == 0) break;
			begin = end + 1;
		}
	}

	// If the given path is not a directory but a file, return it as-is.
	WideString wpath = Widen(path);
	DWORD attr = GetFileAttributesW(wpath.str());
	if(attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY) == 0)
	{
		if(HasValidExt(path.str(), filterlist))
		{
			out.push_back(path);
		}
	}
	else // Search for files.
	{
		AddFilesInDir(out, wpath, recursive, false, filterlist);
	}

	return out;
}

Vector<Path> findDirs(StringRef path, bool recursive)
{
	Vector<Path> out;
	AddFilesInDir(out, Widen(path), recursive, true, Vector<String>());
	return out;
}

}; // namespace File.
}; // namespace Vortex
