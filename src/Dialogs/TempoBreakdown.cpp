#include <Dialogs/TempoBreakdown.h>

#include <Core/StringUtils.h>
#include <Core/Draw.h>

#include <Managers/TempoMan.h>
#include <Managers/SimfileMan.h>

#include <Simfile/SegmentList.h>
#include <Simfile/SegmentGroup.h>

#define Dlg DialogTempoBreakdown

namespace Vortex {

// ================================================================================================
// ChartList

static int GetTempoListH()
{
	auto segments = gTempo->getSegments();
	int h = 0;
	if(segments)
	{
		for(auto list = segments->begin(), listEnd = segments->end(); list != listEnd; ++list)
		{
			if(list->size())
			{
				h += 26 + list->size() * 16;
			}
		}
	}
	return max(h, 16);
}

struct Dlg::TempoList : public WgScrollRegion {

~TempoList()
{
}

TempoList(GuiContext* gui)
	: WgScrollRegion(gui)
{
	setScrollType(SCROLL_NEVER, SCROLL_WHEN_NEEDED);
}

void onUpdateSize() override
{
	scrollHeight_ = GetTempoListH();
	clampScrollValues_();
}

void onDraw() override
{
	if(gSimfile->isClosed()) return;

	int x = rect_.x;
	int y = rect_.y - scrollPositionY_;
	recti view = {rect_.x, rect_.y, getViewWidth(), getViewHeight()};
	
	TextStyle style;
	style.textFlags = 0;

	Renderer::pushScissorRect(view);

	Draw::fill({view.x + view.w / 2, view.y, 1, view.h}, Color32(26));

	auto segments = gTempo->getSegments();
	for(auto list = segments->begin(), listEnd = segments->end(); list != listEnd; ++list)
	{
		if(list->size())
		{
			auto meta = Segment::meta[list->type()];
			auto seg = list->begin(), segEnd = list->end();

			Draw::fill({x, y, view.w, 20}, Color32(26));
			Text::arrange(Text::MC, style, meta->plural);
			Text::draw({x, y, view.w, 20});
			y += 26;

			while(seg != segEnd && y < view.y - 20)
			{
				y += 16, ++seg;
			}
			while(seg != segEnd && y < view.y + view.h + 20)
			{
				String str = Str::val(seg->row * BEATS_PER_ROW, 3, 3);
				Text::arrange(Text::MR, style, str.str());
				Text::draw(vec2i{x + view.w / 2 - 6, y + 8});

				str = meta->getDescription(seg.ptr);
				Text::arrange(Text::ML, style, str.str());
				Text::draw(vec2i{x + view.w / 2 + 6, y + 8});

				y += 16, ++seg;
			}
		}
	}

	Renderer::popScissorRect();

	WgScrollRegion::onDraw();
}

}; // TimingData

// ================================================================================================
// DialogTempoBreakdown

Dlg::~Dlg()
{
	delete myList;
}

Dlg::Dlg()
{
	setTitle("TEMPO BREAKDOWN");
	setWidth(200);

	setMinimumHeight(32);
	setResizeable(false, true);

	myList = new TempoList(getGui());
}

void Dlg::onUpdateSize()
{
	myList->updateSize();
	int h = myList->getScrollHeight();
	setMinimumHeight(min(64, h));
	setMaximumHeight(min(1024, h));
}

void Dlg::onTick()
{
	myList->arrange(getInnerRect());
	myList->tick();
}

void Dlg::onDraw()
{
	myList->draw();
}

}; // namespace Vortex