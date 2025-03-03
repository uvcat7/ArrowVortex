#pragma once

#include <Core/Graphics/Texture.h>
#include <Core/Graphics/Vulkan/Common.h>

namespace AV {
namespace Vk {

// Manages loaded textures.
namespace TextureManager
{
	bool initialize();
	void deinitialize();

	unique_ptr<Vk::Image> create(int width, int height, PixelFormat format);
	void release(unique_ptr<Vk::Image>& descriptor);

	VkDescriptorSetLayout getLayout();

/*
	// Loads a texture from file.
	Entry* load(const FilePath& path, TextureFormat format, TextureFilter filter, Color transparentFill);
	
	// Creates a texture from a pixel buffer.
	Entry* load(const uchar* pixels, int width, int height, int stride,
		PixelFormat pixelFormat, TextureFormat format, TextureFilter filter);
	
	// Makes sure the texture entry stays loaded, even if reference count reaches zero.
	void cache(Entry* entry);
	
	// Increments the reference count of the given texture entry.
	void addReference(Entry* entry);
	
	// Decrements the reference count of the given texture entry.
	void release(Entry* entry);
	
	// Reloads all textures that are currently in use and have been loaded from file.
	void reloadAll();
	
	// Reports statistics on the current texture usage.
	void debugInfo();
	
	// Saves all textures that are currently in use to disk for debugging.
	void debugDump();
*/
};

} // namespace Vk
} // namespace AV
