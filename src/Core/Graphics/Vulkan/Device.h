#pragma once

#include <Core/Graphics/Vulkan/Common.h>

namespace AV {
namespace Vk {

extern VkInstance instance;
extern VkDevice device;
extern VkPhysicalDevice physicalDevice;
extern uint32_t deviceQueueFamily;
extern VkSurfaceKHR deviceSurface;

bool initializeDevice(void* hInstance, void* hWnd);
void deinitializeDevice();

} // namespace Vk
} // namespace AV
