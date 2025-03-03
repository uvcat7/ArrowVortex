#pragma once

#include <System/Thread.h>
#include <System/Debug.h>

#include <Core/String.h>

namespace Vortex {

struct OggConversionThread : public BackgroundThread
{
	OggConversionThread();
	uchar progress;
	String outPath, error;
	void exec() override;
};

}; // namespace Vortex