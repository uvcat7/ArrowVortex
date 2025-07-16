#pragma once

#include <Core/Utils.h>
#include <Core/GuiManager.h>
#include <Core/GuiDraw.h>
#include <Core/GuiDialog.h>

#include <map>

namespace Vortex {

// ================================================================================================
// Gui context.

class GuiContextImpl : public GuiContext
{
public:
	~GuiContextImpl();
	GuiContextImpl();

	void tick(recti view, float deltaTime, InputEvents& events);
	void draw();

	recti getView();
	vec2i getMousePos();
	float getDeltaTime();
	InputEvents& getEvents();

	bool bind(StringRef slot, const int* v);
	bool bind(StringRef slot, const uint32_t* v);
	bool bind(StringRef slot, const long* v);
	bool bind(StringRef slot, const uint64_t* v);
	bool bind(StringRef slot, const float* v);
	bool bind(StringRef slot, const double* v);
	bool bind(StringRef slot, const bool* v);
	bool bind(StringRef slot, const char* str);
	bool bind(StringRef slot, const String* str);

	bool bind(StringRef slot, int* v);
	bool bind(StringRef slot, uint32_t* v);
	bool bind(StringRef slot, long* v);
	bool bind(StringRef slot, uint64_t* v);
	bool bind(StringRef slot, float* v);
	bool bind(StringRef slot, double* v);
	bool bind(StringRef slot, bool* v);
	bool bind(StringRef slot, String* str);
	bool bind(StringRef slot, Functor::Generic* f);

	void addSlot(ValueSlot* slot, const char* name);
	void addSlot(TextSlot* slot, const char* name);
	void addSlot(CallSlot* slot, const char* name);

	void removeSlot(ValueSlot* slot);
	void removeSlot(TextSlot* slot);
	void removeSlot(CallSlot* slot);

	void removeWidget(GuiWidget* w);

	void addDialog(DialogData* f);
	void removeDialog(DialogData* f);
	
	void grabFocus(GuiWidget* w);
	void releaseFocus(GuiWidget* w);

private:
	std::map<String, ValueSlot*> value_slots_;
	std::map<String, TextSlot*> text_slots_;
	std::map<String, CallSlot*> call_slots_;

	recti view_rect_;
	vec2i mouse_position_;
	float delta_time_;
	InputEvents* input_events_;

	Vector<DialogData*> dialogs_;
	Vector<GuiWidget*> focus_widgets_;
};

}; // namespace Vortex
