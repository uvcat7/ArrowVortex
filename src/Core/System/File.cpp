#include <Precomp.h>

#include <Core/Utils/Unicode.h>
#include <Core/System/File.h>
#include <Core/System/Log.h>
#include <Core/System/Debug.h>
#include <Core/Utils/String.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>

#undef ERROR

namespace AV {

using namespace std;

// =====================================================================================================================
// Helper functions.

enum class PathType { File, Directory, Invalid };

inline bool isNewline(char c)
{
	return c == '\n' || c == '\r';
}

inline size_t lastSlash(stringref path)
{
	return path.find_last_of("\\/", string::npos, 2);
}

inline size_t stemLength(stringref filename)
{
	auto p = filename.find_last_of('.');
	if (p == string::npos)
		return filename.length();

	auto n = filename.find_first_not_of('.');
	if (n > p)
		return filename.length();

	return p;
}

static void concat(string& base, stringref extraElements)
{
	if (!base.empty() && base.back() != '\\' && base.back() != '/')
		base.push_back('\\');

	base.append(extraElements);
}

static FILE* openFile(const FilePath& path, bool write, bool append = false)
{
	FILE* file;
	auto pathw = Unicode::widen(path.str);
	if (!(file = _wfopen(pathw.data(), write ? (append ? L"ab" : L"wb") : L"rb")))
	{
		const char* reason = "file not found";
		if (errno == EACCES) reason = "permission denied, file might be read only";
		else if (write) reason = "unable to create file";
		Log::blockBegin("Could not open file.");
		Log::error(format("file: {}", path.str));
		Log::error(format("reason: {}", reason));
		Log::blockEnd();
	}
	return file;
}

static wstring widen(const FilePath& path)
{
	return Unicode::widen(path.str);
}

static wstring widen(const DirectoryPath& path)
{
	return Unicode::widen(path.str);
}

static PathType getPathType(stringref path)
{
	auto attr = GetFileAttributesW(Unicode::widen(path).data());
	if (attr == INVALID_FILE_ATTRIBUTES)
		return PathType::Invalid;

	if (attr & FILE_ATTRIBUTE_DIRECTORY)
		return PathType::Directory;

	return PathType::File;
}

static void iterateDirectory(
	vector<string>& out,
	const wstring& dir,
	bool recursive,
	bool findDirs,
	const wstring& ext,
	stringref prefix)
{
	WIN32_FIND_DATAW ffd;
	wstring search(dir);
	if (ext.empty())
	{
		search.append(L"\\*");
	}
	else
	{
		search.append(L"\\*.");
		search.append(ext);
	}
	HANDLE hFind = FindFirstFileW(search.data(), &ffd);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do {
			if (wcscmp(ffd.cFileName, L".") != 0 && wcscmp(ffd.cFileName, L"..") != 0)
			{
				bool isDirectory = (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
				if (isDirectory)
				{
					if (recursive || findDirs)
					{
						string subprefix = prefix + Unicode::narrow(ffd.cFileName);
						if (findDirs)
						{
							out.push_back(subprefix);
						}
						if (recursive)
						{
							wstring subdir(dir);
							subdir.append(L"\\");
							subdir.append(ffd.cFileName);
							subprefix.push_back('\\');
							iterateDirectory(out, subdir, true, findDirs, ext, subprefix);
						}
					}
				}
				else if (!findDirs)
				{
					out.push_back(prefix + Unicode::narrow(ffd.cFileName));
				}
			}
		} while (FindNextFileW(hFind, &ffd) != 0);
	}
	FindClose(hFind);
}

// =====================================================================================================================
// DirectoryPath.

DirectoryPath::DirectoryPath()
{
}

DirectoryPath::DirectoryPath(stringref path)
	: str(path)
{
}

DirectoryPath::DirectoryPath(const char* path)
	: str(path)
{
}

DirectoryPath::DirectoryPath(const DirectoryPath& base, stringref relative)
	: str(base.str)
{
	concat(str, relative);
}

string DirectoryPath::name() const
{
	auto s = lastSlash(str);
	return s != string::npos ? str.substr(s + 1) : str;
}

DirectoryPath DirectoryPath::parent() const
{
	auto s = lastSlash(str);
	return s != string::npos ? DirectoryPath(str.substr(0, s)) : DirectoryPath();
}

// =====================================================================================================================
// FilePath.

FilePath::FilePath()
{
}

FilePath::FilePath(stringref path)
	: str(path)
{
}

FilePath::FilePath(const char* path)
	: str(path)
{
}

FilePath::FilePath(const DirectoryPath& dir, stringref filename)
	: str(dir.str)
{
	concat(str, filename);
}

FilePath::FilePath(const DirectoryPath& dir, stringref stem, stringref extension)
	: str(dir.str)
{
	concat(str, stem + "." + extension);
}

DirectoryPath FilePath::directory() const
{
	auto s = lastSlash(str);
	return s != string::npos ? DirectoryPath(str.substr(0, s)) : DirectoryPath();
}

string FilePath::filename() const
{
	auto s = lastSlash(str);
	return s != string::npos ? str.substr(s + 1) : str;
}

string FilePath::stem() const
{
	auto f = filename();
	f.resize(stemLength(f));
	return f;
}

string FilePath::extension() const
{
	auto f = filename();
	auto l = stemLength(f);
	auto e =  (l < f.length()) ? f.substr(l + 1) : string();
	String::toLower(e);
	return e;
}

// =====================================================================================================================
// File.

bool FileSystem::isDirectory(stringref path)
{
	return getPathType(path) == PathType::Directory;
}

bool FileSystem::exists(const FilePath& path)
{
	return getPathType(path.str) == PathType::File;
}

bool FileSystem::exists(const DirectoryPath& path)
{
	return getPathType(path.str) == PathType::Directory;
}

bool FileSystem::destroy(const DirectoryPath& path)
{
	auto wpath = widen(path);
	wpath.push_back((wchar_t)0);
	FILEOP_FLAGS flags = FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT;
	SHFILEOPSTRUCTW file_op = { NULL, FO_DELETE, wpath.data(), L"\0\0", flags, false, 0, L"" };
	return SHFileOperationW(&file_op) == 0;
}

bool FileSystem::destroy(const FilePath& path)
{
	return DeleteFileW(widen(path).data()) != 0;
}

bool FileSystem::create(const DirectoryPath& path)
{
	return CreateDirectoryW(widen(path).data(), nullptr) != 0;
}

bool FileSystem::move(const FilePath& from, const FilePath& to, bool replaceExisting)
{
	DWORD flags = replaceExisting ? MOVEFILE_REPLACE_EXISTING : 0;
	BOOL result = MoveFileExW(widen(from).data(), widen(to).data(), flags);
	if (result == FALSE)
	{
		int code = GetLastError();
		Log::blockBegin("Could not move file.");
		Log::error(format("old path: {}", from.str));
		Log::error(format("new path: {}", to.str));
		Log::error(format("windows error code: {}", code));
		Log::blockEnd();
	}
	return result != FALSE;
}

bool FileSystem::readText(const FilePath& path, std::string& out)
{
	out.clear();
	FILE* fp = openFile(path, false);
	if (!fp) return false;
	fseek(fp, 0, SEEK_END);
	int size = (int)ftell(fp);
	out.resize(size);
	fseek(fp, 0, SEEK_SET);
	fread(&out[0], 1, size, fp);
	fclose(fp);
	return true;
}

bool FileSystem::readLines(const FilePath& path, vector<std::string>& out)
{
	out.clear();
	FILE* fp = openFile(path, false);
	if (!fp) return false;
	out.emplace_back();
	char buffer[256];
	for (size_t bytesRead; bytesRead = fread(buffer, 1, 256, fp);)
	{
		if (bytesRead > 0 && isNewline(buffer[0]) && out.back().length())
		{
			out.emplace_back();
		}
		for (size_t pos = 0, end = 0; pos < bytesRead;)
		{
			while (pos < bytesRead && isNewline(buffer[pos]))
			{
				++pos, ++end;
			}
			while (end < bytesRead && !isNewline(buffer[end]))
			{
				++end;
			}
			if (end > pos)
			{
				out.back().append(buffer + pos, end - pos);
			}
			if (end < bytesRead && isNewline(buffer[end]))
			{
				out.emplace_back();
			}
			pos = end;
		}
	}
	if (out.back().empty())
	{
		out.pop_back();
	}
	fclose(fp);
	return true;
}

vector<string> FileSystem::listFiles(const DirectoryPath& dir, bool recursive, const char* ext)
{
	vector<string> out;
	auto wext = ext ? Unicode::widen(ext) : wstring();
	iterateDirectory(out, widen(dir), recursive, false, wext, string());
	return out;
}

vector<string> FileSystem::listDirectories(const DirectoryPath& dir, bool recursive)
{
	vector<string> out;
	iterateDirectory(out, widen(dir), recursive, true, wstring(), string());
	return out;
}

// =====================================================================================================================
// File :: Static data.

const int Seek::Set = SEEK_SET;
const int Seek::Cur = SEEK_CUR;
const int Seek::End = SEEK_END;

// =====================================================================================================================
// File :: File reader.

FileReader::FileReader() : file(nullptr)
{
}

FileReader::~FileReader()
{
	close();
}

bool FileReader::open(const FilePath& path)
{
	close();
	file = openFile(path, false);
	return (file != nullptr);
}

void FileReader::close()
{
	if (file) fclose(file);
	file = nullptr;
}

size_t FileReader::size() const
{
	long pos = ftell(file);
	fseek(file, 0, SEEK_END);
	size_t size = ftell(file);
	fseek(file, pos, SEEK_SET);
	return size;
}

long FileReader::tell() const
{
	return ftell(file);
}

size_t FileReader::read(void* ptr, size_t size, size_t count)
{
	return fread(ptr, size, count, file);
}

int FileReader::seek(long offset, int origin)
{
	return fseek(file, offset, origin);
}

void FileReader::skip(long n)
{
	fseek(file, n, SEEK_CUR);
}

bool FileReader::eof()
{
	return (feof(file) != 0);
}

// =====================================================================================================================
// File :: File writer.

FileWriter::FileWriter() : file(nullptr)
{
}

FileWriter::~FileWriter()
{
	close();
}

bool FileWriter::open(const FilePath& path, bool append)
{
	close();
	file = openFile(path, true, append);
	return (file != nullptr);
}

void FileWriter::close()
{
	if (file) fclose(file);
	file = nullptr;
}

size_t FileWriter::write(const void* ptr, size_t size, size_t count)
{
	return fwrite(ptr, size, count, file);
}

void FileWriter::printf(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vfprintf(file, fmt, args);
	va_end(args);
}

} // namespace AV
