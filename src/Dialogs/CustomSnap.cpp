#include <Dialogs/CustomSnap.h>

#include <Core/Draw.h>
#include <Core/WidgetsLayout.h>

#include <Editor/View.h>

#define Dlg DialogCustomSnap

namespace Vortex {

	Dlg::~Dlg()
	{
	}

	Dlg::Dlg()
	{
		myCustomSnap = gView->getCustomSnap();

		setTitle("CUSTOM SNAP");
		myCreateWidgets();
	}

	void Dlg::myCreateWidgets()
	{
		myLayout.row().col(80).col(80);

		WgSpinner* scol = myLayout.add<WgSpinner>("Snapping");
		scol->value.bind(&myCustomSnap);
		scol->onChange.bind(this, &Dlg::onChange);
		scol->setRange(1.0, 192.0);
		scol->setPrecision(0, 0);
	}

	void Dlg::onChange()
	{
		gView->setCustomSnap(myCustomSnap);
	}
}; // namespace Vortex