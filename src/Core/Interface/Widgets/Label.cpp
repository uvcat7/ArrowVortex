#include <Precomp.h>

#include <Core/Utils/Flag.h>

#include <Core/Graphics/Draw.h>
#include <Core/Graphics/Text.h>

#include <Core/Interface/UiStyle.h>
#include <Core/Interface/Widgets/Label.h>

namespace AV {

WLabel::~WLabel()
{
}

WLabel::WLabel()
{
}

WLabel::WLabel(stringref text)
	: myText(text)
{
}

WLabel::WLabel(const char* text)
	: myText(text)
{
}

void WLabel::draw(bool enabled)
{
	Rect r = myRect;

	auto textStyle = UiText::regular;
	textStyle.flags = TextOptions::Markup | TextOptions::Ellipses;
	if (!isEnabled()) textStyle.textColor = Color(166, 255);

	Text::setStyle(textStyle);
	Text::format(TextAlign::ML, myText);
	Text::draw(r.shrink(2,0));
}

void WLabel::setText(const char* text)
{
	myText = text;
}

} // namespace AV
