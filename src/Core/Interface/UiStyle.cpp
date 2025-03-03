#include <Precomp.h>

#include <Core/Utils/Util.h>

#include <Core/Graphics/Renderer.h>
#include <Core/Graphics/Canvas.h>
#include <Core/Graphics/Image.h>
#include <Core/Graphics/Text.h>
#include <Core/Graphics/FontManager.h>

#include <Core/Interface/UiStyle.h>

namespace AV {

using namespace std;

namespace UiStyle
{
	struct State
	{
		UiStyle::Dialog dialog;
		UiStyle::Button button;
		UiStyle::Spinner spinner;
		UiStyle::Scrollbar scrollbar;
		UiStyle::TextBox textbox;
		UiStyle::Misc misc;
	};
	static State* state = nullptr;
}
using UiStyle::state;

TextStyle UiText::regular;

TextStyle UiText::small;

static unique_ptr<Texture> CreateTexture(const Image::Data& img, int x, int y, int w, int h)
{
	const uchar* pixels = img.pixels + (y * img.width + x) * 4;
	return make_unique<Texture>(pixels, w, h, img.width, PixelFormat::RGBA, TextureFormat::RGBA);
}

static TileBar CreateTileBar(const Image::Data& img, int x, int y, int w, int h, int border)
{
	TileBar out;
	out.texture = CreateTexture(img, x, y, w, h);
	out.border = border;
	return out;
}

static TileRect CreateTileRect(const Image::Data& img, int x, int y, int w, int h, int border)
{
	TileRect out;
	out.texture = CreateTexture(img, x, y, w, h);
	out.border = border;
	return out;
};

void UiStyle::create()
{
	state = new State();

	auto img = Image::load("assets/gui elements.png", PixelFormat::RGBA);
	if (!img.pixels)
		return;

	auto imgDlg = Image::load("assets/gui dialog.png", PixelFormat::RGBA);
	if (!imgDlg.pixels)
		return;

	static const char* GlyphNames[] =
	{
		"up one", "up half", "down one",       "down half",
		"halve",  "double",  "full selection", "half selection",
		"undo",   "redo",    "calculate",      "tweak",
		"search", "copy",    "play",           "arrow right",
	};

	// Create global glyph images.
	for (int y = 0; y < 4; ++y)
	{
		for (int x = 0; x < 4; ++x)
		{
			auto tex = CreateTexture(img, 64 + x * 16, y * 16, 16, 16);
			FontManager::addCustomGlyph(GlyphNames[y * 4 + x], tex, 0, 3, 16);
		}
	}

	// Button images.
	auto& button = state->button;
	button.base  = CreateTileRect(img, 0, 0, 16, 16, 6);
	button.hover = CreateTileRect(img, 0, 16, 16, 16, 6);
	button.press = CreateTileRect(img, 0, 32, 16, 16, 6);

	// Spinner images.
	auto& spinner = state->spinner;
	spinner.topBase  = spinner.bottomBase  = CreateTileRect(img, 16, 0, 16, 16, 4);
	spinner.topHover = spinner.bottomHover = CreateTileRect(img, 16, 16, 16, 16, 4);
	spinner.topPress = spinner.bottomPress = CreateTileRect(img, 16, 32, 16, 16, 4);

	spinner.topBase.uvs = spinner.topHover.uvs = spinner.topPress.uvs = Rectf(0, 0, 1, 0.5f);
	spinner.bottomBase.uvs = spinner.bottomHover.uvs = spinner.bottomPress.uvs = Rectf(0, 0.5f, 1, 1);

	// Scrollbar images.
	auto& scrollbar = state->scrollbar;
	scrollbar.base  = CreateTileRect(img, 32, 0, 16, 16, 4);
	scrollbar.hover = CreateTileRect(img, 32, 16, 16, 16, 4);
	scrollbar.press = CreateTileRect(img, 32, 24, 16, 16, 4);

	// Textbox images.
	auto& textbox = state->textbox;
	textbox.base      = CreateTileRect(img, 48, 0, 16, 16, 4);
	textbox.framed    = CreateTileRect(img, 48, 16, 16, 16, 4);
	textbox.highlight = CreateTileRect(img, 48, 24, 16, 16, 4);

	// Dialog images.
	auto& dialog = state->dialog;

	dialog.outerFrame = CreateTileRect(imgDlg, 0, 0, 16, 16, 4);
	dialog.innerFrame = CreateTileRect(imgDlg, 0, 16, 16, 16, 4);

	dialog.scrollButtonBase  = CreateTileBar(imgDlg, 16, 0, 16, 11, 4);
	dialog.scrollButtonHover = CreateTileBar(imgDlg, 32, 0, 16, 11, 4);
	dialog.scrollButtonPress = CreateTileBar(imgDlg, 48, 0, 16, 11, 4);

	dialog.iconClose    = CreateTexture(imgDlg, 16, 11, 9, 9);
	dialog.iconPin      = CreateTexture(imgDlg, 25, 11, 9, 9);
	dialog.iconUnpin    = CreateTexture(imgDlg, 34, 11, 9, 9);
	dialog.iconExpand   = CreateTexture(imgDlg, 43, 11, 9, 9);
	dialog.iconCollapse = CreateTexture(imgDlg, 52, 11, 9, 9);

	dialog.iconResize   = CreateTexture(imgDlg, 16, 20, 9, 9);
	dialog.iconArrowR   = CreateTexture(imgDlg, 25, 20, 9, 9);

	// Icons and other graphics.
	state->misc.cross = CreateTexture(img, 104, 64, 8, 8);
	state->misc.pin   = CreateTexture(img, 112, 64, 8, 8);
	state->misc.unpin = CreateTexture(img, 120, 64, 8, 8);

	state->misc.arrow = CreateTexture(img, 104, 72, 8, 8);
	state->misc.minus = CreateTexture(img, 112, 72, 8, 8);
	state->misc.plus  = CreateTexture(img, 120, 72, 8, 8);
	
	state->misc.checkmark = CreateTexture(img, 64, 64, 16, 16);

	state->misc.checkerboard = CreateTexture(img, 32, 48, 16, 16);
	// state->misc.checkerboard.source()->setWrapping(true);

	state->misc.colDisabled = Color(166, 255);

	// Free images.
	Image::release(img.pixels);
	Image::release(imgDlg.pixels);

	// Regular text style.
	auto& regular = UiText::regular;
	regular.textColor = Color::White;
	regular.shadowColor = Color(0, 0, 0, 128);

	// Small text style.
	auto& small = UiText::small;
	small.textColor = Color::White;
	small.shadowColor = Color(0, 0, 0, 128);
}

void UiStyle::destroy()
{
	UiText::regular = TextStyle();
	UiText::small = TextStyle();

	Util::reset(state);
}

UiStyle::Dialog& UiStyle::getDialog()
{
	return state->dialog;
}

UiStyle::Button& UiStyle::getButton()
{
	return state->button;
}

UiStyle::Spinner& UiStyle::getSpinner()
{
	return state->spinner;
}

UiStyle::Scrollbar& UiStyle::getScrollbar()
{
	return state->scrollbar;
}

UiStyle::TextBox& UiStyle::getTextBox()
{
	return state->textbox;
}

UiStyle::Misc& UiStyle::getMisc()
{
	return state->misc;
}

void UiStyle::button(TileRect* tr, const Rect& r, bool hover, bool focus)
{
	if (!focus && !hover)
	{
		tr[0].draw(r);
	}
	else
	{
		tr[1].draw(r, Color(255, focus ? 230 : 255));
	}
}

void UiStyle::checkerboard(Rect r, Color color)
{
	// Vertex positions.
	int vp[8];
	vp[0] = vp[4] = r.l;
	vp[1] = vp[3] = r.t;
	vp[2] = vp[6] = r.r;
	vp[5] = vp[7] = r.b;

	// Texture coordinates.
	float uv[8];
	uv[0] = uv[1] = uv[3] = uv[4] = 0;
	uv[2] = uv[6] = (float)r.w() / 16.0f;
	uv[5] = uv[7] = (float)r.h() / 16.0f;

	// Render quad.
	Renderer::bindTextureAndShader(state->misc.checkerboard.get());
	Renderer::setColor(color);
	Renderer::drawQuads(1, vp, uv);
}

} // namespace AV
