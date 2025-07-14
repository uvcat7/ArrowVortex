#include <System/File.h>
#include <System/Debug.h>

#include <Core/WideString.h>
#include <Core/StringUtils.h>

#include <string>
#include <fstream>
#include <filesystem>

namespace Vortex {
namespace {

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
// File utilities.

namespace File {

String getText(StringRef path, bool* success)
{
	size_t size = std::filesystem::file_size(path.str());
	String str(static_cast<int>(size) + 1, '\0');
	std::ifstream in(path.str());
	if (in.fail()) 
	{
		HudError("Failed to open file: %s", strerror(errno));
		if (success != nullptr)
			*success = false;
		return {};
	}

	in.read(str.begin(), size);
	if (success != nullptr)
		*success = true;
	return str;
}

Vector<String> getLines(StringRef path, bool* success)
{
	std::ifstream in(path.str());
	if (in.fail())
	{
		HudError("Failed to open file: %s", strerror(errno));
		if (success != nullptr)
			*success = false;
		return {};
	}

	Vector<String> v;
	std::string line;

	while (std::getline(in, line))
		v.push_back(line.c_str());

	if (success != nullptr)
		*success = true;
	return v;
}

bool moveFile(StringRef path, StringRef newPath, bool replace)
{
	try
	{
		if (replace && fs::exists(newPath.str()))
			fs::remove(newPath.str());
		fs::rename(path.str(), newPath.str());
		return true;
	}
	catch (const fs::filesystem_error& e)
	{
		Debug::blockBegin(Debug::ERROR, "could not move file");
		Debug::log("path1: %s\n", e.path1());
		Debug::log("path2: %s\n", e.path2());
		Debug::log("windows error code: %s\n", e.what());
		Debug::blockEnd();
		return false;
	}
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

template<typename DirectoryIter>
static void AddFilesInDir(Vector<Path>& out, const DirectoryIter& it, bool findDirs, const Vector<String>& filters)
{
	for (const auto& entry : it)
	{
		Path aw_path(entry.path().string().c_str());
		if (fs::is_regular_file(entry) && !findDirs)
		{
			if (HasValidExt(aw_path, filters))
				out.push_back(aw_path);
		}

		if (fs::is_directory(entry) && findDirs)
			out.push_back(aw_path);
	}
}

static void AddFilesInDir(Vector<Path>& out, const WideString& wpath, bool recursive, bool findDirs, const Vector<String>& filters)
{
	fs::path path(wpath.str());
	if (fs::is_regular_file(path))
		return;

	if (recursive)
		AddFilesInDir(out, fs::recursive_directory_iterator(path), findDirs, filters);
	else
		AddFilesInDir(out, fs::directory_iterator(path), findDirs, filters);
}

Vector<Path> findFiles(StringRef path, bool recursive, const char* filters)
{
	if (path.empty()) return {};

	Vector<Path> out;

	// Extract filters from the filter String.
	Vector<String> filterlist;
	if (filters)
	{
		for (const char* begin = filters, *end = begin; true; end = begin)
		{
			while (*end && *end != ';') ++end;
			if (end != begin) filterlist.push_back(String(begin, static_cast<int>(end - begin)));
			if (*end == 0) break;
			begin = end + 1;
		}
	}

	// If the given path is not a directory but a file, return it as-is.
	if (fs::is_regular_file(path.str()) && HasValidExt(path, filterlist))
		out.push_back(path);

	if (fs::is_directory(path.str()))
		AddFilesInDir(out, Widen(path), recursive, false, filterlist);

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
