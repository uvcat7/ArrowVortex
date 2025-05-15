#pragma once

#include <Editor/Common.h>

namespace Vortex {

struct NotefieldPreview
{
	static void create();
	static void destroy();

	virtual void draw() = 0;

	virtual void toggleEnabled() = 0;
};

extern NotefieldPreview* gNotefieldPreview;

}; // namespace Vortex