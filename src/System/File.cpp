#include <System/File.h>
#include <System/Debug.h>

#include <Core/StringUtils.h>

#include <string>
#include <fstream>
#include <filesystem>

namespace Vortex {

// ================================================================================================
// File utilities.
namespace File {

std::string getText(fs::path path, bool* success) {
    std::ifstream in(path.c_str());
    if (in.fail()) {
        HudError("Failed to open file: %s", strerror(errno));
        if (success != nullptr) *success = false;
        return {};
    }

    std::string str(std::istreambuf_iterator<char>(in), {});
    if (success != nullptr) *success = true;
    return str;
}

Vector<std::string> getLines(fs::path path, bool* success) {
    std::ifstream in(path.c_str());
    if (in.fail()) {
        HudError("Failed to open file: %s", strerror(errno));
        if (success != nullptr) *success = false;
        return {};
    }

    Vector<std::string> v;
    std::string line;

    while (std::getline(in, line)) v.push_back(line);

    if (success != nullptr) *success = true;
    return v;
}

static bool HasValidExt(fs::path path, const Vector<std::string>& filters) {
    auto ext = std::string(
        reinterpret_cast<const char*>(path.extension().u8string().c_str()));
    for (auto& filter : filters) {
        if (Str::iequal(filter, ext)) {
            return true;
        }
    }
    return filters.empty();
}

template <typename DirectoryIter>
static void AddFilesInDir(Vector<fs::path>& out, const DirectoryIter& it,
                          bool findDirs, const Vector<std::string>& filters) {
    for (const auto& entry : it) {
        if (fs::is_regular_file(entry) && !findDirs) {
            if (HasValidExt(entry, filters)) out.push_back(entry);
        }
        if (fs::is_directory(entry) && findDirs) out.push_back(entry);
    }
}

static void AddFilesInDir(Vector<fs::path>& out, fs::path path, bool recursive,
                          bool findDirs, const Vector<std::string>& filters) {
    if (fs::is_regular_file(path)) return;

    if (recursive)
        AddFilesInDir(out, fs::recursive_directory_iterator(path), findDirs,
                      filters);
    else
        AddFilesInDir(out, fs::directory_iterator(path), findDirs, filters);
}

Vector<fs::path> findFiles(fs::path path, bool recursive, const char* filters) {
    Vector<fs::path> out;

    if (path.empty()) return out;

    // Extract filters from the filter String.
    Vector<std::string> filterlist;
    if (filters) {
        for (const char *begin = filters, *end = begin; true; end = begin) {
            while (*end && *end != ';') ++end;
            if (end != begin)
                filterlist.push_back(
                    std::string(begin, static_cast<int>(end - begin)));
            if (*end == 0) break;
            begin = end + 1;
        }
    }

    // If the given path is not a directory but a file, return it as-is.
    if (fs::is_regular_file(path) && HasValidExt(path, filterlist))
        out.push_back(path);

    if (fs::is_directory(path))
        AddFilesInDir(out, path, recursive, false, filterlist);

    return out;
}

Vector<fs::path> findDirs(fs::path path, bool recursive) {
    Vector<fs::path> out;
    AddFilesInDir(out, path, recursive, true, {});
    return out;
}

};  // namespace File.
};  // namespace Vortex
