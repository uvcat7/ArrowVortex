#include <Precomp.h>

#include <Vortex/View/Dialogs/Keysounds.h>

#include <Core/Utils/String.h>
#include <Core/Graphics/Draw.h>
#include <Core/Utils/Util.h>

#include <Vortex/Edit/TempoTweaker.h>

#include <Simfile/Simfile.h>
#include <Simfile/Tempo/SegmentSet.h>

#include <Vortex/Editor.h>

#define Dlg DialogKeysounds

namespace AV {

using namespace std;
using namespace Util;

// =====================================================================================================================
// ChartList

static int GetKeysoundListH()
{
	int h = 0;

	auto sim = Editor::currentSimfile();
	if (sim) h += (int)sim->keysounds.size() * 16;

	return max(h, 16);
}

struct Dlg::KeysoundList : public Widget {

~KeysoundList()
{
}

KeysoundList()
{
}

void draw(bool enabled) override
{
	auto sim = Editor::currentSimfile();
	if (!sim) return;

	/*

	int x = myRect.l;
	int y = myRect.t - myScrollY;
	Rect view = {myRect.l, myRect.t, getViewWidth(), getViewHeight()};
	
	TextStyle::Style style;
	style.flags = 0;

	Renderer::pushScissorRect(view);

	Draw::fill(Rect(view.cx(), view.t, view.cx() + 1, view.b), Color(26));

	auto& keysounds = Editor::currentSimfile()->keysounds;
	auto it = keysounds.begin();
	auto end = keysounds.end();
	int index = 0;

	while (it != end && y < view.y - 20)
	{
		y += 16, ++it, ++index;
	}
	while (it != end && y < view.y + view.h + 20)
	{
		string str = String::val(++index);
		auto text = textStyle.arrange(TextAlign::MR, style, str.data());
		Text::draw(x + 28, y + 8);

		auto text = textStyle.arrange(TextAlign::ML, style, it->str());
		Text::draw(x + 36, y + 8);

		y += 16, ++it;
	}

	Renderer::popScissorRect();

	WScrollRegion::draw();*/
}

}; // TimingData

// =====================================================================================================================
// DialogKeysounds

Dlg::~Dlg()
{
}

Dlg::Dlg()
{
	setTitle("KEYSOUNDS");

	setResizeBehaviorH(ResizeBehavior::Unbound);
	setResizeBehaviorV(ResizeBehavior::Bound);

	myContent = myList = make_shared<KeysoundList>();
}

void Dlg::onUpdateSize()
{
}

void Dlg::tick(bool enabled)
{
	myList->arrange(getContentRect());
	myList->tick(enabled);
}

void Dlg::draw(bool enabled)
{
	myList->draw(enabled);
}

} // namespace AV
