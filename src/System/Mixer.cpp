#include <System/Mixer.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <filesystem>
#include <vector>
namespace fs = std::filesystem;

namespace Vortex {

// ================================================================================================
// MixerImpl :: member data.

struct MixerImpl : public Mixer {
    MIX_Mixer* mixer = nullptr;
    MIX_Track* music_track = nullptr;
    MIX_Audio* music = nullptr;
    std::vector<MIX_Audio*> sfx;
    std::vector<MIX_Track*> sfx_tracks;

    int myFreeBlockIndex = 0;

    char* myBlockMemory = nullptr;

    MixSource* mySource;

    // ================================================================================================
    // MixerImpl :: constructor and destructor.

    ~MixerImpl() override {
        close();
        for (MIX_Audio* s : sfx) {
            MIX_DestroyAudio(s);
        }
        for (MIX_Track* t : sfx_tracks) {
            MIX_DestroyTrack(t);
        }
        sfx.clear();
    }

    MixerImpl() {
        if (MIX_Init())
            mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
                                          nullptr);
        if (!mixer) {
            HudError("Failed to create mixer with error %s.\n", SDL_GetError());
        };
        sfx.clear();
        open_sfx(fs::path("assets/sound beat tick.wav"));
        open_sfx(fs::path("assets/sound note tick.wav"));
    }

    bool is_initialized() override { return mixer != nullptr; }

    void close() override {
        MIX_StopAllTracks(mixer, 0);
        MIX_DestroyTrack(music_track);
        MIX_DestroyAudio(music);
    }

    bool open_sfx(fs::path path) override {
        MIX_Audio* s = MIX_LoadAudio(mixer, path.string().c_str(), true);
        MIX_Track* t = MIX_CreateTrack(mixer);
        MIX_SetTrackAudio(t, s);
        sfx.push_back(s);
        sfx_tracks.push_back(t);
        return s != nullptr;
    }

    void play_sfx(SoundEffects s, float volume) override {
        MIX_Track* track = sfx_tracks.at(static_cast<int>(s));
        if (!track) return;
        MIX_SetTrackGain(track, volume);
        if (!MIX_PlayTrack(music_track, 0))
            HudError("Failed to start sfx #d playback with error %s",
                     static_cast<int>(s), SDL_GetError());
    }

    bool open(fs::path path) override {
        music = MIX_LoadAudio(mixer, path.string().c_str(), true);
        music_track = MIX_CreateTrack(mixer);
        MIX_SetTrackAudio(music_track, music);
        return music != nullptr;
    }

    void pause() override { MIX_PauseAllTracks(mixer); }

    void resume(int64_t ms, float rate, float volume) override {
        if (!MIX_PlayTrack(music_track, 0))
            HudError("Failed to start music playback with error %s",
                     SDL_GetError());
        int64_t frames = MIX_TrackMSToFrames(music_track, ms);
        MIX_SetTrackFrequencyRatio(music_track, rate);
        MIX_SetTrackGain(music_track, volume);
        MIX_SetTrackPlaybackPosition(music_track, frames);
        HudWarning("Mixer::resume: %d, %d", ms, frames);
    }

};  // MixerImpl

// ================================================================================================
// Mixer API.

Mixer* Mixer::create() { return new MixerImpl; }

Mixer::~Mixer() = default;

};  // namespace Vortex