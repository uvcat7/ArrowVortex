#include <Precomp.h>

#include <Core/Graphics/Font.h>
#include <Core/Graphics/FontManager.h>

namespace AV {

// =====================================================================================================================
// Font.

typedef FontManager::Entry FontEntry;

Font::~Font()
{
	if (mySource) FontManager::release((FontEntry*)mySource);
}

Font::Font()
	: mySource(nullptr)
{
}

Font::Font(Font&& font)
	: mySource(font.mySource)
{
	font.mySource = nullptr;
}

Font::Font(const Font& font)
	: mySource(font.mySource)
{
	if (mySource) FontManager::addReference((FontEntry*)mySource);
}

Font::Font(const FilePath& path, FontHint hinting)
	: mySource(FontManager::load(path, hinting))
{
}

void Font::cache() const
{
	FontManager::cache((FontEntry*)mySource);
}

Font& Font::operator = (Font&& font)
{
	if (mySource) FontManager::release((FontEntry*)mySource);
	mySource = font.mySource;
	font.mySource = nullptr;
	return *this;
}

Font& Font::operator = (const Font& font)
{
	if (mySource) FontManager::release((FontEntry*)mySource);
	mySource = font.mySource;
	if (mySource) FontManager::addReference((FontEntry*)mySource);
	return *this;
}

} // namespace AV