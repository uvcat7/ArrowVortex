#pragma once

#include <Simfile/Simfile.h>

namespace Vortex {

/// Manages the metadata of the active simfile.
struct MetadataMan {
    static void create();
    static void destroy();

    /// Called when the active simfile changes.
    virtual void update(Simfile* simfile) = 0;

    /// Returns the relative path of first music file in the simfile directory.
    virtual std::string findMusicFile() = 0;

    /// Returns the relative path of first banner file in the simfile directory.
    virtual std::string findBannerFile() = 0;

    /// Returns the relative path of the first background file in the simfile
    /// directory.
    virtual std::string findBackgroundFile() = 0;

    /// Returns the relative path of the first CD title file in the simfile
    /// directory.
    virtual std::string findCdTitleFile() = 0;

    /// Sets the song title.
    virtual void setTitle(const std::string& s) = 0;

    /// Sets the transliterated song title.
    virtual void setTitleTranslit(const std::string& s) = 0;

    /// Sets the song subtitle.
    virtual void setSubtitle(const std::string& s) = 0;

    /// Sets the transliterated song subtitle.
    virtual void setSubtitleTranslit(const std::string& s) = 0;

    /// Sets the song artist.
    virtual void setArtist(const std::string& s) = 0;

    /// Sets the transliterated song artist.
    virtual void setArtistTranslit(const std::string& s) = 0;

    /// Sets the music genre.
    virtual void setGenre(const std::string& s) = 0;

    /// Sets the simfile creator.
    virtual void setCredit(const std::string& s) = 0;

    /// Sets the relative path of the music file.
    virtual void setMusicPath(const std::string& s) = 0;

    /// Sets the relative path of the banner file.
    virtual void setBannerPath(const std::string& s) = 0;

    /// Sets the relative path of the background file.
    virtual void setBackgroundPath(const std::string& s) = 0;

    /// Sets the relative path of the CD title file.
    virtual void setCdTitlePath(const std::string& s) = 0;

    /// Sets the music preview time to the given start time and duration in
    /// seconds.
    virtual void setMusicPreview(double start, double len) = 0;
};

extern MetadataMan* gMetadata;

};  // namespace Vortex
