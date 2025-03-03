#include <Precomp.h>

#include <Core/Utils/Util.h>

#include <Core/Graphics/Draw.h>
#include <Core/Graphics/Text.h>

#include <Core/Interface/UiStyle.h>

#include <Simfile/Simfile.h>
#include <Simfile/Tempo.h>

#include <Vortex/Editor.h>
#include <Vortex/Settings.h>

#include <Vortex/View/View.h>
#include <Vortex/Notefield/BeatRegions.h>
#include <Vortex/Notefield/NotefieldPosTracker.h>

namespace AV {

using namespace std;
using namespace Util;

void BeatRegions::drawStopAndWarpRegions(float x, float w)
{
	auto tempo = Editor::currentTempo();
	if (!tempo) return;

	Renderer::resetColor();
	Renderer::bindShader(Renderer::Shader::Color);

	float viewTop = (float)View::rect().t;
	float viewBtm = (float)View::rect().b;

	auto& timingEvents = tempo->timing.events();
	Renderer::startBatch(Renderer::FillType::ColoredQuads, Renderer::VertexType::Float);

	if (Settings::view().isTimeBased)
	{
		double zeroTimeY = View::offsetToY(0.0);
		double pixelsPerSecond = View::getPixPerSec();

		// In startTime-based mode stops and delays will be visible, but warps won't be.
		for (auto& it : timingEvents)
		{
			if (it.hitTime > it.startTime)
			{
				float t = float(zeroTimeY + pixelsPerSecond * it.startTime);
				float b = float(zeroTimeY + pixelsPerSecond * it.hitTime);
				if (t < viewBtm && b > viewTop)
				{
					Color col = Color(26, 128, 128, 128);
					Renderer::pushQuad(x, t, x + w, b, col);
				}
			}
			if (it.endTime > it.hitTime)
			{
				float t = float(zeroTimeY + pixelsPerSecond * it.hitTime);
				float b = float(zeroTimeY + pixelsPerSecond * it.endTime);
				if (t < viewBtm && b > viewTop)
				{
					Color col = Color(128, 128, 51, 128);
					Renderer::pushQuad(x, t, x + w, b, col);
				}
			}
		}
	}
	else
	{
		double zeroRowY = View::offsetToY(0.0);
		double pixelsPerRow = View::getPixPerRow();

		// In row-based mode warps will be visible, but stops and delays won't be.
		// Note: this loop only iterates up to one before end, because it also uses the next event. 
		for (auto it = timingEvents.begin(), last = timingEvents.end() - 1; it < last; ++it)
		{
			if (it->spr == 0.0)
			{
				float t = float(zeroRowY + pixelsPerRow * it->row);
				float b = float(zeroRowY + pixelsPerRow * (it + 1)->row);
				if (t < viewBtm && b > viewTop)
				{
					Color col = Color(128, 26, 51, 128);
					Renderer::pushQuad(x, t, x + w, b, col);
				}
			}
		}
	}
	Renderer::flushBatch();
}

void BeatRegions::drawSongPreview(float x, float w)
{
	auto sim = Editor::currentSimfile();
	if (!sim) return;

	auto& preview = sim->musicPreview.get();
	if (preview.length <= 0) return;

	float yt = max(0.f, View::timeToY(preview.start));
	float yb = min((float)View::getHeight(), View::timeToY(preview.start + preview.length));
	if (yb < yt) return;

	Draw::fill(Rectf(x, yt, x + w, yb), Color(255, 255, 255, 64));
	if (View::isZoomedIn())
	{
		Text::setStyle(UiText::regular);
		Text::format(TextAlign::TL, "SONG PREVIEW");
		Text::draw((int)x + 2, (int)yt + 2);
	}
}

} // namespace AV
