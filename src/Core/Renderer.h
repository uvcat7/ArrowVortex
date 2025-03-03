#pragma once

#include <Core/Core.h>

namespace Vortex {

class Shader;

// Quad batch with integer vertices, and colors.
struct QuadBatchC
{
	void push(int numQuads = 1);
	void flush();

	int* pos;
	color32* col;
};

// Quad batch with vertices, and texture coordinates.
struct QuadBatchT
{
	void push(int numQuads = 1);
	void flush();

	int* pos;
	float* uvs;
};

// Quad batch with vertices, texture coordinates, and colors.
struct QuadBatchTC
{
	void push(int numQuads = 1);
	void flush();

	int* pos;
	float* uvs;
	color32* col;
};

namespace Renderer
{
	enum DefaultShader
	{
		SH_COLOR,
		SH_COLOR_HSV,
		SH_TEXTURE,
		SH_TEXTURE_ALPHA,
	};

	void create();
	void destroy();

	void startFrame();
	void endFrame();

	void bindTexture(TextureHandle texture);
	void unbindTexture();

	void bindShader(DefaultShader shader);

	void setColor(colorf color);
	void setColor(color32 color);
	void resetColor();

	void pushScissorRect(int x, int y, int w, int h);
	void pushScissorRect(const recti& r);
	void popScissorRect();

	void drawQuads(int numQuads, const int* pos);
	void drawQuads(int numQuads, const int* pos, const float* uvs);
	void drawQuads(int numQuads, const int* pos, const color32* col);
	void drawQuads(int numQuads, const int* pos, const float* uvs, const color32* col);
	void drawQuads(int numQuads, const float* pos, const float* uvs, const color32* col);

	void drawTris(int numTris, const uint* indices, const int* pos);
	void drawTris(int numTris, const uint* indices, const int* pos, const float* uvs);

	QuadBatchC batchC();
	QuadBatchT batchT();
	QuadBatchTC batchTC();

	const TileRect& getRoundedBox();
};

}; // namespace Vortex
