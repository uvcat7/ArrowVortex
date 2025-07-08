#include <Dialogs/CustomSnap.h>

#include <Core/Draw.h>
#include <Core/WidgetsLayout.h>

#include <Editor/View.h>

namespace Vortex {

	DialogCustomSnap::~DialogCustomSnap()
	{
	}

	DialogCustomSnap::DialogCustomSnap()
	{
		myCustomSnap = gView->getCustomSnap();

		setTitle("CUSTOM SNAP");
		myCreateWidgets();
	}

	void DialogCustomSnap::myCreateWidgets()
	{
		myLayout.row().col(80).col(80);

		WgSpinner* scol = myLayout.add<WgSpinner>("Snapping");
		scol->value.bind(&myCustomSnap);
		scol->onChange.bind(this, &DialogCustomSnap::onChange);
		scol->setRange(1.0, 192.0);
		scol->setPrecision(0, 0);
		scol->startCapturingText();
	}

	void DialogCustomSnap::onChange()
	{
		if (myCustomSnap > 0 && myCustomSnap <= 192)
		{
			gView->setCustomSnap(myCustomSnap);
			//requestClose();
		}
	}
}; // namespace Vortex