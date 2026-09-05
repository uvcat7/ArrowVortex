#pragma once

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace Vortex {

/// Writes a zip archive, which is what an .osz and a .qp are.
///
/// Only what those formats need: one flat level of deflated files with their
/// names stored as UTF-8. Entries are written as they are added, so no more
/// than one file is held in memory at a time.
class ZipWriter {
   public:
    ~ZipWriter();
    ZipWriter();

    /// Creates the archive. Returns false if the file could not be opened.
    bool open(fs::path path);

    /// Adds a folder entry, which is what a reader needs to see before the
    /// files inside it. The name carries its own trailing slash.
    bool addFolder(const std::string& nameInArchive);

    /// Adds a file from disk under the given name. Returns false if the file
    /// could not be read.
    bool addFile(fs::path source, const std::string& nameInArchive);

    /// Writes the central directory and closes the file. Returns false if
    /// anything went wrong along the way.
    bool close();

   private:
    struct Entry {
        std::string name;
        bool isFolder;
        uint32_t crc;
        uint32_t compressedSize;
        uint32_t size;
        uint32_t offset;
    };

    std::ofstream myFile;
    std::vector<Entry> myEntries;
    bool myFailed;
};

};  // namespace Vortex
