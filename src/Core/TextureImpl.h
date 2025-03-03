#pragma once

#include <Core/Texture.h>

namespace Vortex {

struct Texture::Data
{
	~Data();
	Data(int w, int h, Texture::Format fmt, const uchar* pixels, bool mipmap);

	void clear();
	void modify(int x, int y, int w, int h, const uchar* pixels);
	void increaseHeight(int newHeight);
	void setFiltering(bool linear);
	void setWrapping(bool repeat);

	uint handle;
	int w, h;
	float rw, rh;
	int refs;
	Texture::Format fmt;
	bool mipmapped;
	bool cached;
};

struct TextureManager {

static void create();
static void destroy();

static Texture::Data* load(const char* path, Texture::Format fmt, bool mipmap);
static Texture::Data* load(int w, int h, Texture::Format fmt, bool mipmap, const uchar* pixels);

static void release(Texture::Data* tex);

};

}; // namespace Vortex
