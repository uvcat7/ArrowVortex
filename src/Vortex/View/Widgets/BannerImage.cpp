#include <Precomp.h>

#include <Core/Utils/Util.h>

#include <Core/Graphics/Text.h>
#include <Core/Graphics/Texture.h>

#include <Core/Interface/UiStyle.h>

#include <Vortex/View/Widgets/BannerImage.h>

namespace AV {

using namespace Util;

WBannerImage::WBannerImage()
{
}

void WBannerImage::onMeasure()
{
	if (banner.texture)
	{
		auto source = banner.texture.get();
		myWidth = myMinWidth = clamp(source->width(), 200, 600);
		myHeight = myMinHeight = clamp(source->height(), 50, 200);
	}
	else
	{
		myWidth = myMinWidth = 100;
		myHeight = myMinHeight = 20;
	}
}

void WBannerImage::draw(bool enabled)
{
	Renderer::pushScissorRect(myRect);
	if (banner.texture)
	{
		banner.draw(myRect);
	}
	else
	{
		Text::setStyle(UiText::regular, Color(100, 255));
		Text::setFontSize(8);
		Text::setShadowColor(Color::Blank);
		Text::format(TextAlign::MC, "[ no banner ]");
		Text::draw(myRect);
	}
	Renderer::popScissorRect();
}

} // namespace AV
