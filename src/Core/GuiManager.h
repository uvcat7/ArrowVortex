#pragma once

#include <Core/Gui.h>
#include <Core/Widgets.h>

#include <Core/Xmr.h>

namespace Vortex {

// ================================================================================================
// GuiContext Manager

struct GuiManager
{
	static void create();
	static void destroy();

	static void registerWidgetClass(StringRef name, GuiMain::CreateWidgetFunction func);
	static GuiWidget* createWidget(StringRef name, GuiContext* gui);

	static void removeWidget(GuiWidget* w);

	static void setWidgetId(GuiWidget* w, StringRef id);
	static GuiWidget* findWidget(StringRef id, GuiContext* gui);

	static void setTooltip(const GuiWidget* w, StringRef str);
	static String getTooltip(const GuiWidget* w);

	static void blockMouseOver(GuiWidget* w);
	static void unblockMouseOver(GuiWidget* w);
	static void makeMouseOver(GuiWidget* i);

	static void startCapturingMouse(GuiWidget* i);
	static void captureText(GuiWidget* i);

	static void stopCapturingMouse(GuiWidget* i);
	static void stopCapturingText(GuiWidget* i);

	static GuiWidget* getMouseOver();
	static GuiWidget* getMouseCapture();
	static GuiWidget* getTextCapture();
	
	static bool isCapturingMouse();
	static bool isCapturingText();

	static void startFrame(float dt);
};

}; // namespace Vortex
