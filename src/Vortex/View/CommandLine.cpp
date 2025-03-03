#include <Precomp.h>

#include <Core/Utils/Util.h>

#include <Vortex/Commands.h>
#include <Core/Graphics/Draw.h>
#include <Core/Graphics/Text.h>
#include <Core/Interface/UiMan.h>
#include <Core/Interface/UiStyle.h>
#include <Core/System/Window.h>

#include <Vortex/View/CommandLine.h>

namespace AV {
	
using namespace std;

struct CommandLineUi : public UiElement
{
	void onKeyPress(KeyPress& input) override;
	void onTextInput(TextInput& input) override;

	void tick(bool enabled) override;
	void draw(bool enabled) override;
};

namespace CommandLine
{
	struct State
	{
		vector<string> history;
		string text;
		bool isOpen = false;
		size_t historyIndex = 0;
	};
	static State* state = nullptr;
}
using CommandLine::state;

// =====================================================================================================================
// Event handling

void CommandLineUi::onKeyPress(KeyPress& input)
{
	if (!state->isOpen || input.handled()) return;

	if (input.key.modifiers != ModifierKeys::None) return;

	input.setHandled();

	if (input.key.code == KeyCode::Backspace)
	{
		if (!state->text.empty())
			state->text.pop_back();
	}
	else if (input.key.code == KeyCode::Return)
	{
		if (auto cmd = Commands::find(state->text, false))
			cmd->run();

		state->history.push_back(state->text);

		if (state->history.size() > 10)
			state->history.erase(state->history.begin());

		state->historyIndex = state->history.size();

		state->text.clear();
	}
	else if (input.key.code == KeyCode::Escape)
	{
		CommandLine::toggle();
	}
	else if (input.key.code == KeyCode::Up)
	{
		if (state->historyIndex > 0)
		{
			--state->historyIndex;
			if (state->historyIndex < state->history.size())
				state->text = state->history[state->historyIndex];
		}
	}
	else if (input.key.code == KeyCode::Down)
	{
		if (state->historyIndex < state->history.size())
		{
			++state->historyIndex;
			if (state->historyIndex < state->history.size())
				state->text = state->history[state->historyIndex];
			else
				state->text.clear();
		}
	}
}

void CommandLineUi::onTextInput(TextInput& input)
{
	if (!state->isOpen || input.handled()) return;

	state->text.append(input.text);
}

// =====================================================================================================================
// Initialization

void CommandLine::initialize(const XmrDoc& settings)
{
	state = new State();
	UiMan::add(make_unique<CommandLineUi>());
}

void CommandLine::deinitialize()
{
	Util::reset(state);
}

void CommandLineUi::tick(bool enabled)
{
	if (!state->isOpen) return;
}

void CommandLineUi::draw(bool enabled)
{
	if (!state->isOpen) return;

	Rect r = { 0, 0, Window::getSize().w, 24 };
	Draw::fill(r, Color(10, 10, 10, 100));
	Draw::fill(r.sliceB(1), Color(100, 100, 100));

	r = r.shrink(2);
	Text::setStyle(UiText::regular);
	Text::format(TextAlign::ML, state->text.data());
	Text::draw(r);
}

void CommandLine::toggle()
{
	state->isOpen = !state->isOpen;

	if (!state->isOpen)
		state->text.clear();
}

bool CommandLine::isOpen()
{
	return state->isOpen;
}

} // namespace AV
