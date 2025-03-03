#pragma once

#include <Core/Graphics/Vulkan/Common.h>

namespace AV {
namespace Vk {

extern VkCommandPool transientCommandPool;
extern VkCommandBuffer commandBuffer;
extern VkQueue graphicsQueue;

bool initializePresenter();
void deinitializePresenter();

bool startFrame();
void finishFrame();

bool initializeDemo();
void drawDemo();

} // namespace Vk
} // namespace AV