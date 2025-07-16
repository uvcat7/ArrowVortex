#pragma once

#include <Dialogs/Dialog.h>
#include <Core/Widgets.h>
#include <Core/WidgetsLayout.h>

namespace Vortex {

class DialogNewChart : public EditorDialog
{
public:
	~DialogNewChart();
	DialogNewChart();

private:
	void myCreateWidgets();
	void myCreateChart();

	WgDroplist* myStyleList;
	String myStepArtist;
	int myRating, myDifficulty, myStyle;
};

}; // namespace Vortex
