#include <Managers/NoteskinMan.h>

#include <Core/QuadBatch.h>
#include <Core/StringUtils.h>
#include <Core/Utils.h>
#include <Core/Xmr.h>

#include <System/File.h>

#include <Simfile/Chart.h>

#include <Editor/Common.h>
#include <Editor/Menubar.h>

#include <Managers/StyleMan.h>

#include <algorithm>
#include <vector>

namespace Vortex {
namespace {

struct SkinType
{
	String name;
	bool supportsAll;
	std::vector<const Style*> supportedStyles;
};

struct NoteskinImpl : public Noteskin
{
	int type;
	const Style* style;
};

struct SpriteTransform
{
	SpriteTransform() : x(0), y(0), w(0), h(0), rot(0), mir(0) {}
	int x, y, w, h, rot, mir;
};

static bool LoadTexture(StringRef path, StringRef dir, Texture& out)
{
	out = Texture((dir + path).str());
	if(out.handle()) return true;

	uchar dummyTex[4] = {255, 0, 255, 255};
	out = Texture(1, 1, dummyTex);
	return false;
}

static int RowToRowType(int row)
{
	int lookup[NUM_ROW_TYPES] = {4, 8, 12, 16, 24, 32, 48, 64, 192};
	for(int i = 0; i < NUM_ROW_TYPES; ++i)
	{
		if(lookup[i] == row) return i;
	}
	return 0;
}

static void ReadRange(bool* flags, const XmrNode* n, const char* attrib, int num, bool areRows)
{
	XmrAttrib* a = n->attrib(attrib);
	if(a)
	{
		for(int i = 0; i < a->numValues; ++i)
		{
			int tmp = 0;
			if(Str::read(a->values[i], &tmp))
			{
				int index = areRows ? RowToRowType(tmp) : tmp;
				if(index >= 0 && index < num)
				{
					flags[index] = true;
				}
				else
				{
					HudWarning("Noteskin, invalid %s (%i).", attrib, index);
				}
			}
		}
	}
	else
	{
		for(int i = 0; i < num; ++i)
		{
			flags[i] = true;
		}
	}
}

int GetMirrorType(const char* mirror)
{
	switch(mirror[0])
	{
		case 'h': return 1;
		case 'v': return 2;
	};
	return 0;
}

static void ParseSprite(XmrNode* node, int numCols, int players, bool hasRows, const char* name,
	std::vector<SpriteTransform>& spr)
{
	ForXmrNodesNamed(n, node, name)
	{
		int uvs[2], size[2], rot;

		bool setUVs = (n->get("src", uvs, 2) == 2);
		bool setSize = (n->get("size", size, 2) == 2);
		bool setRot = n->get("rot", &rot);
		const char* setMir = n->get("mirror");

		bool setCols[SIM_MAX_COLUMNS] = {};
		bool setRows[NUM_ROW_TYPES] = {};
		bool setPlayers[SIM_MAX_PLAYERS] = {};

		ReadRange(setRows, n, "row", NUM_ROW_TYPES, true);
		ReadRange(setCols, n, "col", numCols, false);
		ReadRange(setPlayers, n, "player", players, false);

		for(int pn = 0; pn < players; ++pn)
		{
			if(!setPlayers[pn]) continue;

			for(int c = 0; c < numCols; ++c)
			{
				if(!setCols[c]) continue;

				if(hasRows)
				{
					for(int r = 0; r < NUM_ROW_TYPES; ++r)
					{
						if(!setRows[r]) continue;

						int idx = (pn * numCols + c) * NUM_ROW_TYPES + r;
						SpriteTransform& t = spr[idx];
						if(setUVs)  t.x = uvs[0], t.y = uvs[1];
						if(setSize) t.w = size[0], t.h = size[1];
						if(setRot)  t.rot = rot;
						if(setMir)  t.mir = GetMirrorType(setMir);
					}
				}
				else
				{
					SpriteTransform& t = spr[pn * numCols + c];
					if(setUVs)  t.x = uvs[0], t.y = uvs[1];
					if(setSize) t.w = size[0], t.h = size[1];
					if(setRot)  t.rot = rot;
					if(setMir)  t.mir = GetMirrorType(setMir);
				}
			}
		}
	}
}

static void ParseSpriteAttrib(const XmrNode* node, const char* attrib, int* dst, int numCols)
{
	int v;
	if(node->get(attrib, &v))
	{
		bool setCols[SIM_MAX_COLUMNS] = {};
		ReadRange(setCols, node, "col", SIM_MAX_COLUMNS, false);
		for(int c = 0; c < numCols; ++c)
		{
			if(setCols[c]) dst[c] = v;
		}
	}
}

static void SetUVS(const Texture& tex, SpriteTransform& t, BatchSprite& spr)
{
	spr.width = (t.w >= 0) ? t.w : 64;
	spr.height = (t.h >= 0) ? t.h : 64;

	if(tex.size().x == 0 || tex.size().y == 0)
	{
		float tmp[8] = {0, 0, 1, 0, 0, 1, 1, 1};
		memcpy(spr.uvs, tmp, sizeof(float) * 8);
	}
	else
	{
		double du = 1.0f / (double)tex.size().x;
		double dv = 1.0f / (double)tex.size().y;
		double ul = du * (double)t.x, ur = ul + du * (double)spr.width;
		double vt = dv * (double)t.y, vb = vt + dv * (double)spr.height;
		double uvs[8] = {ul, vt, ur, vt, ul, vb, ur, vb};
		for(int i = 0; i < 8; ++i) spr.uvs[i] = (float)uvs[i];

		// handle rotation.
		float* p = spr.uvs;
		if(t.rot == 90)
		{
			spr.rotateUVs(BatchSprite::ROT_RIGHT_90);
		}
		else if(t.rot == 180)
		{
			spr.rotateUVs(BatchSprite::ROT_180);
		}
		else if(t.rot == 270)
		{
			spr.rotateUVs(BatchSprite::ROT_LEFT_90);
		}

		// handle mirroring.
		if(t.mir == 1)
		{
			spr.mirrorUVs(BatchSprite::MIR_HORZ);
		}
		else if(t.mir == 2)
		{
			spr.mirrorUVs(BatchSprite::MIR_VERT);
		}
	}
}

static NoteskinImpl* CreateNoteskin(const Style* style)
{
	NoteskinImpl* out = new NoteskinImpl;

	int numCols = style->numCols;
	int numPayers = style->numPlayers;

	out->note = new BatchSprite[numPayers * numCols * NUM_ROW_TYPES];
	out->mine = new BatchSprite[numPayers * numCols];

	out->holdBody = new BatchSprite[numCols * 2];
	out->holdTail = new BatchSprite[numCols * 2];

	out->recepOn = new BatchSprite[numCols];
	out->recepOff = new BatchSprite[numCols];
	out->recepGlow = new BatchSprite[numCols];

	out->colX = new int[numCols];
	out->holdY = new int[numCols * 2];

	out->style = style;
	out->type = -1;

	return out;
}

static void DeleteNoteskin(NoteskinImpl* skin)
{
	if(!skin) return;

	delete[] skin->note;
	delete[] skin->mine;

	delete[] skin->holdBody;
	delete[] skin->holdTail;

	delete[] skin->recepOn;
	delete[] skin->recepOff;
	delete[] skin->recepGlow;

	delete[] skin->colX;
	delete[] skin->holdY;

	delete skin;
}

static void LoadNoteskin(NoteskinImpl* skin, const SkinType& type)
{
	XmrDoc doc;
	String dir, path;

	// Load the noteskin file.
	bool loadFallback = true;
	if(type.name.len())
	{
		String filename = type.supportsAll ? "all-styles" : skin->style->id;
		dir = "noteskins/" + type.name + "/";
		path = dir + filename + ".txt";		
		if(doc.loadFile(path.str()) == XMR_SUCCESS)
		{
			loadFallback = false;
		}
		else
		{
			HudError("Unable to load noteskin: %s", path.str());
		}
	}
	
	// If necessary, load the fallback noteskin.
	if(loadFallback)
	{
		dir = "noteskins/";
		path = dir + "fallback.txt";
		if(doc.loadFile(path.str()) != XMR_SUCCESS)
		{
			HudError("Unable to load noteskin: %s", path.str());
		}
	}

	XmrNode* node = &doc;

	int numCols = skin->style->numCols;
	int numPlayers = skin->style->numPlayers;

	// Set default column positions and hold/roll body y-offsets.
	int totalWidth = (numCols - 1) * 68;
	for(int i = 0; i < numCols; ++i)
	{
		skin->colX[i] = i * 68 - totalWidth / 2;
		skin->holdY[i] = skin->holdY[numCols + i] = 0;
	}

	// Read the XMR document.
	SpriteTransform st;

	std::vector<SpriteTransform> note(numCols * NUM_ROW_TYPES * numPlayers, st);
	std::vector<SpriteTransform> mine(numCols * numPlayers, st);
	std::vector<SpriteTransform> holdBody(numCols, st);
	std::vector<SpriteTransform> holdTail(numCols, st);
	std::vector<SpriteTransform> rollBody(numCols, st);
	std::vector<SpriteTransform> rollTail(numCols, st);
	std::vector<SpriteTransform> receptorOn(numCols, st);
	std::vector<SpriteTransform> receptorOff(numCols, st);
	std::vector<SpriteTransform> receptorGlow(numCols, st);

	ParseSprite(node, numCols, numPlayers, true, "Note", note);
	ParseSprite(node, numCols, numPlayers, false, "Mine", mine);
	ParseSprite(node, numCols, 1, false, "Receptor-on", receptorOn);
	ParseSprite(node, numCols, 1, false, "Receptor-off", receptorOff);
	ParseSprite(node, numCols, 1, false, "Receptor-glow", receptorGlow);
	ParseSprite(node, numCols, 1, false, "Hold-body", holdBody);
	ParseSprite(node, numCols, 1, false, "Hold-tail", holdTail);
	ParseSprite(node, numCols, 1, false, "Roll-body", rollBody);
	ParseSprite(node, numCols, 1, false, "Roll-tail", rollTail);

	// Parse the y-position of holds.
	ForXmrNodesNamed(n, node, "Hold-body")
	{
		ParseSpriteAttrib(n, "y", skin->holdY, numCols);
	}

	// Parse the y-position of rolls.
	ForXmrNodesNamed(n, node, "Roll-body")
	{
		ParseSpriteAttrib(n, "y", skin->holdY + numCols, numCols);
	}

	// Parse the x-position of columns.
	ForXmrNodesNamed(n, node, "Receptor")
	{
		ParseSpriteAttrib(n, "x", skin->colX, numCols);
	}

	// Load the noteskin textures.
	auto notesPath = node->get("Texture-notes", "");
	if(!LoadTexture(notesPath, dir, skin->noteTex))
	{
		HudError("Could not load notes texture.");
	}
	auto receptorsPath = node->get("Texture-receptors", "");
	if(!LoadTexture(receptorsPath, dir, skin->recepTex))
	{
		HudError("Could not load receptors texture.");
	}
	auto glowPath = node->get("Texture-glow", "");
	if(!LoadTexture(glowPath, dir, skin->glowTex))
	{
		HudError("Could not load glow texture.");
	}

	// Compute all the sprite UVS.
	for(int c = 0; c < numCols; ++c)
	{
		for(int pn = 0; pn < numPlayers; ++pn)
		{
			for(int r = 0; r < NUM_ROW_TYPES; ++r)
			{
				int idx = (pn * numCols + c) * NUM_ROW_TYPES + r;
				SetUVS(skin->noteTex, note[idx], skin->note[idx]);
			}
			int idx = pn * numCols + c;
			SetUVS(skin->noteTex, mine[idx], skin->mine[idx]);
		}
		SetUVS(skin->recepTex, receptorOn[c], skin->recepOn[c]);
		SetUVS(skin->recepTex, receptorOff[c], skin->recepOff[c]);
		SetUVS(skin->glowTex, receptorGlow[c], skin->recepGlow[c]);
		SetUVS(skin->noteTex, holdBody[c], skin->holdBody[c]);
		SetUVS(skin->noteTex, holdTail[c], skin->holdTail[c]);
		SetUVS(skin->noteTex, rollBody[c], skin->holdBody[numCols + c]);
		SetUVS(skin->noteTex, rollTail[c], skin->holdTail[numCols + c]);
	}

	// Calculate the x-positions of the left and right side of the note field.
	skin->leftX = skin->colX[0] - skin->recepOff[0].width / 2;
	skin->rightX = skin->colX[numCols - 1] + skin->recepOff[numCols - 1].width / 2;
}

}; // anonymous namespace.

// ================================================================================================
// NoteskinManImpl.

#define NOTESKIN_MAN ((NoteskinManImpl*)gNoteskin)

struct NoteskinManImpl : public NoteskinMan {

std::vector<int> myPriorities;
std::vector<SkinType> myTypes;
std::vector<NoteskinImpl*> mySkins;
NoteskinImpl* myActiveSkin;
const Style* myActiveStyle;
SkinType myFallback;

// ================================================================================================
// NoteskinManImpl :: constructor and destructor.

NoteskinManImpl()
{
	myFallback.supportsAll = true;

	for(auto skin : File::findDirs("noteskins", false))
	{
		SkinType type;
		type.supportsAll = false;

		// Capitalize the first letter.
		type.name = skin.name();
		char* c = type.name.begin();
		if(*c >= 'a' && *c <= 'z') *c += 'A' - 'a';

		// Make a list of available noteskin files.
		for(auto files : File::findFiles(skin, false, "txt"))
		{
			String id = files.name();
			if(id == "all-styles")
			{
				type.supportsAll = true;
			}
			else
			{
				auto style = gStyle->findStyle(id);
				if(!style)
				{
					HudError("Could not find style '%s' used in noteskin: %s",
						id.str(), type.name.str());
					continue;
				}
				type.supportedStyles.push_back(style);
			}
		}

		// Make sure the noteskin has at least one valid style.
		if(type.supportsAll == false && type.supportedStyles.empty())
		{
			HudError("Could not find any supported styles in noteskin: %s",
				type.name.str());
			continue;
		}

		myTypes.push_back(type);
	}

	for(int i = 1; i < myTypes.size(); ++i)
	{
		myPriorities.push_back(i);
	}

	myActiveStyle = nullptr;
	myActiveSkin = nullptr;
}

~NoteskinManImpl()
{
	for(auto skin : mySkins)
	{
		DeleteNoteskin(skin);
	}
}

// ================================================================================================
// NoteskinManImpl :: load / save settings.

void loadSettings(XmrNode& settings)
{
	// Read the noteskin preferences from the settings.
	std::vector<String> prefs;
	XmrNode* view = settings.child("view");
	if(view)
	{
		auto attrib = view->attrib("noteskinPrefs");
		if(attrib)
		{
			for(int i = attrib->numValues - 1; i >= 0; --i)
			{
				prefs.push_back(attrib->values[i]);
			}
		}
	}

	// If there are no preferences, use the default preferences.
	if(prefs.empty())
	{
		prefs.push_back("Pump");
		prefs.push_back("Simple");
		prefs.push_back("Classic");
		prefs.push_back("Metal");
	}

	// Translate the preferences to a priority list.
	for(auto& pref : prefs)
	{
		for(int i = 0; i < myTypes.size(); ++i)
		{
			if(pref == myTypes[i].name)
			{
				giveTopPriority(i);
			}
		}
	}
}

void saveSettings(XmrNode& settings)
{
	XmrNode* view = settings.child("view");
	if(!view) view = settings.addChild("view");
	
	// Save the noteskin preferences.
	std::vector<const char*> prefs;
	for(int i : myPriorities) prefs.push_back(myTypes[i].name.str());
	view->addAttrib("noteskinPrefs", prefs.data(), min(prefs.size(), size_t(5)));
}

// ================================================================================================
// NoteskinManImpl :: member functions.

static bool Supports(const SkinType& type, const Style* style)
{
	if(type.supportsAll) return true;

	return std::find(type.supportedStyles.begin(), type.supportedStyles.end(), style) != type.supportedStyles.end();
}

void update(Chart* chart)
{
 	const Style* style = chart ? chart->style : nullptr;
	if(myActiveStyle == style) return;
	myActiveStyle = style;

	int type = -1;
	NoteskinImpl* skin = nullptr;

	// Find the noteskin for the style.
	if(style)
	{
		mySkins.resize(style->index + 1, nullptr);
		skin = mySkins[style->index];

		// Create a new noteskin if the style does not have one.
		if(!skin)
		{
			skin = CreateNoteskin(style);
			mySkins[style->index] = skin;
		}

		// Find the noteskin type with the highest priority that supports the style.
		for(auto i : myPriorities)
		{
			if(Supports(myTypes[i], style))
			{
				type = i;
				break;
			}
		}
	}

	// Replace the active noteskin with the new skin.
	if(myActiveSkin != skin)
	{
		myActiveSkin = skin;
		if(skin && skin->type != type)
		{
			if(type >= 0)
			{
				LoadNoteskin(skin, myTypes[type]);
				skin->type = type;
			}
			else
			{
				LoadNoteskin(skin, myFallback);
			}			
		}
		gMenubar->update(Menubar::VIEW_NOTESKIN);
	}
}

int getNumTypes() const
{
	return myTypes.size();
}

StringRef getName(int type) const
{
	return myTypes[type].name;
}

bool isSupported(int type) const
{
	return Supports(myTypes[type], gStyle->get());
}

void giveTopPriority(int type)
{
	std::erase(myPriorities, type);
	myPriorities.insert(myPriorities.begin(), type);
}

void setType(int type)
{
	type = clamp(type, 0, myTypes.size());
	if(myActiveSkin && myActiveSkin->type != type)
	{
		LoadNoteskin(myActiveSkin, myTypes[type]);
		myActiveSkin->type = type;
		giveTopPriority(type);
		gMenubar->update(Menubar::VIEW_NOTESKIN);
	}
}

int getType() const
{
	return myActiveSkin ? myActiveSkin->type : 0;
}

const Noteskin* get() const
{
	return myActiveSkin;
}

}; // NoteskinImpl.

// ================================================================================================
// NoteskinMan :: create / destroy.

NoteskinMan* gNoteskin = nullptr;

void NoteskinMan::create(XmrNode& settings)
{
	gNoteskin = new NoteskinManImpl;
	NOTESKIN_MAN->loadSettings(settings);
}

void NoteskinMan::destroy()
{
	delete NOTESKIN_MAN;
	gNoteskin = nullptr;
}

}; // namespace Vortex
