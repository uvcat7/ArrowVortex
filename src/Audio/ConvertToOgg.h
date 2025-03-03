#pragma once

#include <Core/System/Thread.h>
#include <Core/System/Debug.h>
#include <Core/System/File.h>

namespace AV {

struct OggConversionThread : public BackgroundThread
{
	OggConversionThread();
	uchar progress;
	FilePath outPath;
	std::string error;
	void exec() override;
};

} // namespace AV
