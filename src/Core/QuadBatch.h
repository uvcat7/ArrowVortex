#pragma once

#include <Core/Core.h>
#include <Core/Vector.h>
#include <Core/Renderer.h>

namespace Vortex {

struct BatchSprite
{
	enum Rotation { ROT_LEFT_90, ROT_RIGHT_90, ROT_180 };
	enum Mirror { MIR_HORZ, MIR_VERT, MIR_BOTH };

	BatchSprite();
	BatchSprite(int w, int h);

	static void init(BatchSprite* spr, int count, int cols, int rows, int sprW, int sprH);
	static void setScale(int scale8); // 128 = 0.5x, 256 = 1.0x, 512 = 2.0x, etc.

	void rotateUVs(Rotation r);
	void mirrorUVs(Mirror m);

	void draw(QuadBatchT* batch, int x, int y);
	void draw(QuadBatchT* batch, int x, int y, int y2);

	void draw(QuadBatchTC* batch, int x, int y);
	void draw(QuadBatchTC* batch, int x, int y, uint8_t alpha);
	void draw(QuadBatchTC *batch, int x, int y, uint32_t color);
	void draw(QuadBatchTC* batch, int x, int y, int y2);
	void draw(QuadBatchTC *batch, int x, int y, int y2, uint8_t alpha);
	void draw(QuadBatchTC *batch, int x, int y, int y2, uint32_t color);

	void draw(QuadBatchTC* batch, float x, float y, float rotation, float scale = 1.f, uint32_t color = 0xFFFFFFFF);

	float uvs[8];
	int width, height;
};

}; // namespace Vortex
