#include <Precomp.h>

#include <Core/Utils/Util.h>

#include <Core/System/System.h>
#include <Core/System/Window.h>

#include <Core/Graphics/Draw.h>

#include <Vortex/View/EditorDialog.h>

#include <Vortex/Managers/DialogMan.h>

namespace AV {

using namespace std;
using namespace Util;

static const char* IdStrings[(int)DialogId::Count] =
{
	"adjustSync",
	"adjustTempo",
	"chartList",
	"chartProperties",
	"dancingBot",
	"generateNotes",
	"newChart",
	"songProperties",
	"tempoBreakdown",
	"waveformSettings",
	"keysounds",
};

EditorDialog::~EditorDialog()
{
}

EditorDialog::EditorDialog()
{
}

void EditorDialog::onUpdateSize()
{
	if (myContent)
	{
		myContent->measure();

		Size2 minSize(myContent->getMinWidth(), myContent->getMinHeight());
		setMinimumSize(minSize.w, minSize.h);

		Size2 prefSize(myContent->getWidth(), myContent->getHeight());
		setPreferredSize(prefSize.w, prefSize.h);
	}
}

void EditorDialog::handleEvent(UiEvent& uiEvent)
{
	if (myContent)
		myContent->handleEvent(uiEvent);

	Dialog::handleEvent(uiEvent);
}

const char* EditorDialog::getName(DialogId id)
{
	return IdStrings[(int)id];
}

void EditorDialog::tickContent(bool enabled)
{
	if (myContent)
	{
		myContent->arrange(getContentRect());
		myContent->tick(enabled);
	}
}

void EditorDialog::drawContent(bool enabled)
{
	if (myContent)
		myContent->draw(enabled);
}

UiElement* EditorDialog::findContentMouseOver(int x, int y)
{
	return myContent->findMouseOver(x, y);
}

} // namespace AV
