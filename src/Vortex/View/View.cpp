#include <Precomp.h>

#include <Vortex/View/View.h>

#include <Core/Utils/Xmr.h>
#include <Core/Common/Event.h>

#include <Core/Utils/Util.h>
#include <Core/Utils/String.h>
#include <Core/Utils/Enum.h>
#include <Core/Utils/Flag.h>

#include <Core/Graphics/Draw.h>

#include <Core/System/System.h>
#include <Core/System/Window.h>
#include <Core/System/Log.h>

#include <Core/Interface/UiElement.h>

#include <Simfile/Simfile.h>
#include <Simfile/Chart.h>
#include <Simfile/NoteSet.h>
#include <Simfile/Tempo.h>

#include <Vortex/Managers/MusicMan.h>
#include <Vortex/Managers/GameMan.h>
#include <Vortex/Noteskin/NoteskinMan.h>
#include <Vortex/Edit/TempoTweaker.h>

#include <Vortex/Common.h>
#include <Vortex/Editor.h>
#include <Vortex/Application.h>

#include <Vortex/Edit/Selection.h>
#include <Vortex/Edit/Editing.h>

#include <Vortex/Notefield/Notefield.h>
#include <Vortex/View/Minimap.h>
#include <Vortex/View/Hud.h>
#include <Vortex/Notefield/Waveform.h>

#include <Core/Graphics/Text.h>

#include <Core/Interface/UiStyle.h>
#include <Core/Interface/UiMan.h>

namespace AV {

using namespace std;
using namespace Util;

static const int RowSnapTypes[(int)SnapType::Count] =
{
	1, 48, 24, 16, 12, 10, 8, 6, 4, 3, 2, 1
};

static const double ZoomLevels[] = { 0.5, 0.75, 1.0 };

struct InterpolatingValue
{
	double value;
	double target;

	double minValue = 0.25;
	double maxValue = 2.0;
	double stepSize = 0.05;
	double updateSpeed = 0;

	bool updating = false;
	bool incrementing = false;

	InterpolatingValue(double initialValue)
	{
		value = initialValue;
		target = initialValue;
	}

	void update(double dt)
	{
		double delta = updateSpeed * dt;

		if (updating)
		{
			updateSpeed = min(2.0, updateSpeed + dt * 2);
			if (incrementing)
			{
				target += delta;
			}
			else
			{
				target -= delta;
			}
			target = clamp(target, minValue, maxValue);
		}

		if (value < target)
		{
			value = min(target, value + delta);
		}
		else if (value > target)
		{
			value = max(target, value - delta);
		}
	}

	void start(bool increment)
	{
		if (updating)
			return;

		updateSpeed = 0.5;
		incrementing = increment;
		updating = true;
	}

	void finish()
	{
		if (!updating)
			return;

		updating = false;

		if (incrementing)
			target = ceil(target / stepSize) * stepSize;
		else
			target = floor(target / stepSize) * stepSize;

		target = clamp(target, minValue, maxValue);
	}

	void set(double v)
	{
		value = target = clamp(v, minValue, maxValue);
	}
};

// =====================================================================================================================
// View :: state data.

struct ViewUi : public UiElement
{
	void onMouseScroll(MouseScroll& input) override;
	void onMousePress(MousePress& input) override;
	void onMouseRelease(MouseRelease& input) override;

	void tick(bool enabled) override;
	void draw(bool enabled) override;
};

namespace View
{
	struct State
	{
		Rect rect = {};
		double chartTopY = 0.0;
		double cursorTime = 0.0;
		double cursorRow = 0;
		Row endRow = 0;
	
		int receptorX = 0;
		int receptorY = 192;
	
		int currentZoomLevel = 0;
	
		double noteScale = 1.0;
		InterpolatingValue noteSpacing = InterpolatingValue(1.0);
	
		double pixPerSec = 0;
		double pixPerRow = 0;
	
		bool isDraggingReceptors = false;
	
		SnapType snapType = SnapType::None;
	
		EventSubscriptions subscriptions;
	};
	static State* state = nullptr;
}
using View::state;

// =====================================================================================================================
// View :: handle events.

static void UpdateZoomValues()
{
	state->pixPerSec = state->noteScale * state->noteSpacing.value * 256.0;
	state->pixPerRow = state->pixPerSec * BeatsPerRow;

	if (Settings::view().reverseScroll)
	{
		state->pixPerSec = -state->pixPerSec;
		state->pixPerRow = -state->pixPerRow;
	}
}

void ViewUi::onMouseScroll(MouseScroll& input)
{
	if (input.handled()) return;

	if (TempoTweaker::getTweakMode() != TweakMode::None && (input.modifiers & (ModifierKeys::Ctrl | ModifierKeys::Alt))) return;

	if ((int)state->snapType > 0)
	{
		int irow = (int)lround(state->cursorRow);
		View::setCursorRow(View::snapRow(irow, input.isUp ? SnapDir::Up : SnapDir::Down));
	}
	else
	{
		double delta = input.isUp ? -128.0 : 128.0;
		if (Settings::view().isTimeBased)
		{
			View::setCursorTime(state->cursorTime + delta / state->pixPerSec);
		}
		else
		{
			View::setCursorRow(state->cursorRow + delta / state->pixPerRow);
		}
	}
	input.setHandled();
}

static bool IsMouseOverReceptors(Vec2 pos)
{
	if (!UiMan::isCapturingMouse())
	{
		auto c = View::getReceptorCoords();
		auto chart = Editor::currentChart();
		float dy = (chart ? 32.f : 8.f) * (float)state->noteScale;
		float x = (float)pos.x;
		float y = (float)pos.y;
		return (x >= c.xl && x < c.xr&& abs(y - state->receptorY) <= dy);
	}
	return false;
}

void ViewUi::onMousePress(MousePress& input)
{
	// Dragging the receptors.
	if (input.button == MouseButton::LMB && IsMouseOverReceptors(input.pos) && input.unhandled())
	{
		state->isDraggingReceptors = true;
		input.setHandled();
	}
}

void ViewUi::onMouseRelease(MouseRelease& input)
{
	// Finish receptor dragging.
	if (input.button == MouseButton::LMB && state->isDraggingReceptors)
	{
		state->isDraggingReceptors = false;
	}
}

void ViewUi::tick(bool enabled)
{
	Size2 windowSize = Window::getSize();
	state->rect = Rect(0, 0, windowSize.w, windowSize.h);

	// Handle zooming/spacing.
	auto dt = Window::getDeltaTime();
	auto targetNoteScale = ZoomLevels[state->currentZoomLevel];
	if (state->noteScale != targetNoteScale)
	{
		auto t = dt * 5.0;
		auto step = (targetNoteScale - state->noteScale) * (1.0 - pow(0.5, t));
		if (state->noteScale < targetNoteScale)
			state->noteScale = min(targetNoteScale, state->noteScale + step + t);
		else
			state->noteScale = max(targetNoteScale, state->noteScale + step - t);
	}

	state->noteSpacing.update(dt);
	UpdateZoomValues();

	// Handle receptor dragging.
	if (state->isDraggingReceptors)
	{
		state->receptorX = Window::getMousePos().x - state->rect.cx();
		state->receptorY = Window::getMousePos().y - state->rect.t;
	}

	// Set cursor to arrows when hovering over/dragging the receptors.
	Vec2 mpos = Window::getMousePos();
	if (state->isDraggingReceptors || IsMouseOverReceptors(mpos))
	{
		Window::setCursor(Window::CursorIcon::SizeAll);
	}

	// Update the cursor startTime.
	if (!MusicMan::isPaused())
	{
		auto sim = Editor::currentSimfile();
		auto tempo = Editor::currentTempo();
		if (tempo)
		{
			auto& timing = tempo->timing;
			Row endRow = state->endRow;
			double begintime = timing.rowToTime(0);
			double endtime = timing.rowToTime(endRow);
			if (state->cursorTime > endtime)
			{
				state->cursorTime = endtime;
				state->cursorRow = endRow;
				MusicMan::pause();
			}
			else
			{
				double time = MusicMan::getPlayTime();
				state->cursorTime = clamp(time, begintime, endtime);
				state->cursorRow = timing.timeToRow(state->cursorTime);
			}
		}
	}

	// Clamp the receptor X and Y to the view region.
	state->receptorY = clamp(state->receptorY, state->rect.t, state->rect.b);
	state->receptorX = clamp(state->receptorX, state->rect.l, state->rect.r);

	// Store the y-position of startTime zero.
	if (Settings::view().isTimeBased)
	{
		state->chartTopY = floor((double)state->receptorY - state->cursorTime * state->pixPerSec);
	}
	else
	{
		state->chartTopY = floor((double)state->receptorY - state->cursorRow * state->pixPerRow);
	}
}

static void HandleSettingChanged(const Setting::Changed& arg)
{
	if (arg == Settings::view().reverseScroll)
		state->receptorY = state->rect.h() - state->receptorY;
	UpdateZoomValues();
}

static void HandleTimingChanged()
{
	View::updateEndRow();
	auto tempo = Editor::currentTempo();
	if (tempo)
		state->cursorTime = tempo->timing.rowToTime(state->cursorRow);
}

static void HandleEndRowChanged()
{
	View::updateEndRow();
}

// =====================================================================================================================
// View :: Initialize.

void View::initialize(const XmrDoc& settings)
{
	state = new State();
	UiMan::add(make_unique<ViewUi>());

	if (AV::Settings::view().reverseScroll)
	{
		state->receptorY = state->rect.h() - state->receptorY;
	}

	UpdateZoomValues();

	state->subscriptions.add<Setting::Changed>(HandleSettingChanged);
	state->subscriptions.add<Editor::ActiveTempoChanged>(HandleTimingChanged);
	state->subscriptions.add<Editor::ActiveChartChanged>(HandleEndRowChanged);
	state->subscriptions.add<Tempo::TimingChanged>(HandleTimingChanged);
	state->subscriptions.add<Tempo::SegmentsChanged>(HandleEndRowChanged);
	state->subscriptions.add<Chart::NotesChangedEvent>(HandleEndRowChanged);
	state->subscriptions.add<MusicMan::MusicLengthChanged>(HandleEndRowChanged);
}

void View::deinitialize()
{
	Util::reset(state);
}

// =====================================================================================================================
// View :: member functions.

double View::getZoomLevel()
{
	return state->noteScale;
}

double View::getNoteSpacing()
{
	return state->noteSpacing.value;
}

SnapType View::getSnapType()
{
	return state->snapType;
}

double View::getPixPerSec()
{
	return state->pixPerSec;
}

double View::getPixPerRow()
{
	return state->pixPerRow;
}

double View::getPixPerOfs()
{
	return AV::Settings::view().isTimeBased ? state->pixPerSec : state->pixPerRow;
}

int View::getCursorRowI()
{
	return (int)lround(state->cursorRow);
}

double View::getCursorRow()
{
	return state->cursorRow;
}

double View::getCursorTime()
{
	return state->cursorTime;
}

double View::getCursorOffset()
{
	return AV::Settings::view().isTimeBased ? state->cursorTime : state->cursorRow;
}

void ViewUi::draw(bool enabled)
{
	// Text::setStyle(UiText::regular);
	// Text::format(TextAlign::TL, String::format("zoom.value: %1").arg(state->noteScale, 3));
	// Text::draw(10, 10);
	// Text::format(TextAlign::TL, String::format("zoom.target: %1").arg(state->zoomLevel.target, 3));
	// Text::draw(10, 20);
}

void View::startZooming(bool zoomIn)
{
	if (zoomIn)
	{
		if (state->currentZoomLevel + 1 < size(ZoomLevels))
			state->currentZoomLevel++;
	}
	else
	{
		if (state->currentZoomLevel > 0)
			state->currentZoomLevel--;
	}
}

void View::finishZooming()
{
}

void View::setZoomLevel(double level)
{
}

void View::startAdjustingNoteSpacing(bool increaseSpacing)
{
	state->noteSpacing.start(increaseSpacing);
}

void View::finishAdjustingNoteSpacing()
{
	state->noteSpacing.finish();
}

void View::setNoteSpacing(double level)
{
	state->noteSpacing.set(level);
}

void View::setCursorTime(double time)
{
	auto sim = Editor::currentSimfile();
	auto tempo = Editor::currentTempo();

	if (!tempo) return;

	auto& timing = tempo->timing;
	double begintime = timing.rowToTime(0);
	double endtime = timing.rowToTime(state->endRow);
	state->cursorTime = clamp(time, begintime, endtime);
	state->cursorRow = timing.timeToRow(state->cursorTime);
	MusicMan::seek(state->cursorTime);
}

void View::setCursorRow(double row)
{
	auto sim = Editor::currentSimfile();
	auto tempo = Editor::currentTempo();

	if (!tempo) return;

	state->cursorRow = min(max(row, 0.0), (double)state->endRow);
	state->cursorTime = tempo->timing.rowToTime(state->cursorRow);
	MusicMan::seek(state->cursorTime);
}

void View::setSnapType(SnapType type)
{
	state->snapType = type;
}

void View::setCursorOffset(ViewOffset ofs)
{
	if (AV::Settings::view().isTimeBased)
	{
		setCursorTime(ofs);
	}
	else
	{
		setCursorRow(int(ofs + 0.5));
	}
}

void View::jumpToCursorRow(Row row)
{
	auto sim = Editor::currentSimfile();
	row = clamp(row, 0, state->endRow);
	int y = (int)rowToY(row);
	if (y < 16 || y > state->rect.h() - 16)
	{
		View::setCursorRow(row);
	}
}

static float GetRowY(Row row)
{
	if (Settings::view().isTimeBased)
	{
		auto tempo = Editor::currentTempo();
		if (tempo)
		{
			double time = tempo->timing.rowToTime(row);
			return float(state->chartTopY + time * state->pixPerSec);
		}
		else
		{
			return (float)state->chartTopY;
		}
	}
	else
	{
		return float(state->chartTopY + (double)row * state->pixPerRow);
	}
}

static float GetTimeY(double time)
{
	if (Settings::view().isTimeBased)
	{
		return float(state->chartTopY + time * state->pixPerSec);
	}
	else
	{
		auto tempo = Editor::currentTempo();
		if (tempo)
		{
			return float(state->chartTopY + tempo->timing.timeToRow(time) * state->pixPerRow);
		}
		else
		{
			return (float)state->chartTopY;
		}
	}
}

View::Coords View::getReceptorCoords()
{
	Coords out;
	out.y = float(state->rect.t + state->receptorY);
	out.xc = float(state->rect.cx() + state->receptorX);
	auto chart = Editor::currentChart();
	if (chart)
	{
		auto noteskin = NoteskinMan::getNoteskin(chart->gameMode);
		float margin = noteskin->width * float(0.5 * state->noteScale);
		out.xl = out.xc - margin;
		out.xr = out.xc + margin;
	}
	else
	{
		float w = 128.f * (float)state->noteScale;
		out.xl = out.xc - (float)w;
		out.xr = out.xc + (float)w;
	}
	return out;
}

View::Coords View::getNotefieldCoords()
{
	Coords out = getReceptorCoords();
	auto& settings = AV::Settings::view();
	if (settings.isTimeBased && settings.showWaveform)
	{
		float w = Waveform::getWidth() * 0.5f;
		out.xl = min(out.xl, out.xc - w - 4);
		out.xr = max(out.xr, out.xc + w + 4);
	}
	return out;
}

float View::getZoomScale()
{
	return (float)state->noteScale;
}

bool View::isZoomedIn()
{
	return state->noteScale > 0.5;
}

float View::columnToX(int col)
{
	float cx = (float)state->rect.cx() + state->receptorX;
	auto chart = Editor::currentChart();
	if (!chart) return cx;
	auto noteskin = NoteskinMan::getNoteskin(chart->gameMode);
	float x = (col < noteskin->gameMode->numCols) ? noteskin->columns[col].x : 0;
	return cx + x * (float)state->noteScale;
}

float View::rowToY(Row row)
{
	if (AV::Settings::view().isTimeBased)
	{
		auto tempo = Editor::currentTempo();
		if (tempo)
		{
			return float(state->chartTopY + tempo->timing.rowToTime(row) * state->pixPerSec);
		}
		else
		{
			return (float)state->chartTopY;
		}
	}
	else
	{
		return float(state->chartTopY + (double)row * state->pixPerRow);
	}
}

float View::timeToY(double time)
{
	if (AV::Settings::view().isTimeBased)
	{
		return float(state->chartTopY + time * state->pixPerSec);
	}
	else
	{
		auto tempo = Editor::currentTempo();
		if (tempo)
		{
			return float(state->chartTopY + tempo->timing.timeToRow(time) * state->pixPerRow);
		}
		else
		{
			return (float)state->chartTopY;
		}
	}
}

float View::offsetToY(ViewOffset ofs)
{
	if (AV::Settings::view().isTimeBased)
	{
		return float(state->chartTopY + ofs * state->pixPerSec);
	}
	else
	{
		return float(state->chartTopY + ofs * state->pixPerRow);
	}
}

ViewOffset View::yToOffset(int viewY)
{
	return ((ViewOffset)viewY - state->chartTopY) / getPixPerOfs();
}

ViewOffset View::rowToOffset(Row row)
{
	if (AV::Settings::view().isTimeBased)
	{
		auto tempo = Editor::currentTempo();
		return tempo ? tempo->timing.rowToTime(row) : 0.0;
	}
	else
	{
		return (ViewOffset)row;
	}
}

ViewOffset View::timeToOffset(double time)
{
	if (AV::Settings::view().isTimeBased)
	{
		return (ViewOffset)time;
	}
	else
	{
		auto tempo = Editor::currentTempo();
		return tempo ? tempo->timing.timeToRow(time) : 0.0;
	}
}

double View::offsetToRow(ViewOffset ofs)
{
	if (AV::Settings::view().isTimeBased)
	{
		auto tempo = Editor::currentTempo();
		return tempo ? tempo->timing.timeToRow(ofs) : 0;
	}
	else
	{
		return ofs;
	}
}

double View::offsetToTime(ViewOffset ofs)
{
	if (AV::Settings::view().isTimeBased)
	{
		return ofs;
	}
	else
	{
		auto tempo = Editor::currentTempo();
		return tempo ? tempo->timing.rowToTime(ofs) : 0.0;
	}
}

pair<double, double> View::getVisibleRows(int pixelMarginTop, int pixelMarginBottom)
{
	double topRow = max(0.0, View::offsetToRow(View::yToOffset(-8)));
	double bottomRow = View::offsetToRow(View::yToOffset(View::getHeight() + 8));
	return { topRow, bottomRow };
}

int View::getWidth()
{
	return state->rect.w();
}

int View::getHeight()
{
	return state->rect.h();
}

int View::getEndRow()
{
	return state->endRow;
}

bool View::updateEndRow()
{
	Row endRow = 0;

	auto chart = Editor::currentChart();
	if (chart)
	{
		endRow = max(endRow, chart->notes.endRow());
	}

	auto tempo = Editor::currentTempo();
	if (tempo)
	{
		// TODO: fix
		/*
		endRow = max(endRow, tempo->endRow());
		*/
		auto songLen = MusicMan::getSongLength();
		endRow = max(endRow, (int)lround(tempo->timing.timeToRow(songLen)));
	}

	if (state->endRow == endRow)
		return false;

	state->endRow = endRow;
	EventSystem::publish<EndRowChanged>();

	return true;
}

const Rect& View::rect()
{
	return state->rect;
}

Row View::snapRow(Row row, SnapDir dir)
{
	if (dir == SnapDir::Closest)
	{
		if (isAlignedToSnap(row))
		{
			return row;
		}
		else
		{
			int uprow = snapRow(row, SnapDir::Up);
			int downrow = snapRow(row, SnapDir::Down);
			int isUp = abs(row - uprow), down = abs(row - downrow);
			return (isUp < down) ? uprow : downrow;
		}
	}
	else
	{
		// Up and down are switched when reverse scroll is enabled.
		if (AV::Settings::view().reverseScroll)
		{
			switch (dir)
			{
			case SnapDir::Up: dir = SnapDir::Down; break;
			case SnapDir::Down: dir = SnapDir::Up; break;
			};
		}

		// Bump the row by one so the snap will jump to the next position if the given row
		// is equal to the snap row.
		int snap = RowSnapTypes[(int)state->snapType];
		row = row + ((dir == SnapDir::Up) ? -1 : 1);

		// Special case, ITG does not really support 20th so we fake it.
		if (snap == 10)
		{
			int beat = row / 48, beatrow = (int)lround(row) % 48;
			Row rows[6] = { 0, 10, 19, 29, 38, 48 };
			if (dir == SnapDir::Up)
			{
				for (int i = 5; i >= 0; --i)
				{
					if (rows[i] < beatrow)
					{
						beatrow = rows[i]; break;
					}
				}
			}
			else
			{
				for (int i = 0; i < 6; ++i)
				{
					if (rows[i] > beatrow)
					{
						beatrow = rows[i]; break;
					}
				}

			}
			row = beat * 48 + beatrow;
		}
		else // Regular case, snap is divisible by 192.
		{
			if (row % snap && dir != SnapDir::Up) row += snap;
			row -= row % snap;
		}

		return row;
	}
}

Row View::snapToNextMeasure(bool isUp)
{
	if (AV::Settings::view().reverseScroll) isUp = !isUp;

	Row rowsPerMeasure = 192;

	if (isUp)
	{
		int delta = lround(state->cursorRow) % rowsPerMeasure;
		if (delta == 0 || (delta < rowsPerMeasure / 2 && !MusicMan::isPaused()))
		{
			delta += rowsPerMeasure;
		}
		return (Row)(state->cursorRow - delta);
	}
	else
	{
		int delta = rowsPerMeasure - lround(state->cursorRow) % rowsPerMeasure;
		return (Row)(state->cursorRow + delta);
	}
}

Row View::snapToStream(bool top)
{
	auto chart = Editor::currentChart();
	if (!chart) return 0;

	// Reduce all notes to a set of rows.
	std::set<int> noteRows;
	for (auto& col : chart->notes)
	{
		for (auto& note : col)
		{
			if (note.type != (uint)NoteType::Mine)
			{
				noteRows.insert(note.row);
			}
		}
	}
	if (noteRows.size() == 0) return 0;

	if (AV::Settings::view().reverseScroll) top = !top;

	auto first = noteRows.begin(), n = first;
	auto last = noteRows.end(); --last;

	if (top)
	{
		// Advance to the first note on/after the cursor row.
		while (n != last && *n < state->cursorRow) ++n;
		if (n != first)
		{
			// Go back to the first note before it with different interval.
			auto prev = n; --prev;
			int interval = *n - *prev;
			while (n != first && (*n - *prev) == interval) --n, --prev;
		}
	}
	else
	{
		// Advance to the last note before/on the cursor row.
		auto next = n; ++next;
		while (n != last && *next <= state->cursorRow) ++n, ++next;
		if (n != last)
		{
			// Advance to the first note after it with a different interval.
			int interval = *n - *next;
			while (n != last && (*n - *next) == interval) ++n, ++next;
		}
	}

	return *n;
}

Row View::snapToSelection(bool top)
{
	if (AV::Settings::view().reverseScroll) top = !top;

	auto region = Selection::getRegion();
	if (region.beginRow != region.endRow)
	{
		return top ? region.beginRow : region.endRow;
	}
	else if (Editor::currentChart())
	{
		NoteSet notes(Editor::currentChart()->gameMode->numCols);

		int firstRow = INT_MAX;
		int lastRow = 0;
		for (auto& col : notes)
		{
			if (col.size())
			{
				firstRow = min(firstRow, col.begin()->row);
				lastRow = max(lastRow, (col.end() - 1)->row);
			}
		}

		if (firstRow != INT_MAX)
			return top ? firstRow : lastRow;
	}

	return 0;
}

bool View::isAlignedToSnap(Row row)
{
	int snap = RowSnapTypes[(int)state->snapType];

	// Special case, ITG/DDR does not really support 20ths so we fake it.
	if (snap == 10)
	{
		Row rows[5] = { 0, 10, 19, 29, 38 };
		return std::find(rows, rows + 5, row % 48) != rows + 5;
	}

	return (row % snap == 0);
}

} // namespace AV
