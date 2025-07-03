#include <Core/GuiContext.h>
#include <Core/GuiWidget.h>
#include <Core/GuiDialog.h>
#include <Core/MapUtils.h>
#include <Core/VectorUtils.h>

namespace Vortex {

GuiContext::~GuiContext()
{
}

GuiContextImpl::~GuiContextImpl()
{
	Vec::release(dialogs_);
}

GuiContextImpl::GuiContextImpl()
{
	viewRect_ = {0, 0, INT_MAX, INT_MAX};
	inputEvents_ = nullptr;
}

recti GuiContextImpl::getView()
{
	return viewRect_;
}

vec2i GuiContextImpl::getMousePos()
{
	return mousePosition_;
}

float GuiContextImpl::getDeltaTime()
{
	return deltaTime_;
}

InputEvents& GuiContextImpl::getEvents()
{
	return *inputEvents_;
}

void GuiContextImpl::tick(recti view, float deltaTime, InputEvents& events)
{
	viewRect_ = view;

	viewRect_.w = max(viewRect_.w, 0);
	viewRect_.h = max(viewRect_.h, 0);

	deltaTime_ = deltaTime;
	inputEvents_ = &events;

	// Update the mouse position.
	for(MouseMove* move = nullptr; events.next(move);)
	{
		mousePosition_ = {move->x, move->y};
	}

	// Rearrange or delete dialogs if requested.
	FOR_VECTOR_REVERSE(dialogs_, i)
	{
		auto dialog = dialogs_[i];
		if(dialog->requestClose_)
		{
			dialogs_.erase_values(dialog);
			delete dialog;
		}
		else if(dialog->requestMoveToTop_)
		{
			dialogs_.erase(i);
			dialogs_.push_back(dialog);
			dialog->requestMoveToTop_ = 0;
		}
	}

	// Arrange the dialogs and widgets.
	FOR_VECTOR_REVERSE(dialogs_, i)
	{
		dialogs_[i]->arrange();
	}

	// Tick widgets with focus first.
	FOR_VECTOR_REVERSE(focusWidgets_, i)
	{
		if(focusWidgets_[i]->isEnabled())
		{
			focusWidgets_[i]->onTick();
		}
	}

	// Tick the dialogs.
	FOR_VECTOR_REVERSE(dialogs_, i)
	{
		dialogs_[i]->tick();
	}

	inputEvents_ = nullptr;
}

void GuiContextImpl::draw()
{
	// Draw the dialogs.
	FOR_VECTOR_FORWARD(dialogs_, i)
	{
		dialogs_[i]->draw();
	}

	// Draw widgets with focus at the top.
	FOR_VECTOR_FORWARD(focusWidgets_, i)
	{
		focusWidgets_[i]->onDraw();
	}
}

void GuiContextImpl::removeWidget(GuiWidget* w)
{
	focusWidgets_.erase_values(w);
}

void GuiContextImpl::addDialog(DialogData* f)
{
	dialogs_.push_back(f);
}

void GuiContextImpl::removeDialog(DialogData* f)
{
	dialogs_.erase_values(f);
}

void GuiContextImpl::grabFocus(GuiWidget* w)
{
	FOR_VECTOR_FORWARD(focusWidgets_, i)
	{
		if(focusWidgets_[i] == w) return;
	}
	focusWidgets_.push_back(w);
}

void GuiContextImpl::releaseFocus(GuiWidget* w)
{
	focusWidgets_.erase_values(w);
}

// ================================================================================================
// GuiContextImpl :: slot bindings.

bool GuiContextImpl::bind(StringRef slot, const int* v)
{
	auto s = Map::findVal(valueSlots_, slot);
	if(s) (*s)->bind(v);
	return s != nullptr;
}

bool GuiContextImpl::bind(StringRef slot, const uint* v)
{
	auto s = Map::findVal(valueSlots_, slot);
	if(s) (*s)->bind(v);
	return s != nullptr;
}

bool GuiContextImpl::bind(StringRef slot, const long* v)
{
	auto s = Map::findVal(valueSlots_, slot);
	if(s) (*s)->bind(v);
	return s != nullptr;
}

bool GuiContextImpl::bind(StringRef slot, const ulong* v)
{
	auto s = Map::findVal(valueSlots_, slot);
	if(s) (*s)->bind(v);
	return s != nullptr;
}

bool GuiContextImpl::bind(StringRef slot, const float* v)
{
	auto s = Map::findVal(valueSlots_, slot);
	if(s) (*s)->bind(v);
	return s != nullptr;
}

bool GuiContextImpl::bind(StringRef slot, const double* v)
{
	auto s = Map::findVal(valueSlots_, slot);
	if(s) (*s)->bind(v);
	return s != nullptr;
}

bool GuiContextImpl::bind(StringRef slot, const bool* v)
{
	auto s = Map::findVal(valueSlots_, slot);
	if(s) (*s)->bind(v);
	return s != nullptr;
}

bool GuiContextImpl::bind(StringRef slot, const char* str)
{
	auto s = Map::findVal(textSlots_, slot);
	if(s) (*s)->bind(str);
	return s != nullptr;
}

bool GuiContextImpl::bind(StringRef slot, const String* str)
{
	auto s = Map::findVal(textSlots_, slot);
	if(s) (*s)->bind(str);
	return s != nullptr;
}

bool GuiContextImpl::bind(StringRef slot, int* v)
{
	auto s = Map::findVal(valueSlots_, slot);
	if(s) (*s)->bind(v);
	return s != nullptr;
}

bool GuiContextImpl::bind(StringRef slot, uint* v)
{
	auto s = Map::findVal(valueSlots_, slot);
	if(s) (*s)->bind(v);
	return s != nullptr;
}

bool GuiContextImpl::bind(StringRef slot, long* v)
{
	auto s = Map::findVal(valueSlots_, slot);
	if(s) (*s)->bind(v);
	return s != nullptr;
}

bool GuiContextImpl::bind(StringRef slot, ulong* v)
{
	auto s = Map::findVal(valueSlots_, slot);
	if(s) (*s)->bind(v);
	return s != nullptr;
}

bool GuiContextImpl::bind(StringRef slot, float* v)
{
	auto s = Map::findVal(valueSlots_, slot);
	if(s) (*s)->bind(v);
	return s != nullptr;
}

bool GuiContextImpl::bind(StringRef slot, double* v)
{
	auto s = Map::findVal(valueSlots_, slot);
	if(s) (*s)->bind(v);
	return s != nullptr;
}

bool GuiContextImpl::bind(StringRef slot, bool* v)
{
	auto s = Map::findVal(valueSlots_, slot);
	if(s) (*s)->bind(v);
	return s != nullptr;
}

bool GuiContextImpl::bind(StringRef slot, String* str)
{
	auto s = Map::findVal(textSlots_, slot);
	if(s) (*s)->bind(str);
	return s != nullptr;
}

bool GuiContextImpl::bind(StringRef slot, Functor::Generic* f)
{
	auto s = Map::findVal(callSlots_, slot);
	if(s) (*s)->bind(f); else delete f;
	return s != nullptr;
}

void GuiContextImpl::addSlot(ValueSlot* slot, const char* name)
{
	valueSlots_.insert({name, slot});
}

void GuiContextImpl::addSlot(TextSlot* slot, const char* name)
{
	textSlots_.insert({name, slot});
}

void GuiContextImpl::addSlot(CallSlot* slot, const char* name)
{
	callSlots_.insert({name, slot});
}

void GuiContextImpl::removeSlot(ValueSlot* slot)
{
	Map::eraseVals(valueSlots_, slot);
}

void GuiContextImpl::removeSlot(TextSlot* slot)
{
	Map::eraseVals(textSlots_, slot);
}

void GuiContextImpl::removeSlot(CallSlot* slot)
{
	Map::eraseVals(callSlots_, slot);
}

GuiContext* GuiContext::create()
{
	return new GuiContextImpl;
}

}; // namespace Vortex
