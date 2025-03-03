#include <Precomp.h>

#include <stdarg.h>

#include <Core/Utils/String.h>
#include <Core/Utils/Enum.h>

#include <Core/System/Debug.h>
#include <Core/System/System.h>

#include <Core/Graphics/Draw.h>

#include <Vortex/Common.h>

#include <Vortex/View/TextOverlay.h>

namespace AV {

using namespace std;

// =====================================================================================================================
// Utility functions.

template <>
const char* Enum::toString<SnapType>(SnapType value)
{
	static const char* text[] =
	{
		"None", "4th", "8th", "12th", "16th", "20th", "24th", "32nd", "48th", "64th", "96th", "192nd"
	};
	size_t i = (size_t)value;
	return (i >= 0 && i < size(text)) ? text[i] : "unknown";
}

Quantization ToQuantization(Row rowIndex)
{
	static Quantization map[192] = {};
	static bool init = false;
	if (!init)
	{
		init = true;
		int mod[8] = {48, 24, 16, 12, 8, 6, 4, 3};
		for (int i = 0; i < 192; ++i)
		{
			for (int j = 0; j < 8 && i % mod[j] != 0; ++j)
			{
				map[i] = (Quantization)((int)map[i] + 1);
			}
		}
	}
	return map[rowIndex % 192];
}

} // namespace AV
