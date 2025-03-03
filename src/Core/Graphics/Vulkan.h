#pragma once

#include <Precomp.h>

namespace AV {
namespace Vulkan {

bool initialize(void* hInstance, void* hWnd);
void deinitialize();

bool startFrame();
void finishFrame();

bool initializeDemo();
void drawDemo();

} // namespace Vulkan
} // namespace AV
