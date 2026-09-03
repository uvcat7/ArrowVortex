#include <System/Zip.h>

#include <zlib.h>

#include <cstring>

namespace Vortex {
namespace {

// ================================================================================================
// Little endian writing, which is what the zip format is defined in.

void Put16(std::ofstream& out, uint32_t v) {
    char b[2] = {static_cast<char>(v & 0xFF),
                 static_cast<char>((v >> 8) & 0xFF)};
    out.write(b, 2);
}

void Put32(std::ofstream& out, uint32_t v) {
    char b[4] = {static_cast<char>(v & 0xFF),
                 static_cast<char>((v >> 8) & 0xFF),
                 static_cast<char>((v >> 16) & 0xFF),
                 static_cast<char>((v >> 24) & 0xFF)};
    out.write(b, 4);
}

// The name is UTF-8, which the reader is told about by bit 11 of the flags.
const uint32_t FLAG_UTF8 = 0x0800;
const uint32_t METHOD_STORE = 0;
const uint32_t METHOD_DEFLATE = 8;

// Deflates the whole of a file into memory. The archive holds an audio file
// and a couple of images, so a file at a time is not much to carry.
bool Deflate(fs::path source, std::vector<char>& out, uint32_t& crc,
             uint32_t& size) {
    std::ifstream in(source, std::ios::binary);
    if (in.fail()) return false;

    z_stream stream = {};
    // A negative window size asks for a raw deflate stream, without the zlib
    // header that a zip entry does not carry.
    if (deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -MAX_WBITS, 8,
                     Z_DEFAULT_STRATEGY) != Z_OK) {
        return false;
    }

    crc = crc32(0, nullptr, 0);
    size = 0;

    std::vector<char> inBuf(64 * 1024);
    std::vector<char> outBuf(64 * 1024);

    bool done = false;
    while (!done) {
        in.read(inBuf.data(), inBuf.size());
        auto read = static_cast<uint32_t>(in.gcount());
        done = (read < inBuf.size());

        crc = crc32(crc, reinterpret_cast<const Bytef*>(inBuf.data()), read);
        size += read;

        stream.next_in = reinterpret_cast<Bytef*>(inBuf.data());
        stream.avail_in = read;

        do {
            stream.next_out = reinterpret_cast<Bytef*>(outBuf.data());
            stream.avail_out = static_cast<uInt>(outBuf.size());
            deflate(&stream, done ? Z_FINISH : Z_NO_FLUSH);
            out.insert(out.end(), outBuf.data(),
                       outBuf.data() + (outBuf.size() - stream.avail_out));
        } while (stream.avail_out == 0);
    }

    deflateEnd(&stream);
    return true;
}

};  // anonymous namespace

// ================================================================================================
// ZipWriter.

ZipWriter::~ZipWriter() = default;

ZipWriter::ZipWriter() : myFailed(false) {}

bool ZipWriter::open(fs::path path) {
    myFile.open(path, std::ios::binary | std::ios::trunc);
    myFailed = myFile.fail();
    return !myFailed;
}

bool ZipWriter::addFolder(const std::string& nameInArchive) {
    if (myFailed) return false;

    Entry entry;
    entry.name = nameInArchive;
    entry.isFolder = true;
    entry.crc = 0;
    entry.compressedSize = 0;
    entry.size = 0;
    entry.offset = static_cast<uint32_t>(myFile.tellp());

    Put32(myFile, 0x04034b50);  // local file header
    Put16(myFile, 20);          // version needed
    Put16(myFile, FLAG_UTF8);
    Put16(myFile, METHOD_STORE);
    Put16(myFile, 0);  // modification time
    Put16(myFile, 0);  // modification date
    Put32(myFile, 0);  // crc
    Put32(myFile, 0);  // compressed size
    Put32(myFile, 0);  // size
    Put16(myFile, static_cast<uint32_t>(entry.name.length()));
    Put16(myFile, 0);  // extra field length
    myFile.write(entry.name.data(), entry.name.length());

    myEntries.push_back(entry);
    myFailed = myFile.fail();
    return !myFailed;
}

bool ZipWriter::addFile(fs::path source, const std::string& nameInArchive) {
    if (myFailed) return false;

    std::vector<char> data;
    uint32_t crc = 0, size = 0;
    if (!Deflate(source, data, crc, size)) return false;

    Entry entry;
    entry.name = nameInArchive;
    entry.isFolder = false;
    entry.crc = crc;
    entry.compressedSize = static_cast<uint32_t>(data.size());
    entry.size = size;
    entry.offset = static_cast<uint32_t>(myFile.tellp());

    Put32(myFile, 0x04034b50);  // local file header
    Put16(myFile, 20);          // version needed
    Put16(myFile, FLAG_UTF8);
    Put16(myFile, METHOD_DEFLATE);
    Put16(myFile, 0);  // modification time
    Put16(myFile, 0);  // modification date
    Put32(myFile, entry.crc);
    Put32(myFile, entry.compressedSize);
    Put32(myFile, entry.size);
    Put16(myFile, static_cast<uint32_t>(entry.name.length()));
    Put16(myFile, 0);  // extra field length
    myFile.write(entry.name.data(), entry.name.length());
    myFile.write(data.data(), data.size());

    myEntries.push_back(entry);
    myFailed = myFile.fail();
    return !myFailed;
}

bool ZipWriter::close() {
    if (!myFile.is_open()) return false;

    auto start = static_cast<uint32_t>(myFile.tellp());

    for (auto& entry : myEntries) {
        Put32(myFile, 0x02014b50);  // central directory entry
        Put16(myFile, 20);          // version made by
        Put16(myFile, 20);          // version needed
        Put16(myFile, FLAG_UTF8);
        Put16(myFile, entry.isFolder ? METHOD_STORE : METHOD_DEFLATE);
        Put16(myFile, 0);  // modification time
        Put16(myFile, 0);  // modification date
        Put32(myFile, entry.crc);
        Put32(myFile, entry.compressedSize);
        Put32(myFile, entry.size);
        Put16(myFile, static_cast<uint32_t>(entry.name.length()));
        Put16(myFile, 0);  // extra field length
        Put16(myFile, 0);  // comment length
        Put16(myFile, 0);  // disk number
        Put16(myFile, 0);  // internal attributes
        // 0x10 is the directory attribute, which is how a reader that
        // ignores the trailing slash still sees a folder.
        Put32(myFile, entry.isFolder ? 0x10 : 0);  // external attributes
        Put32(myFile, entry.offset);
        myFile.write(entry.name.data(), entry.name.length());
    }

    auto size = static_cast<uint32_t>(myFile.tellp()) - start;

    Put32(myFile, 0x06054b50);  // end of central directory
    Put16(myFile, 0);           // disk number
    Put16(myFile, 0);           // disk with the central directory
    Put16(myFile, static_cast<uint32_t>(myEntries.size()));
    Put16(myFile, static_cast<uint32_t>(myEntries.size()));
    Put32(myFile, size);
    Put32(myFile, start);
    Put16(myFile, 0);  // comment length

    myFailed = myFailed || myFile.fail();
    myFile.close();
    return !myFailed;
}

};  // namespace Vortex
