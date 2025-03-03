#pragma once

#include <Core/Graphics/Draw.h>
#include <Core/Graphics/Text.h>

namespace AV {

struct UiText
{
	static TextStyle regular;

	static TextStyle small;
};

namespace UiStyle
{
	struct Button
	{
		TileRect base;
		TileRect hover;
		TileRect press;
	};

	struct Spinner
	{
		TileRect topBase;
		TileRect topHover;
		TileRect topPress;

		TileRect bottomBase;
		TileRect bottomHover;
		TileRect bottomPress;
	};

	struct Scrollbar
	{
		TileRect base;
		TileRect hover;
		TileRect press;
	};

	struct TextBox
	{
		TileRect base;
		TileRect framed;
		TileRect highlight;
	};

	struct Dialog
	{
		TileRect outerFrame;
		TileRect innerFrame;

		TileBar scrollButtonBase;
		TileBar scrollButtonHover;
		TileBar scrollButtonPress;

		Sprite iconClose;
		Sprite iconPin;
		Sprite iconUnpin;
		Sprite iconExpand;
		Sprite iconCollapse;
		Sprite iconResize;
		Sprite iconArrowR;
	};

	struct Misc
	{
		Sprite pin;
		Sprite unpin;

		Sprite plus;
		Sprite minus;

		Sprite arrow;
		Sprite cross;
		Sprite checkmark;

		unique_ptr<Texture> checkerboard;

		Color colDisabled;
	};

	void create();
	void destroy();

	Dialog& getDialog();
	Button& getButton();
	Spinner& getSpinner();
	Scrollbar& getScrollbar();
	TextBox& getTextBox();
	Misc& getMisc();

	void button(TileRect* tr, const Rect& r, bool hover, bool focus);

	void checkerboard(Rect r, Color color);
};

} // namespace AV
