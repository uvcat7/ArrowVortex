#include <Precomp.h>

#include <Core/System/Debug.h>
#include <Core/System/File.h>
#include <Core/System/Log.h>

#include <Core/Utils/Util.h>
#include <Core/Utils/String.h>
#include <Core/Utils/vector.h>
#include <Core/Utils/Map.h>

#include <Core/Graphics/Vulkan/TextureManager.h>
#include <Core/Graphics/Vulkan/Device.h>

namespace AV {

using namespace std;
using namespace Util;

static const char* FormatNames[(int)TextureFormat::Count] =
{
	"RGBA",
	"LUMA",
	"LUM",
	"ALPHA",
};

static const VkFormat VulkanFormats[(int)PixelFormat::Count] =
{
	VK_FORMAT_R8G8B8A8_UNORM,
	VK_FORMAT_R8G8B8_SRGB,
	VK_FORMAT_R8G8_SRGB,
	VK_FORMAT_R8_SRGB,
	VK_FORMAT_R8_SRGB,
};

/*

TextureManager::Entry::Entry(const uchar* pixels, int width, int height, int stride,
	PixelFormat pixelFormat, TextureFormat format, TextureFilter filter)
	: Texture(pixels, width, height, stride, pixelFormat, format, filter)
{
}

class PrivateTextureEntry : public TextureManager::Entry
{
public:
	PrivateTextureEntry(const uchar* pixels, int width, int height, int stride,
		PixelFormat pixelFormat, TextureFormat format, TextureFilter filter)
		: Entry(pixels, width, height, stride, pixelFormat, format, filter)
		, referenceCount(1)
		, isCached(false)
	{
	}
	int referenceCount;
	bool isCached;
};

struct TextureKey
{
	string path;
	TextureFilter filter;
	TextureFormat format;
};

bool operator < (const TextureKey& first, const TextureKey& second)
{
	int order = String::compare(first.path, second.path);
	if (order != 0) return order < 0;

	if (first.filter != second.filter)
		return first.filter < second.filter;

	return first.format < second.format;
}

*/

static VkSampler CreateSampler()
{
	VkSamplerCreateInfo samplerInfo	{
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.magFilter = VK_FILTER_LINEAR,
		.minFilter = VK_FILTER_LINEAR,
		.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
	};

	VkSampler result;
	if (VkGuard(vkCreateSampler(Vk::device, &samplerInfo, nullptr, &result)))
		return nullptr;

	return result;
}

static VkDescriptorSetLayout CreateLayout()
{
	VkDescriptorSetLayoutBinding binding {
		.binding = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
	};
	VkDescriptorSetLayoutCreateInfo info {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = 1,
		.pBindings = &binding,
	};

	VkDescriptorSetLayout result;
	if (VkGuard(vkCreateDescriptorSetLayout(Vk::device, &info, nullptr, &result)))
		return nullptr;

	return result;
}

// =====================================================================================================================
// Texture manager state.

namespace TextureManager
{
	struct State
	{
		VkSampler sampler = nullptr;
		VkDescriptorSetLayout layout = nullptr;
		vector<VkDescriptorPool> descriptorPools;
		vector<VkDescriptorSet> reusableDescriptors;
		uint32_t descriptorsLeftInCurrentPool = 0;
	};
	static State* state = nullptr;
}
using TextureManager::state;

// =====================================================================================================================
// Texture manager.

bool Vk::TextureManager::initialize()
{
	state = new AV::TextureManager::State();

	state->sampler = CreateSampler();
	state->layout = CreateLayout();

	return state->sampler && state->layout;
}

void Vk::TextureManager::deinitialize()
{
	for (auto it : state->descriptorPools)
		vkDestroyDescriptorPool(Vk::device, it, nullptr);

	if (state->layout)
		vkDestroyDescriptorSetLayout(Vk::device, state->layout, nullptr);

	if (state->sampler)
		vkDestroySampler(Vk::device, state->sampler, nullptr);

	Util::reset(state);
}

// =====================================================================================================================
// Descriptors.

static constexpr uint32_t PoolSize = 32;

static VkDescriptorPool GetAvailableDescriptorPool()
{
	if (state->descriptorsLeftInCurrentPool > 0)
		return state->descriptorPools.back();

	VkDescriptorPoolSize size {
		.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = PoolSize,
	};
	VkDescriptorPoolCreateInfo info {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.maxSets = PoolSize,
		.poolSizeCount = 1,
		.pPoolSizes = &size,
	};

	VkDescriptorPool handle;
	if (VkGuard(vkCreateDescriptorPool(Vk::device, &info, nullptr, &handle)))
		return nullptr;

	state->descriptorPools.push_back(handle);
	state->descriptorsLeftInCurrentPool = PoolSize;
	return handle;
}

static VkDescriptorSet GetAvailableDescriptorSet()
{
	if (!state->reusableDescriptors.empty())
	{
		auto result = state->reusableDescriptors.back();
		state->reusableDescriptors.pop_back();
		return result;
	}

	auto pool = GetAvailableDescriptorPool();
	if (!pool)
		return nullptr;

	VkDescriptorSetAllocateInfo info {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = pool,
		.descriptorSetCount = 1,
		.pSetLayouts = &state->layout,
	};

	VkDescriptorSet result;
	if (VkGuard(vkAllocateDescriptorSets(Vk::device, &info, &result)))
		return nullptr;

	state->descriptorsLeftInCurrentPool--;
	return result;
}

unique_ptr<Vk::Image> Vk::TextureManager::create(int width, int height, PixelFormat format)
{
	auto descriptor = GetAvailableDescriptorSet();
	if (!descriptor)
		return nullptr;

	auto image = Vk::Image::create(
		width,
		height,
		VulkanFormats[(int)format],
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		descriptor);

	if (!image)
	{
		state->reusableDescriptors.push_back(descriptor);
		return nullptr;
	}

	VkDescriptorImageInfo descInfo {
		.sampler = state->sampler,
		.imageView = image->view,
		.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
	};
	VkWriteDescriptorSet descWrite {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = image->descriptor,
		.dstBinding = 0,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.pImageInfo = &descInfo,
	};
	vkUpdateDescriptorSets(Vk::device, 1, &descWrite, 0, nullptr);

	return image;
}

void Vk::TextureManager::release(unique_ptr<Vk::Image>& image)
{
	if (image)
	{
		state->reusableDescriptors.push_back(image->descriptor);
		image.reset();
	}
}

VkDescriptorSetLayout Vk::TextureManager::getLayout()
{
	return state->layout;
}

/*

TextureManager::Entry* TextureManager::load(
	const FilePath& path,
	TextureFormat format,
	TextureFilter filter,
	Color transparentFill)
{
	if (!state)
	{
		VortexAssertf(false, "Texture manager must be created before a texture is created.");
		return nullptr;
	}

	// Check if the image is already loaded.
	TextureKey key = { path.str, filter, format };
	auto it = state->files.find(key);
	if (it != state->files.end())
	{
		auto* out = it->second;
		out->referenceCount++;
		return out;
	}

	// If not, try to load the image.
	auto pixelFormat = Texture::getPixelFormat(format);
	Image::Data image = Image::load(path, pixelFormat);
	if (image.pixels == nullptr)
		return nullptr;

	// Fill all fully transparent pixels with the given fill color.
	if (pixelFormat == PixelFormat::RGBA && transparentFill.alpha() == 0xFF)
	{
		const uchar r = transparentFill.red();
		const uchar g = transparentFill.green();
		const uchar b = transparentFill.blue();
		for (auto p = image.pixels, end = p + image.width * image.height * 4; p != end; p += 4)
		{
			if (p[3] == 0)
				p[0] = r, p[1] = g, p[2] = b;
		}
	}

	auto result = new PrivateTextureEntry(image.pixels, image.width, image.height, image.width,
		pixelFormat, format, filter);

	state->textures.push_back(result);
	state->files.emplace(key, result);
	Image::release(image.pixels);

	return result;
}

TextureManager::Entry* TextureManager::load(
	const uchar* pixels,
	int width,
	int height,
	int stride,
	PixelFormat pixelFormat,
	TextureFormat format,
	TextureFilter filtering)
{
	if (!state)
	{
		VortexAssertf(false, "Texture manager must be created before a texture is created.");
		return nullptr;
	}

	auto result = new PrivateTextureEntry(pixels, width, height, stride, pixelFormat, format, filtering);
	state->textures.push_back(result);

	return result;
}

void TextureManager::cache(Entry* _entry)
{
	auto entry = (PrivateTextureEntry*)_entry;
	entry->isCached = true;
}

void TextureManager::addReference(Entry* _entry)
{
	auto entry = (PrivateTextureEntry*)_entry;
	entry->referenceCount++;
}

void TextureManager::release(Entry* _entry)
{
	auto entry = (PrivateTextureEntry*)_entry;
	entry->referenceCount--;
	if (entry->referenceCount == 0 && !entry->isCached)
	{
		// Textures can still exist after shutdown, don't assume state is valid.
		if (state)
		{
			Map::eraseVals(state->files, entry);
			Vector::eraseValues(state->textures, entry);
		}
		else
		{
			VortexAssertf(false, "Lifetime of texture is longer than lifetime of texture manager.");
		}
		delete entry;
	}
}

void TextureManager::reloadAll()
{
	for (auto entry : state->textures)
	{
		auto key = Map::findKey(state->files, entry);
		if (!key) continue;

		auto pixelFormat = Texture::getPixelFormat(key->format);
		Image::Data image = Image::load(FilePath(key->path), pixelFormat);
		if (image.pixels)
		{
			int width = min(image.width, entry->width());
			int height = min(image.height, entry->height());
			entry->modify(image.pixels, 0, 0, width, height, image.width, pixelFormat);
			Image::release(image.pixels);
		}
	}
}

void TextureManager::debugInfo()
{
	Log::blockBegin("Loaded textures:");
	Log::info("amount: %i", state->textures.size());
	for (auto tex : state->textures)
	{
		auto key = Map::findKey(state->files, tex);
		string path = key ? key->path : "-none-";
		Log::lineBreak();
		Log::info("path   : %s", path.data());
		Log::info("size   : %i x %i", tex->width(), tex->height());
		Log::info("refs   : %i", tex->referenceCount);
		Log::info("cached : %i", tex->isCached);
		Log::info("format : %s", FormatNames[(int)tex->format()]);
	}
	Log::blockEnd();
}

#define ENABLE_DEBUG_DUMP

#ifdef ENABLE_DEBUG_DUMP
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "ImageWriter.h"
#endif

void TextureManager::debugDump()
{
#ifdef ENABLE_DEBUG_DUMP
	int counter = 0;
	for (auto tex : state->textures)
	{
		if (tex->format() != TextureFormat::RGBA) continue;

		auto pixelFormat = Texture::getPixelFormat(tex->format());
		int numChannels = Image::numChannels(pixelFormat);
		int bufferSize = tex->width() * tex->height() * numChannels;

		vector<uchar> pixels(bufferSize);
		tex->readPixels(pixels.data(), bufferSize);

		string outPath = "output/tex" + String::fromInt(counter) + ".png";
		stbi_write_png(outPath.data(), tex->width(), tex->height(), numChannels, pixels.data(), 0);

		++counter;
	}
#endif
}

*/

} // namespace AV
