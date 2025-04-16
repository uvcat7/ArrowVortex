#include <Dialogs/Zoom.h>

#include <Core/Widgets.h>
#include <Core/WidgetsLayout.h>

#include <Editor/View.h>

#define Dlg DialogZoom

namespace Vortex {

enum Actions
{
	ACT_ZOOM,
	ACT_SPACING,
};

Dlg::~Dlg()
{
}

Dlg::Dlg()
{
	setTitle("ZOOM");
	myCreateWidgets();
}

void Dlg::myCreateWidgets()
{
	myLayout.row().col(64).col(80);

	WgSlider* zoom = myLayout.add<WgSlider>("Zoom");
	zoom->value.bind(&myZoomLevel);
	zoom->onChange.bind(this, &Dlg::onAction, (int)ACT_ZOOM);
	zoom->setRange(1.0, 4.0);
	zoom->setTooltip("Zoom Level / Note Size");

	WgSlider* spacing = myLayout.add<WgSlider>("Spacing");
	spacing->value.bind(&mySpacingLevel);
	spacing->onChange.bind(this, &Dlg::onAction, (int)ACT_SPACING);
	spacing->setRange(1.0, 16.0);
	spacing->setTooltip("Note Spacing");
}

void Dlg::onTick()
{
	myZoomLevel = gView->getZoomLevel();
	mySpacingLevel = gView->getSpacingLevel();
	EditorDialog::onTick();
}

void Dlg::onAction(int id)
{
	switch (id)
	{
	case ACT_ZOOM: {
		gView->setZoomLevel(myZoomLevel);
	} break;
	case ACT_SPACING: {
		gView->setSpacingLevel(mySpacingLevel);
	} break;
	}
}
}; // namespace Vortex
