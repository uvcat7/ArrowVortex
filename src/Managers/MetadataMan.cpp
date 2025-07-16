#include <Managers/MetadataMan.h>

#include <Core/StringUtils.h>

#include <System/File.h>

#include <Editor/Common.h>
#include <Editor/History.h>
#include <Editor/Music.h>
#include <Editor/Editor.h>

#include <Managers/SimfileMan.h>

namespace Vortex {

#define METADATA_MAN ((MetadataManImpl*)gMetadata)

struct MetadataManImpl : public MetadataMan {

// ================================================================================================
// MetadataManImpl :: member data.

Simfile* mySimfile;

History::EditId myApplyStringPropertyId;
History::EditId myApplyMusicPreviewId;

// ================================================================================================
// MetadataManImpl :: constructor and destructor.

~MetadataManImpl()
{
}

MetadataManImpl()
{
	mySimfile = nullptr;

	myApplyStringPropertyId = gHistory->addCallback(ApplyStringProperty);
	myApplyMusicPreviewId   = gHistory->addCallback(ApplyMusicPreview);
}

// ================================================================================================
// MetadataManImpl :: string property editing.

void myQueueStringProperty(std::string* target, const std::string& value, const char* name)
{
	WriteStream stream;
	stream.write(target);
	stream.writeStr(*target);
	stream.writeStr(value);
	stream.write(name);
	gHistory->addEntry(myApplyStringPropertyId, stream.data(), stream.size());
}

static std::string ApplyStringProperty(ReadStream& in, History::Bindings bound, bool undo, bool redo)
{
	std::string msg;
	auto target = in.read<std::string*>();
	auto before = in.readStr();
	auto after = in.readStr();
	auto name = in.read<const char*>();
	if(in.success())
	{
		bool isRemove = (before != "" && after == "");
		bool isChange = (before != "" && after != "");

		*target = undo ? before : after;

		msg = (isChange ? "Changed " : (isRemove ? "Removed " : "Added "));
		msg = msg + name;
		msg = msg + ": ";
		msg = msg + (isChange ? (before + " {g:arrow right} " + after) : (isRemove ? before : after));

		auto sim = bound.simfile;
		int changes = VCM_SONG_PROPERTIES_CHANGED;
		if(target == &sim->background)
		{
			changes |= VCM_BACKGROUND_PATH_CHANGED;
		}
		else if(target == &sim->banner)
		{
			changes |= VCM_BANNER_PATH_CHANGED;
		}
		else if(target == &sim->music)
		{
			gMusic->load();
			changes |= VCM_MUSIC_PATH_CHANGED;
		}
		gEditor->reportChanges(changes);
	}
	return msg;
}

// ================================================================================================
// MetadataManImpl :: music preview editing.

struct PreviewTime { double start, len; };

void myQueueMusicPreview(double start, double length)
{
	WriteStream stream;
	stream.write<PreviewTime>({mySimfile->previewStart, mySimfile->previewLength});
	stream.write<PreviewTime>({start, length});
	gHistory->addEntry(myApplyMusicPreviewId, stream.data(), stream.size());
}

static std::string ApplyMusicPreview(ReadStream& in, History::Bindings bound, bool undo, bool redo)
{
	std::string msg;
	auto before = in.read<PreviewTime>();
	auto after = in.read<PreviewTime>();
	if(in.success())
	{
		PreviewTime value = (undo ? before : after);
		if(value.start == 0.0 && value.len == 0.0)
		{
			msg = "Cleared music preview";
		}
		else
		{
			msg = "Changed music preview: ";
			msg = msg + Str::formatTime(value.start);
			msg = msg + " - ";
			msg = msg + Str::formatTime(value.start + value.len);
		}

		auto sim = bound.simfile;
		sim->previewStart = value.start;
		sim->previewLength = value.len;
		gEditor->reportChanges(VCM_SONG_PROPERTIES_CHANGED);
	}
	return msg;
}

// ================================================================================================
// MetadataManImpl :: set functions.

void mySetString(std::string* target, const std::string& value, const char* name)
{
	if(mySimfile && !(*target == value))
	{
		myQueueStringProperty(target, value, name);
	}
}

void setMusicPreview(double start, double length)
{
	if(length <= 0.0) start = length = 0.0;

	if(mySimfile && (mySimfile->previewStart != start || mySimfile->previewLength != length))
	{
		myQueueMusicPreview(start, length);
	}
}

void setTitle(const std::string& s)
{
	mySetString(&mySimfile->title, s, "title");
}

void setTitleTranslit(const std::string& s)
{
	mySetString(&mySimfile->titleTr, s, "transliterated title");
}

void setSubtitle(const std::string& s)
{
	mySetString(&mySimfile->subtitle, s, "subtitle");
}

void setSubtitleTranslit(const std::string& s)
{
	mySetString(&mySimfile->subtitleTr, s, "transliterated subtitle");
}

void setArtist(const std::string& s)
{
	mySetString(&mySimfile->artist, s, "artist");
}

void setArtistTranslit(const std::string& s)
{
	mySetString(&mySimfile->artistTr, s, "transliterated artist");
}

void setGenre(const std::string& s)
{
	mySetString(&mySimfile->genre, s, "genre");
}

void setCredit(const std::string& s)
{
	mySetString(&mySimfile->credit, s, "credit");
}

void setMusicPath(const std::string& s)
{
	mySetString(&mySimfile->music, s, "music");
}

void setBannerPath(const std::string& s)
{
	mySetString(&mySimfile->banner, s, "banner");
}

void setBackgroundPath(const std::string& s)
{
	mySetString(&mySimfile->background, s, "background");
}

void setCdTitlePath(const std::string& s)
{
	mySetString(&mySimfile->cdTitle, s, "CD title");
}

// ================================================================================================
// MetadataManImpl :: autofill functions.

std::string findImageFile(const char* full, const char* abbrev)
{
	auto paths = File::findFiles(gSimfile->getDir(), false);
	for(auto& path : paths)
	{
		std::string f(path.filename());
		Str::toLower(f);
		std::string cmp[] = {" ", "-", "_"};
		for(auto& s : cmp)
		{
			std::string prefix = abbrev + s;
			std::string postfix = s + abbrev;
			if(Str::startsWith(f, prefix.c_str()) || Str::endsWith(f, postfix.c_str()))
			{
				return path.filename();
			}
		}
		if(Str::find(f, abbrev) != std::string::npos || Str::find(f, full) != std::string::npos)
		{
			return path.filename();
		}
	}
	paths = File::findFiles(gSimfile->getDir() + "..\\", false);
	for (auto& path : paths)
	{
		std::string f(path.filename());
		Str::toLower(f);
		std::string cmp[] = { " ", "-", "_" };
		for (auto& s : cmp)
		{
			std::string prefix = abbrev + s;
			std::string postfix = s + abbrev;
			if (Str::startsWith(f, prefix.c_str()) || Str::endsWith(f, postfix.c_str()))
			{
				return "..\\" + path.filename();
			}
		}
		if (Str::find(f, abbrev) != std::string::npos || Str::find(f, full) != std::string::npos)
		{
			return "..\\" + path.filename();
		}
	}
	return {};
}

std::string findMusicFile()
{
	std::string out;
	int priority = 0;
	auto audioFiles = File::findFiles(gSimfile->getDir(), false, "ogg;wav;mp3");
	for(auto& audioFile : audioFiles)
	{
		if(audioFile.hasExt("ogg") && priority < 3)
		{
			out = audioFile.filename();
			priority = 3;
		}
		if(audioFile.hasExt("wav") && priority < 2)
		{
			out = audioFile.filename();
			priority = 2;
		}
		if(audioFile.hasExt("mp3") && priority < 1)
		{
			out = audioFile.filename();
			priority = 1;
		}
	}
	return out;
}

std::string findBannerFile()
{
	return findImageFile("bn", "banner");
}

std::string findBackgroundFile()
{
	return findImageFile("bg", "background");
}

std::string findCdTitleFile()
{
	return findImageFile("cd", "title");
}

// ================================================================================================
// MetadataManImpl :: other member functions.

void update(Simfile* simfile)
{
	mySimfile = simfile;
}

}; // MetadataManImpl

// ================================================================================================
// Song :: create and destroy.

MetadataMan* gMetadata = nullptr;

void MetadataMan::create()
{
	gMetadata = new MetadataManImpl;
}

void MetadataMan::destroy()
{
	delete METADATA_MAN;
	gMetadata = nullptr;
}

}; // namespace Vortex
