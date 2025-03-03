#include <Precomp.h>

#include <Core/Utils/Util.h>
#include <Core/Utils/String.h>

#include <Core/Graphics/Text.h>
#include <Core/Graphics/Draw.h>

#include <Simfile/Simfile.h>
#include <Simfile/Tempo.h>

#include <Vortex/Editor.h>
#include <Vortex/Settings.h>

#include <Vortex/View/View.h>
#include <Vortex/Notefield/Notefield.h>
#include <Vortex/Notefield/Background.h>
#include <Vortex/Notefield/NotefieldPosTracker.h>

namespace AV {

using namespace std;

// =====================================================================================================================
// Draw.

void Background::draw(const Sprite& image)
{
	auto& settings = Settings::view();
	if (image.texture && settings.bgBrightness != 0)
	{
		auto color = Color(settings.bgBrightness * 255 / 100, 255);
		image.draw(View::rect(), color, Sprite::Orientation::Normal, settings.bgMode);
	}
}

} // namespace AV
