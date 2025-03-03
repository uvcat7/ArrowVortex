#pragma once

#include <Precomp.h>

#include <Core/Common/NonCopyable.h>

#include <source_location>

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

namespace AV {
namespace Vk {

// =====================================================================================================================
// Helper functions.

#define VkGuard(exp) (Vk::guardCheck(exp) && Vk::guardLog(#exp, std::source_location::current()))

bool guardCheck(VkResult);
bool guardLog(const char*, const std::source_location&);

const char* getResultString(VkResult result);

void logMissingProcedure(const char* name, bool required);

// =====================================================================================================================
// Helper structs.

struct SingleUseCommandBuffer
{
	SingleUseCommandBuffer();
	~SingleUseCommandBuffer();

	const VkCommandBuffer buffer;
};

struct Buffer : NonCopyable
{
	Buffer(VkBuffer buffer, VkDeviceMemory memory);
	~Buffer();

	static unique_ptr<Buffer> create(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);

	void copyTo(VkBuffer target, VkDeviceSize size, VkDeviceSize srcOffset = 0, VkDeviceSize dstOffset = 0);

	const VkBuffer buffer;
	const VkDeviceMemory memory;
};

struct Image : NonCopyable
{
	Image(VkFormat format, VkImage image, VkImageView imageView, VkDeviceMemory memory, VkDescriptorSet descriptor);
	~Image();

	static unique_ptr<Image> create(uint32_t width, uint32_t height, VkFormat format,
		VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
		VkDescriptorSet descriptor);

	bool transitionLayout(VkImageLayout oldLayout, VkImageLayout newLayout);
	void copyFromBuffer(VkBuffer buffer, uint32_t width, uint32_t height);

	const VkFormat format;
	const VkImage image;
	const VkImageView view;
	const VkDeviceMemory memory;
	const VkDescriptorSet descriptor;
};

struct Shader : NonCopyable
{
	Shader(VkShaderModule vertexModule, VkShaderModule fragmentModule);
	~Shader();

	static unique_ptr<Shader> create(const char* name);

	const VkShaderModule vertexModule;
	const VkShaderModule fragmentModule;
};

struct Pipeline : NonCopyable
{
	Pipeline(VkPipelineLayout layout, VkPipeline pipeline);
	~Pipeline();

	static unique_ptr<Pipeline> create(
		Shader* shader,
		VkDescriptorSetLayout uboLayout,
		VkDescriptorSetLayout texLayout,
		VkVertexInputBindingDescription bindingDescription,
		std::span<VkVertexInputAttributeDescription> attributeDescriptions,
		VkRenderPass renderPass);

	const VkPipelineLayout layout;
	const VkPipeline pipeline;
};

struct IndexedVertexBuffer
{
	IndexedVertexBuffer(VkBuffer vertexBuffer, VkBuffer indexBuffer, VkDeviceMemory memory);
	~IndexedVertexBuffer();

	static unique_ptr<IndexedVertexBuffer> create(VkDeviceSize vertexBufferSize, VkDeviceSize indexBufferSize);

	const VkBuffer vertexBuffer;
	const VkBuffer indexBuffer;
	const VkDeviceMemory memory;
};

} // namespace Vk
} // namespace AV
