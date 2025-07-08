#include <Core/QuadBatch.h>

#include <Core/Utils.h>
#include <Core/Draw.h>
#include <Core/Renderer.h>

#include <math.h>

namespace Vortex {

static int DrawScale = 256;

BatchSprite::BatchSprite() : BatchSprite(0, 0)
{
}

BatchSprite::BatchSprite(int w, int h) : width(w), height(h)
{
	float tmp[8] = {0, 0, 1, 0, 0, 1, 1, 1};
	memcpy(uvs, tmp, sizeof(float) * 8);
}

void BatchSprite::init(BatchSprite* spr, int count, int cols, int rows, int sprW, int sprH)
{
	float du = 1.f / (float)cols, dv = 1.f / (float)rows;
	for(int i = 0; i != count; ++i)
	{
		float u = (float)(i % cols) * du, v = (float)(i / cols) * dv;
		float uvs[8] = {u, v, u + du, v, u, v + dv, u + du, v + dv};
		memcpy(spr[i].uvs, uvs, sizeof(float) * 8);
		spr[i].width = sprW, spr[i].height = sprH;
	}
}

void BatchSprite::setScale(int scale8)
{
	DrawScale = scale8;
}

static void SwapUVs(float* v, int a, int b, int c, int d, int e, int f, int g, int h)
{
	float tmp[8] = {v[a], v[b], v[c], v[d], v[e], v[f], v[g], v[h]};
	memcpy(v, tmp, sizeof(float) * 8);
}

void BatchSprite::rotateUVs(Rotation r)
{
	if(r == ROT_LEFT_90)
		SwapUVs(uvs, 2, 3, 6, 7, 0, 1, 4, 5);
	else if(r == ROT_RIGHT_90)
		SwapUVs(uvs, 4, 5, 0, 1, 6, 7, 2, 3);
	else if(r == ROT_180)
		SwapUVs(uvs, 6, 7, 4, 5, 2, 3, 0, 1);
}

void BatchSprite::mirrorUVs(Mirror m)
{
	if(m == MIR_HORZ)
		SwapUVs(uvs, 2, 3, 0, 1, 6, 7, 4, 5);
	else if(m == MIR_VERT)
		SwapUVs(uvs, 4, 5, 6, 7, 0, 1, 2, 3);
	else if(m == MIR_BOTH)
		SwapUVs(uvs, 6, 7, 4, 5, 2, 3, 0, 1);
}

void BatchSprite::draw(QuadBatchT* out, int x, int y)
{
	int w = width * DrawScale / 512;
	int h = height * DrawScale / 512;
	int vp[8] = {x - w, y - h, x + w, y - h, x - w, y + h, x + w, y + h};
	
	out->push();
	memcpy(out->pos, vp, sizeof(int) * 8);
	memcpy(out->uvs, uvs, sizeof(float) * 8);
}

void BatchSprite::draw(QuadBatchT* out, int x, int y, int y2)
{
	int w = width * DrawScale / 512;
	int vp[8] = {x - w, y, x + w, y, x - w, y2, x + w, y2};

	out->push();
	memcpy(out->pos, vp, sizeof(int) * 8);
	memcpy(out->uvs, uvs, sizeof(float) * 8);
}

void BatchSprite::draw(QuadBatchTC* out, int x, int y, uchar alpha)
{
	draw(out, x, y, (color32)RGBAtoColor32(255, 255, 255, alpha));
}

void BatchSprite::draw(QuadBatchTC* out, int x, int y, color32 col)
{
	int w = width * DrawScale / 512;
	int h = height * DrawScale / 512;

	int vp[8] = {x - w, y - h, x + w, y - h, x - w, y + h, x + w, y + h};
	uint vc[4] = {col, col, col, col};

	out->push();
	memcpy(out->pos, vp, sizeof(int) * 8);
	memcpy(out->uvs, uvs, sizeof(float) * 8);
	memcpy(out->col, vc, sizeof(color32) * 4);
}

void BatchSprite::draw(QuadBatchTC* out, float x, float y, float rotation, float scale, uint color)
{
	float rsin = (float)sin(rotation);
	float rcos = (float)cos(rotation);

	float w = (float)(width * DrawScale / 512) * scale;
	float h = (float)(height * DrawScale / 512) * scale;
	const float xy[8] = {-w, -h, +w, -h, -w, +h, +w, +h};

	out->push();
	int* vp = out->pos;
	for(int i = 0, j = 1; i < 8; i += 2, j += 2)
	{
		vp[i] = (int)(x + xy[i] * rcos - xy[j] * rsin);
		vp[j] = (int)(y + xy[i] * rsin + xy[j] * rcos);
	}

	uint vc[4] = {color, color, color, color};
	memcpy(out->col, vc, sizeof(color32) * 4);
	memcpy(out->uvs, uvs, sizeof(float) * 8);
}

}; // namespace Vortex
