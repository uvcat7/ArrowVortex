#pragma once

#include <Simfile/Simfile.h>

namespace Vortex {

/// Manages the metadata of the active simfile.
struct MetadataMan
{
	static void create();
	static void destroy();

	/// Called when the active simfile changes.
	virtual void update(Simfile* simfile) = 0;

	/// Returns the relative path of first music file in the simfile directory.
	virtual String findMusicFile() = 0;

	/// Returns the relative path of first banner file in the simfile directory.
	virtual String findBannerFile() = 0;

	/// Returns the relative path of the first background file in the simfile directory.
	virtual String findBackgroundFile() = 0;

	/// Sets the song title.
	virtual void setTitle(StringRef s) = 0;

	/// Sets the transliterated song title.
	virtual void setTitleTranslit(StringRef s) = 0;

	/// Sets the song subtitle.
	virtual void setSubtitle(StringRef s) = 0;

	/// Sets the transliterated song subtitle.
	virtual void setSubtitleTranslit(StringRef s) = 0;

	/// Sets the song artist.
	virtual void setArtist(StringRef s) = 0;

	/// Sets the transliterated song artist.
	virtual void setArtistTranslit(StringRef s) = 0;

	/// Sets the music genre.
	virtual void setGenre(StringRef s) = 0;

	/// Sets the simfile creator.
	virtual void setCredit(StringRef s) = 0;

	/// Sets the relative path of the music file.
	virtual void setMusicPath(StringRef s) = 0;

	/// Sets the relative path of the banner file.
	virtual void setBannerPath(StringRef s) = 0;

	/// Sets the relative path of the background file.
	virtual void setBackgroundPath(StringRef s) = 0;

	/// Sets the relative path of the CD title file.
	virtual void setCdTitlePath(StringRef s) = 0;

	/// Sets the music preview time to the given start time and duration in seconds.
	virtual void setMusicPreview(double start, double len) = 0;
};

extern MetadataMan* gMetadata;

}; // namespace Vortex
