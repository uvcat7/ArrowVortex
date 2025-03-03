#pragma once

#include <Core/Graphics/Draw.h>

#include <Core/Interface/Widgets/VerticalTabs.h>

namespace AV {

struct WBannerImage : public Widget
{
	WBannerImage();

	void onMeasure() override;
	void draw(bool enabled) override;

	Sprite banner;
};

} // namespace AV
