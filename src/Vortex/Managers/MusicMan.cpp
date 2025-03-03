#include <Precomp.h>

#include <Core/Utils/Xmr.h>

#include <Core/Utils/Util.h>
#include <Core/Utils/String.h>

#include <Core/System/Log.h>
#include <Core/System/File.h>
#include <Core/System/Debug.h>
#include <Core/System/Thread.h>
#include <Core/System/System.h>
#include <Core/System/AudioOut.h>

#include <Simfile/Simfile.h>
#include <Simfile/Tempo/TimingData.h>
#include <Simfile/Chart.h>
#include <Simfile/GameMode.h>
#include <Simfile/NoteSet.h>

#include <Vortex/Editor.h>
#include <Vortex/Common.h>
#include <Vortex/Settings.h>
#include <Vortex/Application.h>

#include <Vortex/Edit/TempoTweaker.h>
#include <Vortex/Managers/MusicMan.h>

#include <Audio/ConvertToOgg.h>

#include <Vortex/View/Hud.h>
#include <Vortex/View/View.h>

namespace AV {

using namespace std;
using namespace Util;

// =====================================================================================================================
// Static data.

struct TickData
{
	Sound sound;
	vector<int64_t> frames;
	bool enabled;
};

static constexpr int NumMixChannels = 2;

enum class LoadState
{
	Streaming,
	Preallocated,
	Done,
};

struct MusicManMixer : public AudioOut::Mixer
{
	void mix(short* buffer, int frames) override;
};

namespace MusicMan
{
	struct State
	{
		Sound samples;
		double playTimer = 0.0;
		TickData beatTick = {};
		TickData noteTick = {};
		string path;
		int musicVolume = 100;
		int musicSpeed = 100;
		int tickOffsetMs = 0;
		double playPosition = 0.0;
		double playStartTime = 0.0;
		bool isPaused = true;
		bool isMuted = false;
		bool musicLoadingEnabled = true;
		LoadState loadState = LoadState::Done;
	
		MusicManMixer source;
	
		InfoBoxWithProgress* infoBox = nullptr;
	
		vector<short> mixBuffer;
	
		OggConversionThread* oggConversionThread = nullptr;
	
		EventSubscriptions subscriptions;
	};
	static State* state = nullptr;
}
using MusicMan::state;

// =====================================================================================================================
// Stream.

static void InterruptStream()
{
	if (!state->isPaused)
	{
		state->playStartTime = MusicMan::getPlayTime();
		AudioOut::pause();
	}
}

static void ResumeStream()
{
	if (!state->isPaused)
	{
		state->playPosition = state->playStartTime * (double)state->samples.getFrequency();
		state->playTimer = System::getElapsedTime();
		AudioOut::resume();
	}
}

static void UpdateBeatTicks()
{
	state->beatTick.frames.clear();

	auto sim = Editor::currentSimfile();
	auto tempo = Editor::currentTempo();

	if (tempo)
	{
		double freq = (double)state->samples.getFrequency();
		double ofs = state->tickOffsetMs / 1000.0;

		auto tracker = tempo->timing.timeTracker();
		for (Row row = 0, end = View::getEndRow(); row < end; row += int(RowsPerBeat))
		{
			double time = tracker.advance(row);
			auto frame = (int64_t)((time + ofs) * freq);
			state->beatTick.frames.push_back(frame);
		}
	}
}

static void UpdateNoteTicks()
{
	state->noteTick.frames.clear();

	auto chart = Editor::currentChart();
	auto tempo = Editor::currentTempo();
	if (!chart) return;

	int numCols = chart->gameMode->numCols;

	std::set<int> tickRows;
	for (int col = 0; col < numCols; ++col)
	{
		for (auto& note : chart->notes[col])
		{
			if (note.type != (uint)NoteType::Mine && note.type != (uint)NoteType::Fake && !note.isWarped)
			{
				tickRows.insert(note.row);
			}
		}
	}

	if (chart)
	{
		double freq = (double)state->samples.getFrequency();
		double ofs = state->tickOffsetMs / 1000.0;
		auto tracker = tempo->timing.timeTracker();
		for (auto row : tickRows)
		{
			int64_t frame = (int64_t)((tracker.advance(row) + ofs) * freq);
			state->noteTick.frames.push_back(frame);
		}
	}
}

static void FinishOggConversion()
{
	if (state->oggConversionThread)
	{
		if (state->oggConversionThread->error.empty())
		{
			auto sim = Editor::currentSimfile();
			if (sim)
				sim->music.set(Sound::find(sim->directory));
		}
		else
		{
			Hud::error("Conversion failed: %s.", state->oggConversionThread->error.data());
		}
		Util::reset(state->oggConversionThread);
	}
}

// =====================================================================================================================
// Event handlers.

static void HandleTimingChanged()
{
	InterruptStream();
	UpdateNoteTicks();
	UpdateBeatTicks();
	ResumeStream();
}

static void HandleNotesChanged()
{
	InterruptStream();
	UpdateNoteTicks();
	ResumeStream();
}

static void HandleEndRowChanged()
{
	InterruptStream();
	UpdateBeatTicks();
	ResumeStream();
}

void MusicMan::tick()
{
	if (state->loadState != LoadState::Done && state->infoBox)
	{
		if (state->samples.getLoadingProgress() > 0)
		{
			state->infoBox->setProgress(state->samples.getLoadingProgress() * 0.01f);
		}
		else
		{
			state->infoBox->setTime(state->samples.getLoadingTime());
		}
	}

	if (state->loadState == LoadState::Streaming)
	{
		if (state->samples.isAllocated())
		{
			state->loadState = LoadState::Preallocated;
			EventSystem::publish<MusicMan::MusicLengthChanged>();
		}
	}
	else if (state->loadState == LoadState::Preallocated)
	{
		if (state->samples.isCompleted())
		{
			delete state->infoBox;
			state->infoBox = nullptr;
			state->loadState = LoadState::Done;
			EventSystem::publish<MusicMan::MusicSamplesChanged>();
		}
	}

	if (state->oggConversionThread)
	{
		if (state->infoBox)
		{
			state->infoBox->setProgress(state->oggConversionThread->progress * 0.01f);
		}
		if (state->oggConversionThread->isDone())
		{
			Util::reset(state->infoBox);
			FinishOggConversion();
		}
	}
}

static void HandleSettingsChanged()
{
	// TODO
}

static void OnSetVolume()
{
	auto& settings = AV::Settings::audio();
	InterruptStream();
	state->musicVolume = settings.musicVolume;
	ResumeStream();
	Hud::note("Volume: %i%%", state->musicVolume);
}

static void OnSetSpeed()
{
	auto& settings = AV::Settings::audio();
	InterruptStream();
	state->musicSpeed = settings.musicSpeed;
	ResumeStream();
	Hud::note("Speed: %i%%", state->musicSpeed);
}

static void OnSetTickOffset()
{
	auto& settings = AV::Settings::audio();
	InterruptStream();
	state->tickOffsetMs = settings.tickOffsetMs;
	ResumeStream();
}

static void OnSetMuted()
{
	auto& settings = AV::Settings::audio();
	InterruptStream();
	state->isMuted = settings.muteMusic;
	ResumeStream();
	Hud::note("Audio: %s", state->isMuted ? "muted" : "unmuted");
}

static void OnSetBeatTicks()
{
	auto& settings = AV::Settings::audio();
	InterruptStream();
	state->beatTick.enabled = settings.beatTicks;
	ResumeStream();
	Hud::note("Beat tick: %s", state->beatTick.enabled ? "on" : "off");
}

static void OnSetNoteTicks()
{
	auto& settings = AV::Settings::audio();
	InterruptStream();
	state->noteTick.enabled = settings.noteTicks;
	ResumeStream();
	Hud::note("Note tick: %s", state->noteTick.enabled ? "on" : "off");
}

// =====================================================================================================================
// Initialization.

void MusicMan::initialize(const XmrDoc& doc)
{
	state = new State();

	auto beatTickPath = "assets/sound beat tick.wav";
	auto resultA = state->beatTick.sound.load(beatTickPath, false);
	if (resultA != Sound::LoadResult::Success)
		Log::error("Failed to load beat tick.");

	auto noteTickPath = "assets/sound note tick.wav";
	auto resultB = state->noteTick.sound.load(noteTickPath, false);
	if (resultB != Sound::LoadResult::Success)
		Log::error("Failed to load note tick.");

	state->subscriptions.add<Tempo::TimingChanged>(HandleTimingChanged);
	state->subscriptions.add<Chart::NotesChangedEvent>(HandleNotesChanged);
	state->subscriptions.add<Editor::ActiveChartChanged>(HandleNotesChanged);
	state->subscriptions.add<View::EndRowChanged>(HandleEndRowChanged);

	// TODO: fix event subscriptions

	auto& settings = AV::Settings::audio();

	state->musicVolume = settings.musicVolume;
	// state->subscriptions.add(settings.musicVolume, OnSetVolume);

	state->musicSpeed = settings.musicSpeed;
	// state->subscriptions.add(settings.musicSpeed, OnSetSpeed);

	state->tickOffsetMs = settings.tickOffsetMs;
	// state->subscriptions.add(settings.tickOffsetMs, OnSetTickOffset);

	state->isMuted = settings.muteMusic;
	// state->subscriptions.add(settings.muteMusic, OnSetMuted);

	state->beatTick.enabled = settings.beatTicks;
	// state->subscriptions.add(settings.beatTicks, OnSetBeatTicks);

	state->noteTick.enabled = settings.noteTicks;
	// state->subscriptions.add(settings.noteTicks, OnSetNoteTicks);
}

void MusicMan::deinitialize()
{
	MusicMan::unload();

	delete state->infoBox;
	Util::reset(state);
}

// =====================================================================================================================
// Loading and unloading music.

static void TerminateOggConversion()
{
	if (state->oggConversionThread)
	{
		state->oggConversionThread->terminate();
		Util::reset(state->oggConversionThread);
	}
}

void MusicMan::unload()
{
	TerminateOggConversion();

	AudioOut::close();

	state->samples.clear();

	state->loadState = LoadState::Done;
}

void MusicMan::setMusicLoadingEnabled(bool enabled)
{
	state->musicLoadingEnabled = enabled;
}

Sound::LoadResult MusicMan::load(Simfile* sim)
{
	unload();

	if (!state->musicLoadingEnabled) return Sound::LoadResult::Success;

	// Construct the path of the music file.
	string filename = sim->music.get();
	if (filename.empty())
	{
		Hud::error("Could not load music, the music property is blank.");
		return Sound::LoadResult::Failure;
	}
	auto path = FilePath(sim->directory, filename);

	// Start loading the audio from file.
	bool multithread = AV::Settings::general().multithread;
	auto result = state->samples.load(path, multithread);

	if (result == Sound::LoadResult::Success && state->samples.getFrequency() > 0)
	{
		state->loadState = LoadState::Streaming;

		AudioOut::open(&state->source, state->samples.getFrequency());

		if (!state->infoBox) state->infoBox = new InfoBoxWithProgress;
		state->infoBox->left = "Loading music...";
	}
	else
	{
		state->samples.clear();
		Hud::error("Invalid audio: %s", filename.data());
	}

	return result;
}

// =====================================================================================================================
// Mixing functions

void WriteTickSamples(short* dst, size_t startFrame, size_t numFrames, const TickData& tick, int rate)
{
	if (rate != 100)
		startFrame = startFrame * 100 / rate;

	const short* srcL = tick.sound.samplesL() + startFrame;
	const short* srcR = tick.sound.samplesR() + startFrame;

	if (rate == 100)
	{
		auto n = min(numFrames, tick.sound.getNumFrames() - startFrame);
		for (size_t i = 0; i < n; ++i)
		{
			*dst++ = min(max(*dst + *srcL++, SHRT_MIN), SHRT_MAX);
			*dst++ = min(max(*dst + *srcR++, SHRT_MIN), SHRT_MAX);
		}
	}
	else
	{
		size_t idx = 0;
		double srcPos = 0.0;
		const size_t tickEndPos = tick.sound.getNumFrames() - startFrame;
		const double srcDelta = 100.0 / (double)rate;
		for (; numFrames > 0 && idx < tickEndPos; --numFrames)
		{
			const float frac = float(srcPos - floor(srcPos));

			float sampleL = lerp((float)srcL[idx], (float)srcL[idx + 1], frac);
			float sampleR = lerp((float)srcR[idx], (float)srcR[idx + 1], frac);
			
			*dst++ = min(max(*dst + (short)sampleL, SHRT_MIN), SHRT_MAX);
			*dst++ = min(max(*dst + (short)sampleR, SHRT_MIN), SHRT_MAX);

			srcPos += srcDelta;
			idx = (int)srcPos;
		}
	}
}

void WriteTicks(short* buf, size_t frames, const TickData& tick, int rate)
{
	auto playPos = (int64_t)state->playPosition;
	auto count = tick.frames.size();
	auto ticks = tick.frames.data();

	// Jump forward to the first audible tick.
	size_t first = 0;
	int64_t firstAudibleTickPos = playPos - tick.sound.getNumFrames();
	while (first < count && ticks[first] < firstAudibleTickPos) ++first;

	// Write all ticks that intersect the current buffer.
	int64_t curFrame = -1;
	for (size_t i = first; i < count; ++i)
	{
		auto beginFrame = tick.frames[i] - playPos;
		if (beginFrame == curFrame) continue; // avoid double ticks for jumps.
		if (beginFrame > (int64_t)frames) break;

		auto srcPos = max(0ll, -beginFrame);
		auto dstPos = max(0ll, beginFrame);
		auto dst = buf + dstPos * 2;

		WriteTickSamples(dst, srcPos, frames - dstPos, tick, rate);

		curFrame = beginFrame;
	}
}

void WriteSourceFrames(short* buffer, size_t frames, int64_t srcPos)
{
	short* dst = buffer;
	int musicVolume = state->musicVolume;

	// If the stream pos is before the start of the song, start with silence.
	auto framesLeft = frames;
	if (srcPos < 0)
	{
		auto n = min(framesLeft, (size_t)-srcPos);
		memset(dst, 0, sizeof(short) * NumMixChannels * n);
		dst += n * NumMixChannels;
		framesLeft -= n;
		srcPos = 0;
	}

	// Fill the remaining buffer with music samples.
	if (framesLeft > 0 && state->samples.isAllocated() && musicVolume > 0 && !state->isMuted)
	{
		auto totalFrames = state->samples.getNumFrames();
		auto n = ((int64_t)totalFrames > srcPos) ? totalFrames - srcPos : 0;
		n = min(n, framesLeft);

		const short* srcL = state->samples.samplesL() + srcPos;
		const short* srcR = state->samples.samplesR() + srcPos;
		if (musicVolume == 100)
		{
			for (int i = 0; i < n; ++i)
			{
				*dst++ = *srcL++;
				*dst++ = *srcR++;
			}
		}
		else
		{
			int vol = ((musicVolume * musicVolume) << 15) / (100 * 100);
			for (int i = 0; i < n; ++i)
			{
				*dst++ = (short)(((*srcL++) * vol) >> 15);
				*dst++ = (short)(((*srcR++) * vol) >> 15);
			}
		}
		framesLeft -= n;
	}

	// If there are still frames left, end with silence.
	if (framesLeft > 0)
	{
		memset(dst, 0, sizeof(short) * NumMixChannels * framesLeft);
	}

	// Write beat and step ticks.
	int rate = state->musicSpeed;
	if (state->beatTick.enabled)
	{
		WriteTicks(buffer, frames, state->beatTick, rate);
	}
	if (state->noteTick.enabled)
	{
		WriteTicks(buffer, frames, state->noteTick, rate);
	}
}

void MusicManMixer::mix(short* buffer, int frames)
{
	double srcAdvance = (double)frames;
	if (state->musicSpeed == 100)
	{
		// Source and target samplerate are equal.
		int64_t srcPos = llround(state->playPosition);
		WriteSourceFrames(buffer, frames, srcPos);
	}
	else
	{
		double rate = (double)state->musicSpeed / 100.0;
		srcAdvance *= rate;

		// Source and target samplerate are different, mix to temporary buffer.
		int64_t srcPos = (int64_t)(state->playPosition);
		int tmpFrames = frames * state->musicSpeed / 100;
		int tmpSamples = tmpFrames * 2;
		if ((int)state->mixBuffer.size() < tmpSamples)
		{
			state->mixBuffer.resize(tmpSamples);
		}
		WriteSourceFrames(state->mixBuffer.data(), tmpFrames, srcPos);

		// Interpolate to the target samplerate.
		double tmpPos = 0.0;
		const short* tmpL = state->mixBuffer.data() + 0;
		const short* tmpR = state->mixBuffer.data() + 1;
		int tmpEnd = tmpFrames - 1;

		short* dst = buffer;
		for (int i = 0; i < frames; ++i)
		{
			int index0 = min((int)tmpPos, tmpEnd);
			int index1 = min(index0 + 1, tmpEnd);
			index0 *= NumMixChannels;
			index1 *= NumMixChannels;

			float w1 = float(tmpPos - floor(tmpPos));
			float w0 = 1.0f - w1;

			float l = (float)tmpL[index0] * w0 + (float)tmpL[index1] * w1;
			float r = (float)tmpR[index0] * w0 + (float)tmpR[index1] * w1;

			*dst++ = (short)min(max((int)l, SHRT_MIN), SHRT_MAX);
			*dst++ = (short)min(max((int)r, SHRT_MIN), SHRT_MAX);

			tmpPos += rate;
		}
	}

	state->playPosition += srcAdvance;
}

// =====================================================================================================================
// OggVorbis conversion.

void MusicMan::startOggConversion()
{
	auto sim = Editor::currentSimfile();
	if (!sim) return;

	auto path = FilePath(sim->directory, sim->music.get());
	if (path.extension() == "ogg")
	{
		Hud::note("Music is already in Ogg Vorbis format.");
	}
	else if (!state->samples.isCompleted())
	{
		Hud::note("Wait for the music to finish loading.");
	}
	else if (state->samples.getNumFrames() == 0)
	{
		Hud::note("There is no music loaded.");
	}
	else if (state->oggConversionThread)
	{
		Hud::note("Conversion is currently in progress.");
	}
	else
	{
		auto oggPath = FilePath(sim->directory, path.stem(), "ogg");

		state->oggConversionThread = new OggConversionThread;
		state->oggConversionThread->outPath = oggPath;

		if (AV::Settings::general().multithread)
		{
			if (!state->infoBox) state->infoBox = new InfoBoxWithProgress;
			state->infoBox->left = "Converting music to Ogg Vorbis...";
			state->oggConversionThread->run();
		}
		else
		{
			state->oggConversionThread->exec();
			FinishOggConversion();
		}
	}
}

/*void trimBeats(int numKeep, int numTrim)
{
	static bool done = false;
	if (done) return;
	done = true;
	HudInfo("Trimming beats");

	if (Editor::isClosed()) return;

	pause();

	int curRow = 0, endRow = Editor::getEndRow();
	int samplerate = state->samples.getFrequency();

	TimingData timing = Editor::currentTempo()->timing;
	auto tracker = timing.timeTracker();
	vector<Sound::Stretch> stretches;

	while (curRow < endRow)
	{
		curRow += numKeep * RowsPerBeat;
		int a = int(tracker.advance(curRow) * samplerate);
		if (curRow >= endRow) break;

		curRow += numTrim * RowsPerBeat;
		int b = int(tracker.advance(curRow) * samplerate);
		if (curRow >= endRow) break;

		stretches.push_back({a, b - a, 2});
	}

	state->samples.stretch(stretches.data(), stretches.size());
}
*/

// =====================================================================================================================
// General API.

void MusicMan::pause()
{
	if (!state->isPaused)
	{
		InterruptStream();
		state->isPaused = true;
	}
}

void MusicMan::play()
{
	if (state->isPaused)
	{
		state->isPaused = false;
		ResumeStream();
	}
}

void MusicMan::seek(double seconds)
{
	InterruptStream();
	state->playStartTime = seconds;
	ResumeStream();
}

double MusicMan::getPlayTime()
{
	double time = state->playStartTime;
	if (!state->isPaused)
	{
		double rate = (double)state->musicSpeed * 0.01;
		time += (System::getElapsedTime() - state->playTimer) * rate;
	}
	return time;
}

double MusicMan::getSongLength()
{
	return (double)state->samples.getNumFrames() / (double)state->samples.getFrequency();
}

bool MusicMan::isPaused()
{
	return state->isPaused;
}

const Sound& MusicMan::getSamples()
{
	return state->samples;
}

} // namespace AV
