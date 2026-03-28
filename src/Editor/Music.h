#pragma once

#include <Editor/Sound.h>
#include <Editor/ConvertAudio.h>

namespace Vortex {

struct Music {
    static void create(XmrNode& settings);
    static void destroy();

    virtual void saveSettings(XmrNode& settings) = 0;

    virtual void tick() = 0;

    /// Called by the editor when changes were made to the simfile.
    virtual void onChanges(int changes) = 0;

    /// Destroys the audio mixer and unloads the current music.
    virtual void unload() = 0;

    /// Creates the audio mixer and starts loading music from the simfile music
    /// path.
    virtual void load() = 0;

    /// Pauses the audio mixer.
    virtual void pause() = 0;

    /// Resumes the audio mixer.
    virtual void play() = 0;

    /// Sets the playback position of the audio mixer to the given location.
    virtual void seek(double seconds) = 0;

    /// Sets the volume level of the audio mixer [0-100%].
    virtual void setVolume(int percentage) = 0;

    /// Returns the volume level of the audio mixer [0-100%].
    virtual int getVolume() = 0;

    /// Mutes or unmutes the audio mixer.
    virtual void setMuted(bool mute) = 0;

    /// Returns true if the audio mixer is muted, false otherwise.
    virtual bool isMuted() = 0;

    /// Sets the playback speed of the audio mixer [10-400%].
    virtual void setSpeed(int percentage) = 0;

    /// Returns the playback speed of the audio mixer [10-400%].
    virtual int getSpeed() = 0;

    /// Enables/disables the beat tick sound.
    virtual void toggleBeatTick() = 0;

    /// Enables/disables the note tick sound.
    virtual void toggleNoteTick() = 0;

    /// Convert a source file to a supported audio format.
    /// Uses dialogs to select the source and output file.
    virtual void startAudioConversion() = 0;

    /// Convert the current simfile music to the given audio format.
    virtual void startAudioConversion(AudioFormat fmt) = 0;

    /// Created a thread audio conversion for a source file to the given audio
    /// format, updating the simfile audio if it's currently loaded.
    virtual void startAudioConversion(AudioFormat fmt, fs::path source,
                                      fs::path output, bool isSimfile) = 0;

    /// Returns true if the audio mixer is paused, false otherwise.
    virtual bool isPaused() = 0;

    /// Returns the playback position of the audio mixer.
    virtual double getPlayTime() = 0;

    /// Returns the total song length of the current music.
    virtual double getSongLength() = 0;

    /// Returns the title of the current music, as reported by the metadata.
    virtual const std::string& getTitle() = 0;

    /// Returns the artist of the current music, as reported by the metadata.
    virtual const std::string& getArtist() = 0;

    /// Returns the audio samples of the current music.
    virtual const Sound& getSamples() = 0;
};

extern Music* gMusic;

};  // namespace Vortex