#include <Precomp.h>

#include <bitset>

#include <Core/Utils/String.h>

#include <Core/Graphics/Vulkan.h>
#include <Core/Graphics/Vulkan/Device.h>
#include <Core/Graphics/Vulkan/Presenter.h>
#include <Core/Graphics/Vulkan/TextureManager.h>

#include <Core/System/Log.h>
#include <Core/System/Window.h>
#include <Core/System/File.h>

namespace AV {

using namespace std;

bool Vulkan::initialize(void* hInstance, void* hWnd)
{
	return Vk::initializeDevice(hInstance, hWnd)
		&& Vk::initializePresenter()
		&& Vk::TextureManager::initialize();
}

void Vulkan::deinitialize()
{
	vkDeviceWaitIdle(Vk::device);
	Vk::TextureManager::deinitialize();
	Vk::deinitializePresenter();
	Vk::deinitializeDevice();
}

bool Vulkan::startFrame()
{
	return Vk::startFrame();
}

void Vulkan::finishFrame()
{
	return Vk::finishFrame();
}

bool Vulkan::initializeDemo()
{
	return Vk::initializeDemo();
}

void Vulkan::drawDemo()
{
	Vk::drawDemo();
}

} // namespace AV