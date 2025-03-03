#pragma once

#include <Core/Interface/Widget.h>

namespace AV {

class WSeperator : public Widget
{
public:
	~WSeperator();
	WSeperator();

	void draw(bool enabled) override;
};

} // namespace AV
