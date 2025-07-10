#include <Core/Canvas.h>

#include <Core/Utils.h>

#include <math.h>

namespace Vortex {

// ================================================================================================
// Canvas helper functions.

namespace {

// Helper structs to make the blending algorithms simpler to implement.
struct RGB
{
	float r, g, b;
	RGB operator + (const RGB& o) const { return {r + o.r, g + o.g, b + o.b}; }
	RGB operator - (const RGB& o) const { return {r - o.r, g - o.g, b - o.b}; }
	RGB operator * (float v)      const { return {r * v, g * v, b * v}; }
};
struct RGBA { RGB c; float a; };

// Converts a 8-bit RGBA colorfto a float RGBA color.
static RGBA ToRGBA(const colorf& i)
{
	RGBA c = {{i.r, i.g, i.b}, i.a}; return c;
}

// Canvas blend functions.
typedef void(*BlendFunc)(RGBA& d, const RGBA& s, float a);

// Linear interpolation between destination and source.
static void BlendLinear(RGBA& d, const RGBA& s, float a)
{
	d.c = d.c + (s.c - d.c) * a, d.a = d.a + (s.a - d.a) * a;
}

// Alpha composition between destination and source.
static void BlendAlpha(RGBA& d, const RGBA& s, float a)
{
	float sa = s.a*a, oa = sa + d.a * (1 - sa);
	if(oa > 0) d.c = (d.c * (d.a * (1 - sa)) + s.c * sa) * (1 / oa), d.a = oa;
}

// Additive blending, adds source to destination.
static void BlendAdd(RGBA& d, const RGBA& s, float a)
{
	float sa = s.a * a, oa = sa + d.a * (1 - sa);
	if(oa > 0) d.c = (d.c * d.a + s.c * sa) * (1 / oa), d.a = oa;
}

// Canvas distance functions.
struct DistanceFunc
{
	virtual float Get(float px, float py) const = 0;
};

// Returns the distance from a point to a line segment.
struct GetLineDist : public DistanceFunc
{
	const float ax, ay, bx, by, size;

	GetLineDist(float ax, float ay, float bx, float by, float size)
		:ax(ax), ay(ay), bx(bx), by(by), size(size) {}

	float Get(float px, float py) const
	{
		float num = (px - ax)*(bx - ax) + (py - ay)*(by - ay);
		float den = (bx - ax)*(bx - ax) + (by - ay)*(by - ay);
		float rdn = 1 / den, r = num * rdn;
		float s = ((ay - py)*(bx - ax) - (ax - px)*(by - ay)) * rdn;
		if(r >= 0 && r <= 1) return (float)fabs(s * sqrt(den)) - size;

		float d1sq = (px - ax)*(px - ax) + (py - ay)*(py - ay);
		float d2sq = (px - bx)*(px - bx) + (py - by)*(py - by);
		return (float)sqrt(d1sq < d2sq ? d1sq : d2sq) - size;
	}
};

// Returns the distance from a point to a circle.
struct GetCircleDist : public DistanceFunc
{
	const float x, y, r;

	GetCircleDist(float x, float y, float r)
		:x(x), y(y), r(r) {}

	float Get(float px, float py) const
	{
		float dx = px - x, dy = py - y;
		return (float)sqrt(dx*dx + dy*dy) - r;
	}
};

// Returns the distance from a point to a rectangle with rounded corners.
struct GetRoundRectDist : public DistanceFunc
{
	const float x1, y1, x2, y2, r;

	GetRoundRectDist(float x1, float y1, float x2, float y2, float r)
		:x1(x1), y1(y1), x2(x2), y2(y2), r(r) {}

	float Get(float px, float py) const
	{
		float x = min(max(px, x1 + r), x2 - r);
		float y = min(max(py, y1 + r), y2 - r);
		if(x == px && y == py)
		{
			float dx = min(x - x1, x2 - x);
			float dy = min(y - y1, y2 - y);
			return -min(dx, dy);
		}
		else
		{
			float dx = px - x, dy = py - y;
			return (float)sqrt(dx*dx + dy*dy) - r;
		}
	}
};

// Returns the distance from a point to a generic polygon.
struct GetPolyDist : public DistanceFunc
{
	const float* x, *y;
	const int count;

	GetPolyDist(const float* x, const float* y, int count)
		:x(x), y(y), count(count) {}

	bool InsidePoly(float px, float py) const
	{
		int numIntersects = 0;
		for(int i = 0, j = count - 1; i < count; j = i, ++i)
			if((y[i] >= py) != (y[j] >= py) && px <= (x[j] - x[i])*(py - y[i]) / (y[j] - y[i]) + x[i])
				++numIntersects;
		return (numIntersects & 1);
	}

	float Get(float px, float py) const
	{
		float d = 1e10;
		for(int i = 0, j = count - 1; i < count; j = i, ++i)
		{
			GetLineDist ld(x[i], y[i], x[j], y[j], 0);
			d = min(d, ld.Get(px, py));
		}
		return InsidePoly(px, py) ? -d : d;
	}
};

}; // anonymous namespace

// ================================================================================================
// Canvas parameters.

struct Canvas::Data
{
	Data();
	void draw(float* buf, int w, int h, const areaf& area, DistanceFunc* func);

	areai mask;
	RGBA tl, tr, bl, br;
	float outerGlow, innerGlow, lineWidth;
	Canvas::BlendMode blendMode;
	bool outline;
};

Canvas::Data::Data()
{
	tl = tr = bl = br = ToRGBA({1, 1, 1, 1});
	outerGlow = 0.f, innerGlow = 0.f, lineWidth = 1;
	blendMode = Canvas::BM_ALPHA;
	outline = false;
}

void Canvas::Data::draw(float* buf, int w, int h, const areaf& area, DistanceFunc* func)
{
	BlendFunc blendfunc = BlendLinear;
	if(blendMode == Canvas::BM_ALPHA) blendfunc = BlendAlpha;
	if(blendMode == Canvas::BM_ADD) blendfunc = BlendAdd;

	int x1 = max(mask.l, (int)(area.l - outerGlow - 1 + 0.5f));
	int y1 = max(mask.t, (int)(area.t - outerGlow - 1 + 0.5f));
	int x2 = min(mask.r, (int)(area.r + outerGlow + 1 + 0.5f));
	int y2 = min(mask.b, (int)(area.b + outerGlow + 1 + 0.5f));

	float rh = 1.f / max(1, y2 - y1);
	float rw = 1.f / max(1, x2 - x1);
	float ig = 1.f / (innerGlow + 1);
	float og = 1.f / (outerGlow + 1);

	float yf = (float)y1 + 0.5f;
	for(int y = y1; y < y2; ++y, yf += 1)
	{
		RGBA l = tl; BlendLinear(l, bl, yf*rh);
		RGBA r = tr; BlendLinear(r, br, yf*rh);

		float xf = (float)x1 + 0.5f;
		for(int x = x1; x < x2; ++x, xf += 1)
		{
			float dist = func->Get(xf, yf);
			if(dist >= outerGlow + 1) continue;
			RGBA* dst = (RGBA*)(buf + ((y*w + x) * 4));
			RGBA src = l; BlendLinear(src, r, xf*rw);
			if(outline && dist < -0.5f)
			{
				float a = 1 - ig * ((-dist - lineWidth) + 0.5f);
				if(a > 0) blendfunc(*dst, src, min(a, 1.f));
			}
			else
			{
				float a = 1 - og * (dist + 0.5f);
				if(a > 0) blendfunc(*dst, src, min(a, 1.f));
			}
		}
	}
}

// ================================================================================================
// Canvas

Canvas::~Canvas()
{
	if(canvas_data_) free(canvas_data_);
	delete data_;
}

Canvas::Canvas()
	: canvas_data_(nullptr), canvas_width_(0), canvas_height_(0), data_(new Canvas::Data)
{
	data_->mask = {0, 0, canvas_width_, canvas_height_};
}

Canvas::Canvas(int w, int h, float lum)
	: canvas_data_(nullptr), canvas_width_(w), canvas_height_(h), data_(new Canvas::Data)
{
	canvas_data_ = (float*)malloc(w * h * 4 * sizeof(float));
	data_->mask = {0, 0, canvas_width_, canvas_height_};
	clear(lum);
}

Canvas::Canvas(const Canvas& other)
	: canvas_data_(nullptr), canvas_width_(0), canvas_height_(0), data_(new Canvas::Data)
{
	*this = other;
}

void Canvas::setMask(int l, int t, int r, int b)
{
	data_->mask = {max(0, l), max(0, t), min(r, canvas_width_), min(r, canvas_height_)};
}

void Canvas::setOutline(float size)
{
	data_->lineWidth = size;
}

void Canvas::setOuterGlow(float size)
{
	data_->outerGlow = size;
}

void Canvas::setInnerGlow(float size)
{
	data_->innerGlow = size;
}

void Canvas::setFill(bool enabled)
{
	data_->outline = !enabled;
}

void Canvas::setBlendMode(BlendMode bm)
{
	data_->blendMode = bm;
}

void Canvas::setColor(float lum, float alpha)
{
	colorf c = {lum, lum, lum, alpha};
	setColor(c, c, c, c);
}

void Canvas::setColor(const colorf& c)
{
	setColor(c, c, c, c);
}

void Canvas::setColor(const colorf& t, const colorf& b)
{
	setColor(t, t, b, b);
}

void Canvas::setColor(const colorf& tl, const colorf& tr, const colorf& bl, const colorf& br)
{
	data_->tl = ToRGBA(tl), data_->tr = ToRGBA(tr);
	data_->bl = ToRGBA(bl), data_->br = ToRGBA(br);
}

void Canvas::clear(float l)
{
	for(float* p = canvas_data_, *end = p + canvas_width_ * canvas_height_ * 4; p < end;)
	{
		*p = l; ++p;
		*p = l; ++p;
		*p = l; ++p;
		*p = 0; ++p;
	}
}

void Canvas::line(float x1, float y1, float x2, float y2, float width)
{
	float r = width * 0.5f;
	float xmin = min(x1, x2), xmax = max(x1, x2);
	float ymin = min(y1, y2), ymax = max(y1, y2);
	GetLineDist func(x1, y1, x2, y2, r);
	data_->draw(canvas_data_, canvas_width_, canvas_height_, {xmin - r, ymin - r, xmax + r, ymax + r}, &func);
}

void Canvas::circle(float x, float y, float r)
{
	GetCircleDist func(x, y, r);
	data_->draw(canvas_data_, canvas_width_, canvas_height_, {x - r, y - r, x + r, y + r}, &func);
}

void Canvas::box(float x1, float y1, float x2, float y2, float radius)
{
	float xl = min(x1, x2), xr = max(x1, x2);
	float yt = min(y1, y2), yb = max(y1, y2);
	float r = min(min(xr - xl, yb - yt)*0.5f, max(0.f, radius));
	GetRoundRectDist func(xl, yt, xr, yb, r);
	data_->draw(canvas_data_, canvas_width_, canvas_height_, {xl, yt, xr, yb}, &func);
}

void Canvas::box(int x1, int y1, int x2, int y2, float radius)
{
	box((float)x1, (float)y1, (float)x2, (float)y2, radius);
}

void Canvas::polygon(const float* x, const float* y, int vertexCount)
{
	if(vertexCount < 3) return;
	GetPolyDist func(x, y, vertexCount);
	data_->draw(canvas_data_, canvas_width_, canvas_height_, {0, 0, (float)canvas_width_, (float)canvas_height_}, &func);
}

Texture Canvas::createTexture(bool mipmap) const
{
	uchar* dst = (uchar*)malloc(canvas_width_*canvas_height_ * 4 * sizeof(uchar));
	for(int i = 0; i < canvas_width_ * canvas_height_ * 4; ++i)
	{
		int v = (int)(canvas_data_[i] * 255.f + 0.5f);
		dst[i] = min(max(v, 0), 255);
	}
	Texture result(canvas_width_, canvas_height_, dst, mipmap);
	free(dst);
	return result;
}

Canvas& Canvas::operator = (const Canvas& other)
{
	if(canvas_data_) free(canvas_data_);
	*data_ = *other.data_;
	canvas_width_ = other.canvas_width_, canvas_height_ = other.canvas_height_, canvas_data_ = nullptr;
	if(other.canvas_data_) canvas_data_ = (float*)malloc(canvas_width_ * canvas_height_ * 4 * sizeof(float));
	memcpy(canvas_data_, other.canvas_data_, canvas_width_ * canvas_height_ * 4 * sizeof(float));
	return *this;
}

}; // namespace Vortex
