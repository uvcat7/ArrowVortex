#include <Precomp.h>

#include <Core/Graphics/Draw.h>

#include <Core/Interface/Widgets/Seperator.h>

namespace AV {

WSeperator::~WSeperator()
{
}

WSeperator::WSeperator()
{
}

void WSeperator::draw(bool enabled)
{
	Draw::fill(Rect(myRect.l, myRect.cy() - 1, myRect.r, myRect.cy()), Color(13, 255));
	Draw::fill(Rect(myRect.l, myRect.cy(), myRect.r, myRect.cy() + 1), Color(77, 255));
}

} // namespace AV
