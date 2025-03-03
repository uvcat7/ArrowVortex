#include <Precomp.h>

#include <Core/Utils/Util.h>
#include <Core/Utils/String.h>

#include <Core/Graphics/Draw.h>
#include <Core/Graphics/Text.h>
#include <Core/Graphics/Canvas.h>

#include <Core/Interface/UiStyle.h>

#include <Simfile/Simfile.h>

#include <Vortex/Common.h>
#include <Vortex/Editor.h>

#include <Vortex/Managers/GameMan.h>

#include <Vortex/View/Dialogs/ChartList.h>

#define Dlg DialogChartList

namespace AV {

using namespace std;

// =====================================================================================================================
// DialogChartList

Dlg::~Dlg()
{
}

Dlg::Dlg()
{
	setTitle("LIST OF CHARTS");

	setResizeBehaviorH(ResizeBehavior::Unbound);
	setResizeBehaviorV(ResizeBehavior::Bound);

	myContent = myTabs = make_shared<WChartTabs>();

	mySubscriptions.add<Simfile::ChartsChanged>(this, &Dlg::myHandleChartsChanged);
}

void Dlg::myHandleChartsChanged()
{
	myTabs->updateList();
}

} // namespace AV
