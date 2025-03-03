#include <Precomp.h>

#include <Core/Utils/Util.h>
#include <Core/Utils/String.h>

#include <Core/Graphics/Draw.h>
#include <Core/Graphics/Text.h>
#include <Core/Graphics/Canvas.h>

#include <Core/Interface/UiStyle.h>

#include <Simfile/Simfile.h>
#include <Simfile/Tempo.h>

#include <Vortex/Common.h>
#include <Vortex/Editor.h>

#include <Vortex/Managers/GameMan.h>

#include <Vortex/View/Dialogs/TempoBreakdown.h>

#define Dlg DialogTempoBreakdown

namespace AV {

using namespace std;

// =====================================================================================================================
// DialogTempoBreakdown

Dlg::~Dlg()
{
}

Dlg::Dlg()
{
	setTitle("TEMPO BREAKDOWN");

	setResizeBehaviorH(ResizeBehavior::Unbound);
	setResizeBehaviorV(ResizeBehavior::Bound);

	myContent = myTabs = make_shared<WTempoTabs>();

	mySubscriptions.add<Tempo::SegmentsChanged>(this, &Dlg::myHandleSegmentsChanged);
}

void Dlg::myHandleSegmentsChanged()
{
	myTabs->updateList();
}

} // namespace AV
