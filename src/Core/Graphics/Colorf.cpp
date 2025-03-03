#include <Precomp.h>

#include <Core/Graphics/Colorf.h>

namespace AV {

using namespace std;

Colorf::Colorf(HSVA hsva)
	: Colorf(hsva, hsva.alpha)
{
}

Colorf::Colorf(HSVA hsv, float a)
{
	float h = hsv.hue, s = hsv.saturation, v = hsv.value, r, g, b;
	float c = v * s; // Chroma
	float hprime = (float)fmod(h * 6.0f, 6);
	float x = c * (1 - (float)fabs(fmod(hprime, 2) - 1));
	float m = v - c;

	if (0 <= hprime && hprime < 1)
	{
		r = c;
		g = x;
		b = 0;
	}
	else if (1 <= hprime && hprime < 2)
	{
		r = x;
		g = c;
		b = 0;
	}
	else if (2 <= hprime && hprime < 3)
	{
		r = 0;
		g = c;
		b = x;
	}
	else if (3 <= hprime && hprime < 4)
	{
		r = 0;
		g = x;
		b = c;
	}
	else if (4 <= hprime && hprime < 5)
	{
		r = x;
		g = 0;
		b = c;
	}
	else if (5 <= hprime && hprime < 6)
	{
		r = c;
		g = 0;
		b = x;
	}
	else
	{
		r = 0;
		g = 0;
		b = 0;
	}

	r += m;
	g += m;
	b += m;

	red = r;
	green = g;
	blue = b;
	alpha = a;
}

Colorf::HSVA Colorf::toHSVA() const
{
	float r = red, g = green, b = blue, h, s, v;
	float cmax = max(max(r, g), b);
	float cmin = min(min(r, g), b);
	float delta = cmax - cmin;
	if (delta > 0)
	{
		if (cmax == r)
		{
			h = (1.0f / 6.0f) * ((float)fmod(((g - b) / delta), 6));
		}
		else if (cmax == g)
		{
			h = (1.0f / 6.0f) * (((b - r) / delta) + 2);
		}
		else if (cmax == b)
		{
			h = (1.0f / 6.0f) * (((r - g) / delta) + 4);
		}

		if (cmax > 0)
		{
			s = delta / cmax;
		}
		else
		{
			s = 0;
		}

		v = cmax;
	}
	else
	{
		h = 0;
		s = 0;
		v = cmax;
	}

	if (h < 0)
		h = 1.0f + h;

	return {h, s, v, alpha};
}

} // namespace AV