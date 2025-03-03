#include <Precomp.h>

#include <filesystem>

#include <Core/Utils/Unicode.h>
#include <Core/Utils/Zip.h>
#include <Core/System/File.h>
#include <Core/System/Log.h>
#include <Core/Graphics/Image.h>

namespace AV {

using namespace std;
namespace fs = std::filesystem;

static constexpr uint32_t LfSignature = 0x04034b50; // "PK\3\4".
static constexpr uint32_t CdSignature = 0x02014b50;
static constexpr uint32_t EocdSignature = 0x06054b50;

#pragma pack(1)

struct LocalFileHeader
{
	uint32_t signature;
	uint16_t neededVersion;
	uint16_t flags;
	uint16_t compression;
	uint16_t modTime, modDate;
	uint32_t crc32;
	uint32_t compressedSize;
	uint32_t uncompressedSize;
	uint16_t filenameLen;
	uint16_t extrafieldLen;
};

struct CentralDirectoryHeader
{
	uint32_t signature;
	uint16_t version;
	uint16_t neededVersion;
	uint16_t flags;
	uint16_t compression;
	uint16_t modTime, modDate;
	uint32_t crc32;
	uint32_t compressedSize;
	uint32_t uncompressedSize;
	uint16_t filenameLen;
	uint16_t extrafieldLen;
	uint16_t fileCommentLen;
	uint16_t diskNum;
	uint16_t internalFileAttrib;
	uint32_t externalFileAttrib;
	uint32_t lfhOffset;
};

struct EndOfCentralDirectoryRecord
{
	uint32_t signature;
	uint16_t diskNr;
	uint16_t diskStart;
	uint16_t numRecordsOnDisk;
	uint16_t numRecordsTotal;
	uint32_t centralDirectorySize;
	uint32_t cdhOffset;
	uint16_t commentLen;
};

struct GZipHeader
{
	uint8_t magicbytes[2] = { 0x1f, 0x8b };
	uint8_t compression = 8;
	uint8_t fileFlags = 0;
	uint32_t timestamp = 0;
	uint8_t compressionFlags = 0;
	uint8_t operatingSystem = 0;
};

struct GZipFooter
{
	uint32_t crc32;
	uint32_t uncompressedSize;
};

#pragma pack()

static string Error(const char* msg)
{
	Log::error(msg);
	return string();
}

static fs::path GetTemporaryFolder()
{
	auto path = fs::temp_directory_path();
	path.append("ArrowVortex");
	if (fs::exists(path))
	{
		for (auto& it : fs::directory_iterator(path))
			fs::remove_all(it.path());
	}
	return path.wstring();
}

static bool WriteFile(const fs::path& root, stringref filename, const void* data, size_t size)
{
	auto path = root / Unicode::widen(filename);
	auto dir = path.parent_path();
	fs::create_directories(dir);
	if (!fs::exists(dir))
		return false;

	FileWriter file;
	if (!file.open(path.string()))
		return false;
	
	return file.write(data, size, 1) == 1;
}

DirectoryPath Zip::unzipFile(const FilePath& path)
{
	FileReader file;
	if (!file.open(path))
		return Error(format("Could not open '%s'", path.str).data());

	EndOfCentralDirectoryRecord eocd;
	if (file.seek(-(long)sizeof(eocd), SEEK_END) != 0 || file.read(&eocd, sizeof(eocd), 1) != 1)
		return Error("Invalid zip, could not read EOCD record.");

	if (eocd.signature != EocdSignature)
		return Error("Invalid zip, bad EOCD signature.");

	LocalFileHeader lfh;
	CentralDirectoryHeader cdh;
	vector<uint8_t> buffer, deflated;

	auto folder = GetTemporaryFolder();
	long cdhOffset = eocd.cdhOffset;
	for (int i = 0; i < eocd.numRecordsTotal; ++i)
	{
		if (file.seek(cdhOffset, SEEK_SET) != 0 || file.read(&cdh, sizeof(cdh), 1) != 1)
			return Error("Invalid zip, could not read CD header.");

		if (cdh.signature != CdSignature)
			return Error("Invalid zip, bad CD signature.");

		string filename(cdh.filenameLen, 0);
		if (file.read(filename.data(), cdh.filenameLen, 1) != 1)
			return Error("Invalid zip, bad filename.");

		if (file.seek(cdhOffset, SEEK_SET) != 0 || file.read(&cdh, sizeof(cdh), 1) != 1)
			return Error("Invalid zip, could not read CD header.");

		cdhOffset += sizeof(cdh) + cdh.filenameLen + cdh.extrafieldLen + cdh.fileCommentLen;

		if (file.seek(cdh.lfhOffset, SEEK_SET) != 0 || file.read(&lfh, sizeof(lfh), 1) != 1)
			return Error("Invalid zip, could not read LF header.");

		buffer.resize(lfh.compressedSize);
		long dataOffset = cdh.lfhOffset + sizeof(lfh) + lfh.filenameLen + lfh.extrafieldLen;
		if (file.seek(dataOffset, SEEK_SET) != 0 || file.read(buffer.data(), buffer.size(), 1) != 1)
			return Error("Invalid zip, cound not read file data.");

		if (lfh.compression == 0)
		{
			WriteFile(folder, filename, buffer.data(), buffer.size());
		}
		else if (lfh.compression == 8)
		{
			deflated.resize(lfh.uncompressedSize);
			if (!Zlib::deflate(buffer.data(), buffer.size(), deflated.data(), deflated.size()))
				return Error("Invalid zip, error deflating file data.");
			WriteFile(folder, filename, deflated.data(), deflated.size());
		}
		else return Error(format("Unsupported zip, compression {}", lfh.compression).data());
	}
	return folder.string();
}

} // namespace AV