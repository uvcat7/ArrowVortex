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
	Vec::release(myDialogs);
}

GuiContextImpl::GuiContextImpl()
{
	myView = {0, 0, INT_MAX, INT_MAX};
	myEvents = nullptr;
}

recti GuiContextImpl::getView()
{
	return myView;
}

vec2i GuiContextImpl::getMousePos()
{
	return myMousePos;
}

float GuiContextImpl::getDeltaTime()
{
	return myDeltaTime;
}

InputEvents& GuiContextImpl::getEvents()
{
	return *myEvents;
}

void GuiContextImpl::tick(recti view, float deltaTime, InputEvents& events)
{
	myView = view;

	myView.w = max(myView.w, 0);
	myView.h = max(myView.h, 0);

	myDeltaTime = deltaTime;
	myEvents = &events;

	// Update the mouse position.
	for(MouseMove* move = nullptr; events.next(move);)
	{
		myMousePos = {move->x, move->y};
	}

	// Rearrange or delete dialogs if requested.
	FOR_VECTOR_REVERSE(myDialogs, i)
	{
		auto dialog = myDialogs[i];
		if(dialog->myRequestClose)
		{
			myDialogs.erase_values(dialog);
			delete dialog;
		}
		else if(dialog->myRequestMoveToTop)
		{
			myDialogs.erase(i);
			myDialogs.push_back(dialog);
			dialog->myRequestMoveToTop = 0;
		}
	}

	// Arrange the dialogs and widgets.
	FOR_VECTOR_REVERSE(myDialogs, i)
	{
		myDialogs[i]->arrange();
	}

	// Tick widgets with focus first.
	FOR_VECTOR_REVERSE(myFocusWidgets, i)
	{
		if(myFocusWidgets[i]->isEnabled())
		{
			myFocusWidgets[i]->onTick();
		}
	}

	// Tick the dialogs.
	FOR_VECTOR_REVERSE(myDialogs, i)
	{
		myDialogs[i]->tick();
	}

	myEvents = nullptr;
}

void GuiContextImpl::draw()
{
	// Draw the dialogs.
	FOR_VECTOR_FORWARD(myDialogs, i)
	{
		myDialogs[i]->draw();
	}

	// Draw widgets with focus at the top.
	FOR_VECTOR_FORWARD(myFocusWidgets, i)
	{
		myFocusWidgets[i]->onDraw();
	}
}

void GuiContextImpl::removeWidget(GuiWidget* w)
{
	myFocusWidgets.erase_values(w);
}

void GuiContextImpl::addDialog(DialogData* f)
{
	myDialogs.push_back(f);
}

void GuiContextImpl::removeDialog(DialogData* f)
{
	myDialogs.erase_values(f);
}

void GuiContextImpl::grabFocus(GuiWidget* w)
{
	FOR_VECTOR_FORWARD(myFocusWidgets, i)
	{
		if(myFocusWidgets[i] == w) return;
	}
	myFocusWidgets.push_back(w);
}

void GuiContextImpl::releaseFocus(GuiWidget* w)
{
	myFocusWidgets.erase_values(w);
}

// ================================================================================================
// GuiContextImpl :: slot bindings.

bool GuiContextImpl::bind(StringRef slot, const int* v)
{
	auto s = Map::findVal(myValueSlots, slot);
	if(s) (*s)->bind(v);
	return s != nullptr;
}

bool GuiContextImpl::bind(StringRef slot, const uint* v)
{
	auto s = Map::findVal(myValueSlots, slot);
	if(s) (*s)->bind(v);
	return s != nullptr;
}

bool GuiContextImpl::bind(StringRef slot, const long* v)
{
	auto s = Map::findVal(myValueSlots, slot);
	if(s) (*s)->bind(v);
	return s != nullptr;
}

bool GuiContextImpl::bind(StringRef slot, const ulong* v)
{
	auto s = Map::findVal(myValueSlots, slot);
	if(s) (*s)->bind(v);
	return s != nullptr;
}

bool GuiContextImpl::bind(StringRef slot, const float* v)
{
	auto s = Map::findVal(myValueSlots, slot);
	if(s) (*s)->bind(v);
	return s != nullptr;
}

bool GuiContextImpl::bind(StringRef slot, const double* v)
{
	auto s = Map::findVal(myValueSlots, slot);
	if(s) (*s)->bind(v);
	return s != nullptr;
}

bool GuiContextImpl::bind(StringRef slot, const bool* v)
{
	auto s = Map::findVal(myValueSlots, slot);
	if(s) (*s)->bind(v);
	return s != nullptr;
}

bool GuiContextImpl::bind(StringRef slot, const char* str)
{
	auto s = Map::findVal(myTextSlots, slot);
	if(s) (*s)->bind(str);
	return s != nullptr;
}

bool GuiContextImpl::bind(StringRef slot, const String* str)
{
	auto s = Map::findVal(myTextSlots, slot);
	if(s) (*s)->bind(str);
	return s != nullptr;
}

bool GuiContextImpl::bind(StringRef slot, int* v)
{
	auto s = Map::findVal(myValueSlots, slot);
	if(s) (*s)->bind(v);
	return s != nullptr;
}

bool GuiContextImpl::bind(StringRef slot, uint* v)
{
	auto s = Map::findVal(myValueSlots, slot);
	if(s) (*s)->bind(v);
	return s != nullptr;
}

bool GuiContextImpl::bind(StringRef slot, long* v)
{
	auto s = Map::findVal(myValueSlots, slot);
	if(s) (*s)->bind(v);
	return s != nullptr;
}

bool GuiContextImpl::bind(StringRef slot, ulong* v)
{
	auto s = Map::findVal(myValueSlots, slot);
	if(s) (*s)->bind(v);
	return s != nullptr;
}

bool GuiContextImpl::bind(StringRef slot, float* v)
{
	auto s = Map::findVal(myValueSlots, slot);
	if(s) (*s)->bind(v);
	return s != nullptr;
}

bool GuiContextImpl::bind(StringRef slot, double* v)
{
	auto s = Map::findVal(myValueSlots, slot);
	if(s) (*s)->bind(v);
	return s != nullptr;
}

bool GuiContextImpl::bind(StringRef slot, bool* v)
{
	auto s = Map::findVal(myValueSlots, slot);
	if(s) (*s)->bind(v);
	return s != nullptr;
}

bool GuiContextImpl::bind(StringRef slot, String* str)
{
	auto s = Map::findVal(myTextSlots, slot);
	if(s) (*s)->bind(str);
	return s != nullptr;
}

bool GuiContextImpl::bind(StringRef slot, Functor::Generic* f)
{
	auto s = Map::findVal(myCallSlots, slot);
	if(s) (*s)->bind(f); else delete f;
	return s != nullptr;
}

void GuiContextImpl::addSlot(ValueSlot* slot, const char* name)
{
	myValueSlots.insert({name, slot});
}

void GuiContextImpl::addSlot(TextSlot* slot, const char* name)
{
	myTextSlots.insert({name, slot});
}

void GuiContextImpl::addSlot(CallSlot* slot, const char* name)
{
	myCallSlots.insert({name, slot});
}

void GuiContextImpl::removeSlot(ValueSlot* slot)
{
	Map::eraseVals(myValueSlots, slot);
}

void GuiContextImpl::removeSlot(TextSlot* slot)
{
	Map::eraseVals(myTextSlots, slot);
}

void GuiContextImpl::removeSlot(CallSlot* slot)
{
	Map::eraseVals(myCallSlots, slot);
}

GuiContext* GuiContext::create()
{
	return new GuiContextImpl;
}

}; // namespace Vortex
