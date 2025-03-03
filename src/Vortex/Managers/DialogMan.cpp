#include <Precomp.h>

#include <Core/Utils/Util.h>
#include <Core/Utils/Xmr.h>

#include <Core/System/System.h>
#include <Core/System/Window.h>
#include <Core/System/Log.h>

#include <Core/Utils/String.h>
#include <Core/Utils/Vector.h>

#include <Core/Interface/UiMan.h>

#include <Vortex/Managers/DialogMan.h>

#include <Vortex/View/Dialogs/SongProperties.h>
#include <Vortex/View/Dialogs/ChartList.h>
#include <Vortex/View/Dialogs/ChartProperties.h>
#include <Vortex/View/Dialogs/NewChart.h>
#include <Vortex/View/Dialogs/AdjustTempo.h>
#include <Vortex/View/Dialogs/AdjustSync.h>
#include <Vortex/View/Dialogs/DancingBot.h>
#include <Vortex/View/Dialogs/TempoBreakdown.h>
#include <Vortex/View/Dialogs/GenerateNotes.h>
#include <Vortex/View/Dialogs/WaveformSettings.h>
#include <Vortex/View/Dialogs/Keysounds.h>

#include <Vortex/Commands.h>

namespace AV {

using namespace std;

struct DialogData
{
	weak_ptr<EditorDialog> dialog;
	bool requested;
};

// =====================================================================================================================
// Singleton.

struct DialogManUi : public UiElement
{
	UiElement* findMouseOver(int x, int y) override;
	void handleEvent(UiEvent& uiEvent) override;
	void tick(bool enabled) override;
	void draw(bool enabled) override;
};

namespace DialogMan
{
	struct State
	{
		DialogData dialogData[(int)DialogId::Count] = {};
		vector<pair<DialogId, shared_ptr<Dialog>>> dialogs;
	};
	static State* state = nullptr;
}
using DialogMan::state;

static void HandleDialogOpening(DialogId id, Rect rect)
{
	auto& target = state->dialogData[(int)id];
	target.requested = false;
	if (!target.dialog.expired()) return;

	shared_ptr<EditorDialog> dialog;
	switch (id)
	{
	case DialogId::AdjustSync:
		dialog = make_shared<DialogAdjustSync>(); break;
	case DialogId::AdjustTempo:
		dialog = make_shared<DialogAdjustTempo>(); break;
	case DialogId::ChartList:
		dialog = make_shared<DialogChartList>(); break;
	case DialogId::ChartProperties:
		dialog = make_shared<DialogChartProperties>(); break;
	case DialogId::DancingBot:
		dialog = make_shared<DialogDancingBot>(); break;
	case DialogId::GenerateNotes:
		dialog = make_shared<DialogGenerateNotes>(); break;
	case DialogId::NewChart:
		dialog = make_shared<DialogNewChart>(); break;
	case DialogId::SongProperties:
		dialog = make_shared<DialogSongProperties>(); break;
	case DialogId::TempoBreakdown:
		dialog = make_shared<DialogTempoBreakdown>(); break;
	case DialogId::WaveformSettings:
		dialog = make_shared<DialogWaveformSettings>(); break;
	case DialogId::Keysounds:
		dialog = make_shared<DialogKeysounds>(); break;
	};

	if (rect.w() > 0 && rect.h() > 0)
	{
		dialog->setPosition(rect.l, rect.t);
		dialog->setWidth(rect.w());
		dialog->setHeight(rect.h());
		dialog->requestPin();
	}
	else
	{
		dialog->onUpdateSize();
		Size2 windowSize = Window::getSize();
		Size2 dialogSize = dialog->getPreferredSize();
		int x = windowSize.w / 2 - dialogSize.w / 2;
		int y = windowSize.h / 2 - dialogSize.h / 2;
		dialog->setPosition(x, y);
	}

	state->dialogs.emplace_back(id, dialog);
	target.dialog = dialog;
}

// =====================================================================================================================
// Initialization.

void DialogMan::initialize(const XmrDoc& settings)
{
	state = new State();
	UiMan::add(make_unique<DialogManUi>());

	// Read stored dialog data.
	auto dialogs = settings.child("dialogs");
	if (dialogs)
	{
		for (auto dialog : dialogs->children())
		{
			int r[4];
			auto a = dialog->child("rect");
			if (a == nullptr || !String::readInts(a->value, r))
				continue;

			Rect rect = { r[0], r[1], r[2], r[3] };
			for (int id = 0; id < (int)DialogId::Count; ++id)
			{
				auto name = EditorDialog::getName((DialogId)id);
				if (dialog->name == name)
					HandleDialogOpening((DialogId)id, rect);
			}
		}
	}
}

void DialogMan::deinitialize()
{
	// for (auto& dialog : state->dialogs)
	// {
	// 	auto& dlg = dialog.second;
	// 	if (dlg && dlg->isPinned())
	// 	{
	// 		auto r = dlg->getContentRect();
	// 		if (r.w() > 0 && r.h() > 0)
	// 		{
	// 			auto name = EditorDialog::getName(dialog.first);
	// 			dialogSettings.entries.push_back({ name, r });
	// 		}
	// 	}
	// 
	// 	auto dialogs = doc.getOrAddChild("dialogs");
	// 	for (auto& entry : entries)
	// 	{
	// 		XmrNode* node = dialogs->addChild(entry.name.data());
	// 		node->addChild("rect",
	// 			String::format("%1, %2, %3, %4")
	// 			.arg(entry.rect.l)
	// 			.arg(entry.rect.t)
	// 			.arg(entry.rect.r)
	// 			.arg(entry.rect.b));
	// 	}
	// }

	state->dialogs.clear();
	Util::reset(state);
}

// =====================================================================================================================
// Open and close dialog.

void DialogMan::requestOpen(DialogId id)
{
	if (state->dialogData[(int)id].dialog.expired())
		state->dialogData[(int)id].requested = true;
}

UiElement* DialogManUi::findMouseOver(int x, int y)
{
	for (auto& dialog : state->dialogs)
	{
		if (auto mouseOver = dialog.second->findMouseOver(x, y))
			return mouseOver;
	}
	return nullptr;
}

void DialogManUi::handleEvent(UiEvent& uiEvent)
{
	for (auto& dialog : state->dialogs)
		dialog.second->handleEvent(uiEvent);
}

void DialogManUi::tick(bool enabled)
{
	for (int i = int(state->dialogs.size()) - 1; i >= 0; --i)
	{
		auto frame = state->dialogs[i].second->frame();
		if (frame->requestMoveToTop)
		{
			auto data = state->dialogs[i];
			state->dialogs.erase(state->dialogs.begin() + i);
			state->dialogs.emplace(state->dialogs.begin(), data);
			frame->requestMoveToTop = 0;
		}
	}

	for (int id = 0; id < int(DialogId::Count); ++id)
	{
		if (state->dialogData[id].requested)
			HandleDialogOpening(DialogId(id), {});
	}

	for (auto it = state->dialogs.begin(); it != state->dialogs.end();)
	{
		if (it->second->hasCloseRequest())
		{
			it = state->dialogs.erase(it);
		}
		else
		{
			it->second->arrange();
			it->second->tick(enabled);
			++it;
		}
	}
}

void DialogManUi::draw(bool enabled)
{
	for (auto& dialog : state->dialogs)
		dialog.second->draw(enabled);
}

bool DialogMan::dialogIsOpen(DialogId id)
{
	return !state->dialogData[(int)id].dialog.expired();
}

} // namespace AV
