#pragma once

#include <Core/Utils/Enum.h>

namespace AV {
namespace Minimap {

enum class Mode { Notes, Density };

void initialize(const XmrDoc& settings);
void deinitialize();

} // namespace Minimap
} // namespace AV
