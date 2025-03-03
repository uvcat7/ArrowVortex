#pragma once

#include <Core/Common/NonCopyable.h>
#include <Core/System/File.h>

namespace AV {
namespace Zip {

// Unzips the files in the given zip file to a temporary folder, and returns the path to that folder.
DirectoryPath unzipFile(const FilePath& path);

} // namespace Zip
} // namespace AV