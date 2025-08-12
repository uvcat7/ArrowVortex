#include <Editor/Music.h>

#include <limits.h>
#include <stdint.h>
#include <math.h>
#include <chrono>

#include <Core/Vector.h>
#include <Core/Reference.h>
#include <Core/Utils.h>
#include <Core/StringUtils.h>
#include <Core/Xmr.h>

#include <Managers/MetadataMan.h>
#include <Managers/SimfileMan.h>
#include <Managers/TempoMan.h>
#include <Managers/NoteMan.h>

#include <Simfile/TimingData.h>

#include <Editor/ConvertToOgg.h>
#include <Editor/Editor.h>
#include <Editor/Common.h>
#include <Editor/TextOverlay.h>

#include <System/File.h>
#include <System/Debug.h>
#include <System/Thread.h>
#include <System/System.h>
#include <System/Mixer.h>

namespace Vortex {

struct TickData
{
	Sound sound;
	Vector<int> frames;
	bool enabled;
};

static const int MIX_CHANNELS = 2;

enum LoadState { LOADING_ALLOCATING_AND_READING, LOADING_ALLOCATED_AND_READING, LOADING_DONE };

// ================================================================================================
// MusicImpl :: member data.

struct MusicImpl : public Music, public MixSource {

Mixer* myMixer;
Sound mySamples;
std::chrono::steady_clock::time_point myPlayTimer;
TickData myBeatTick, myNoteTick;
std::string myTitle, myArtist;
int myMusicSpeed;
int myMusicVolume;
int myTickOffsetMs;
double myPlayPosition;
double myPlayStartTime;
bool myIsPaused, myIsMuted;
LoadState myLoadState;
Reference<InfoBoxWithProgress> myInfoBox;

Vector<short> myMixBuffer;

OggConversionThread* myOggConversionThread;

// ================================================================================================
// MusicImpl :: constructor and destructor.

~MusicImpl()
{
	unload();
	delete myMixer;
}

MusicImpl()
{
	myMixer = Mixer::create();

	myMusicSpeed = 100;
	myMusicVolume = 100;
	myTickOffsetMs = 0;
	myPlayPosition = 0.0;
	myPlayStartTime = 0.0;
	myIsPaused = true;
	myIsMuted = false;
	myLoadState = LOADING_DONE;

	myBeatTick.enabled = false;
	myNoteTick.enabled = false;

	myOggConversionThread = nullptr;

	bool success;

	std::string placeholderTitle;
	std::string placeholderArtist;

	success = myBeatTick.sound.load("assets/sound beat tick.wav", false, placeholderTitle, placeholderArtist);
	if(!success) HudError("%s", "Failed to load beat tick.\n");

	success = myNoteTick.sound.load("assets/sound note tick.wav", false, placeholderTitle, placeholderArtist);
	if(!success) HudError("%s", "Failed to load note tick.\n");
}

// ================================================================================================
// MusicImpl :: loading and saving settings.

void loadSettings(XmrNode& settings)
{
	XmrNode* audio = settings.child("audio");
	if(audio)
	{
		audio->get("musicVolume", &myMusicVolume);
		audio->get("tickOffsetMs", &myTickOffsetMs);
	}
}

void saveSettings(XmrNode& settings)
{
	XmrNode* audio = settings.addChild("audio");

	audio->addAttrib("musicVolume", (long)myMusicVolume);
	audio->addAttrib("tickOffsetMs", (long)myTickOffsetMs);
}

// ================================================================================================
// MusicImpl :: loading and unloading music.

void unload()
{
	terminateOggConversion();

	myMixer->close();

	mySamples.clear();
	myTitle.clear();

	myLoadState = LOADING_DONE;
}

void load()
{
	unload();

	if(gSimfile->isClosed()) return;

	std::string dir = gSimfile->getDir();
	std::string file = gSimfile->get()->music;

	if(file.empty())
	{
		HudError("Could not load music, the music property is blank.");
		return;
	}

	Path path(dir, file);
	bool success = mySamples.load(
		path.str.c_str(), gEditor->hasMultithreading(), myTitle, myArtist);

	if(success && mySamples.getFrequency() > 0)
	{
		myLoadState = LOADING_ALLOCATING_AND_READING;

		myMixer->open(this, mySamples.getFrequency());

		auto box = myInfoBox.create();
		box->left = "Loading music...";
	}
	else
	{
		mySamples.clear();
		HudError("Could not load \"%s\".", path.filename().c_str());
	}
}

// ================================================================================================
// MusicImpl :: mixing functions

void WriteTickSamples(short* dst, int startFrame, int numFrames, const TickData& tick, int rate)
{
	if(rate != 100)
	{
		startFrame = (int)((int64_t)startFrame * 100 / rate);
	}

	const short* srcL = tick.sound.samplesL() + startFrame;
	const short* srcR = tick.sound.samplesR() + startFrame;

	if(rate == 100)
	{
		int n = min(numFrames, tick.sound.getNumFrames() - startFrame);
		for(int i = 0; i < n; ++i)
		{
			*dst++ = min(max(*dst + *srcL++, SHRT_MIN), SHRT_MAX);
			*dst++ = min(max(*dst + *srcR++, SHRT_MIN), SHRT_MAX);
		}
	}
	else
	{
		int idx = 0;
		double srcPos = 0.0;
		const int tickEndPos = tick.sound.getNumFrames() - startFrame;
		const double srcDelta = 100.0 / (double)rate;
		for(; numFrames > 0 && idx < tickEndPos; --numFrames)
		{
			const float frac = (float)(srcPos - floor(srcPos));

			float sampleL = lerp((float)srcL[idx], (float)srcL[idx + 1], frac);
			float sampleR = lerp((float)srcR[idx], (float)srcR[idx + 1], frac);
			
			*dst++ = min(max(*dst + (short)sampleL, SHRT_MIN), SHRT_MAX);
			*dst++ = min(max(*dst + (short)sampleR, SHRT_MIN), SHRT_MAX);

			srcPos += srcDelta;
			idx = (int)srcPos;
		}
	}
}

void WriteTicks(short* buf, int frames, const TickData& tick, int rate)
{
	int playPos = (int)myPlayPosition;
	int count = tick.frames.size();
	const int* ticks = tick.frames.data();

	// Jump forward to the first audible tick.
	int first = 0, firstAudibleTickPos = playPos - tick.sound.getNumFrames();
	while(first < count && ticks[first] < firstAudibleTickPos) ++first;

	// Write all ticks that intersect the current buffer.
	int curFrame = -1;
	for(int i = first; i < count; ++i)
	{
		int beginFrame = tick.frames[i] - playPos;
		if(beginFrame == curFrame) continue; // avoid double ticks for jumps.
		if(beginFrame > frames) break;

		int srcPos = max(0, -beginFrame);
		int dstPos = max(0, beginFrame);
		short* dst = buf + dstPos * 2;
		int dstFrames = frames - dstPos;

		WriteTickSamples(dst, srcPos, frames - dstPos, tick, rate);

		curFrame = beginFrame;
	}
}

void WriteSourceFrames(short* buffer, int frames, int64_t srcPos)
{
	short* dst = buffer;
	int musicVolume = gMusic->getVolume();

	// If the stream pos is before the start of the song, start with silence.
	int framesLeft = frames;
	if(srcPos < 0)
	{
		int n = min(framesLeft, (int)max(-srcPos, (int64_t)INT_MIN));
		memset(dst, 0, sizeof(short) * MIX_CHANNELS * n);
		dst += n * MIX_CHANNELS;
		framesLeft -= n;
		srcPos = 0;
	}

	// Fill the remaining buffer with music samples.
	if(framesLeft > 0 && mySamples.isAllocated() && musicVolume > 0 && !myIsMuted)
	{
		int n = (int)min(max(mySamples.getNumFrames() - srcPos, (int64_t)0), (int64_t)framesLeft);
		const short* srcL = mySamples.samplesL() + srcPos;
		const short* srcR = mySamples.samplesR() + srcPos;
		if(musicVolume == 100)
		{
			for(int i = 0; i < n; ++i)
			{
				*dst++ = *srcL++;
				*dst++ = *srcR++;
			}
		}
		else
		{
			int vol = ((musicVolume * musicVolume) << 15) / (100 * 100);
			for(int i = 0; i < n; ++i)
			{
				*dst++ = (short)(((*srcL++) * vol) >> 15);
				*dst++ = (short)(((*srcR++) * vol) >> 15);
			}
		}
		framesLeft -= n;
	}

	// If there are still frames left, end with silence.
	if(framesLeft > 0)
	{
		memset(dst, 0, sizeof(short) * MIX_CHANNELS * framesLeft);
	}

	// Write beat and step ticks.
	int rate = gMusic->getSpeed();
	if(myBeatTick.enabled) WriteTicks(buffer, frames, myBeatTick, rate);
	if(myNoteTick.enabled) WriteTicks(buffer, frames, myNoteTick, rate);
}

void writeFrames(short* buffer, int frames) override
{
	double srcAdvance = (double)frames;
	if(myMusicSpeed == 100)
	{
		// Source and target samplerate are equal.
		int64_t srcPos = llround(myPlayPosition);
		WriteSourceFrames(buffer, frames, srcPos);
	}
	else
	{
		double rate = (double)myMusicSpeed / 100.0;
		srcAdvance *= rate;

		// Source and target samplerate are different, mix to temporary buffer.
		int64_t srcPos = (int64_t)(myPlayPosition);
		int tmpFrames = frames * myMusicSpeed / 100;
		myMixBuffer.grow(tmpFrames * 2);
		WriteSourceFrames(myMixBuffer.data(), tmpFrames, srcPos);

		// Interpolate to the target samplerate.
		double tmpPos = 0.0;
		const short* tmpL = myMixBuffer.data() + 0;
		const short* tmpR = myMixBuffer.data() + 1;
		int tmpEnd = tmpFrames - 1;

		short* dst = buffer;
		for(int i = 0; i < frames; ++i)
		{
			int index0 = min((int)tmpPos, tmpEnd);
			int index1 = min(index0 + 1, tmpEnd);
			index0 *= MIX_CHANNELS;
			index1 *= MIX_CHANNELS;

			float w1 = (float)(tmpPos - floor(tmpPos));
			float w0 = 1.0f - w1;

			float l = (float)tmpL[index0] * w0 + (float)tmpL[index1] * w1;
			float r = (float)tmpR[index0] * w0 + (float)tmpR[index1] * w1;

			*dst++ = (short)min(max((int)l, SHRT_MIN), SHRT_MAX);
			*dst++ = (short)min(max((int)r, SHRT_MIN), SHRT_MAX);

			tmpPos += rate;
		}
	}

	myPlayPosition += srcAdvance;
}


// ================================================================================================
// MusicImpl :: OggVorbis conversion.

void startOggConversion()
{
	if(gSimfile->isClosed()) return;

	std::string dir = gSimfile->getDir();
	std::string file = gSimfile->get()->music;

	Path path(dir, file);
	if(path.hasExt("ogg"))
	{
		HudNote("Music is already in Ogg Vorbis format.");
	}
	else if(!mySamples.isCompleted())
	{
		HudNote("Wait for the music to finish loading.");
	}
	else if(mySamples.getNumFrames() == 0)
	{
		HudNote("There is no music loaded.");
	}
	else if(myOggConversionThread)
	{
		HudNote("Conversion is currently in progress.");
	}
	else
	{
		path.dropExt();
		Str::append(path.str, ".ogg");

		myOggConversionThread = new OggConversionThread;
		myOggConversionThread->outPath = path;

		if(gEditor->hasMultithreading())
		{
			auto box = myInfoBox.create();
			box->left = "Converting music to Ogg Vorbis...";
			myOggConversionThread->start();
		}
		else
		{
			myOggConversionThread->exec();
			finishOggConversion();
		}
	}
}

void terminateOggConversion()
{
	if(myOggConversionThread)
	{
		myOggConversionThread->terminate();
		delete myOggConversionThread;
		myOggConversionThread = nullptr;
	}
}

void finishOggConversion()
{
	if(myOggConversionThread)
	{
		if(myOggConversionThread->error.empty())
		{
			gMetadata->setMusicPath(gMetadata->findMusicFile());
		}
		else
		{
			HudError("Conversion failed: %s.", myOggConversionThread->error.c_str());
		}
		delete myOggConversionThread;
		myOggConversionThread = nullptr;
	}
}

// ================================================================================================
// MusicImpl :: general API.

void interruptStream()
{
	if(!myIsPaused)
	{
		myPlayStartTime = getPlayTime();
		myMixer->pause();
	}
}

void resumeStream()
{
	if(!myIsPaused)
	{
		myPlayPosition = myPlayStartTime * (double)mySamples.getFrequency();
		myPlayTimer = Debug::getElapsedTime();
		myMixer->resume();
	}
}

void tick()
{
	if(myLoadState != LOADING_DONE && myInfoBox)
	{
		if(mySamples.getLoadingProgress() > 0)
		{
			myInfoBox->setProgress(mySamples.getLoadingProgress() * 0.01f);
		}
		else
		{
			myInfoBox->setTime(mySamples.getLoadingTime());
		}
	}

	if(myLoadState == LOADING_ALLOCATING_AND_READING)
	{
		if(mySamples.isAllocated())
		{
			myLoadState = LOADING_ALLOCATED_AND_READING;
			gEditor->reportChanges(VCM_MUSIC_IS_ALLOCATED);
		}
	}
	else if(myLoadState == LOADING_ALLOCATED_AND_READING)
	{
		if(mySamples.isCompleted())
		{
			myInfoBox.destroy();
			myLoadState = LOADING_DONE;
			gEditor->reportChanges(VCM_MUSIC_IS_LOADED);
		}
	}

	if(myOggConversionThread)
	{
		if(myInfoBox)
		{
			myInfoBox->setProgress(myOggConversionThread->progress * 0.01f);
		}
		if(myOggConversionThread->isDone())
		{
			myInfoBox.destroy();
			finishOggConversion();
		}
	}
}

void pause()
{
	if(!myIsPaused)
	{
		interruptStream();
		myIsPaused = true;
	}
}

void play()
{
	if(myIsPaused)
	{
		myIsPaused = false;
		resumeStream();
	}
}

void seek(double seconds)
{
	interruptStream();
	myPlayStartTime = seconds;
	resumeStream();
}

double getPlayTime()
{
	double time = myPlayStartTime;
	if(!myIsPaused)
	{
		double rate = (double)myMusicSpeed * 0.01;
		time += Debug::getElapsedTime(myPlayTimer) * rate;
	}
	return time;
}

double getSongLength()
{
	return (double)mySamples.getNumFrames() / (double)mySamples.getFrequency();
}

const std::string& getTitle()
{
	return myTitle;
}

const std::string& getArtist()
{
	return myArtist;
}

bool isPaused()
{
	return myIsPaused;
}

const Sound& getSamples()
{
	return mySamples;
}

void setSpeed(int speed)
{
	speed = min(max(speed, 10), 400);
	if(myMusicSpeed != speed)
	{
		interruptStream();
		myMusicSpeed = speed;
		resumeStream();
		HudNote("Speed: %i%%", speed);
	}
}

int getSpeed()
{
	return myMusicSpeed;
}

void setVolume(int vol)
{
	vol = min(max(vol, 0), 100);
	if(myMusicVolume != vol)
	{
		interruptStream();
		myMusicVolume = vol;
		myIsMuted = false;
		resumeStream();
		HudNote("Volume: %i%%", vol);
	}
}

int getVolume()
{
	return myMusicVolume;
}

void setMuted(bool mute)
{
	if(myIsMuted != mute)
	{
		interruptStream();
		myIsMuted = mute;
		resumeStream();
		HudNote("Audio: %s", mute ? "muted" : "unmuted");
	}
}

bool isMuted()
{
	return myIsMuted;
}

void toggleBeatTick()
{
	interruptStream();
	myBeatTick.enabled = !myBeatTick.enabled;
	resumeStream();
	HudNote("Beat tick: %s", myBeatTick.enabled ? "on" : "off");
}

bool hasBeatTick()
{
	return myBeatTick.enabled;
}

void toggleNoteTick()
{
	interruptStream();
	myNoteTick.enabled = !myNoteTick.enabled;
	resumeStream();
	HudNote("Note tick: %s", myNoteTick.enabled ? "on" : "off");
}

bool hasNoteTick()
{
	return myNoteTick.enabled;
}

// ================================================================================================
// MusicImpl :: handling of external changes.

void updateBeatTicks()
{
	myBeatTick.frames.clear();

	double freq = (double)mySamples.getFrequency();
	double ofs = myTickOffsetMs / 1000.0;

	TempoTimeTracker tracker(gTempo->getTimingData());
	for(int row = 0, end = gSimfile->getEndRow(); row < end; row += ROWS_PER_BEAT)
	{
		double time = tracker.advance(row);
		int frame = (int)((time + ofs) * freq);
		myBeatTick.frames.push_back(frame);
	}
}

void updateNoteTicks()
{
	myNoteTick.frames.clear();

	double freq = (double)mySamples.getFrequency();
	double ofs = myTickOffsetMs / 1000.0;

	for(auto& note : *gNotes)
	{
		if(!(note.isMine | note.isWarped | note.isFake))
		{
			int frame = (int)((note.time + ofs) * freq);
			myNoteTick.frames.push_back(frame);
		}
	}
}

void onChanges(int changes)
{
	const int bits = VCM_NOTES_CHANGED | VCM_TEMPO_CHANGED | VCM_END_ROW_CHANGED | VCM_CHART_CHANGED;

	if (changes & bits)
	{
		if (changes & VCM_CHART_CHANGED) interruptStream();

		if (changes & VCM_TEMPO_CHANGED)
		{
			updateNoteTicks();
			updateBeatTicks();
		}
		else
		{
			if (changes & VCM_NOTES_CHANGED) updateNoteTicks();
			if (changes & VCM_END_ROW_CHANGED) updateBeatTicks();
		}
    
		if (changes & VCM_CHART_CHANGED) resumeStream();
	}
}

}; // MusicImpl

// ================================================================================================
// Audio :: create and destroy.

Music* gMusic = nullptr;

void Music::create(XmrNode& settings)
{
	gMusic = new MusicImpl;
	((MusicImpl*)gMusic)->loadSettings(settings);
}

void Music::destroy()
{
	delete (MusicImpl*)gMusic;
	gMusic = nullptr;
}

}; // namespace Vortex