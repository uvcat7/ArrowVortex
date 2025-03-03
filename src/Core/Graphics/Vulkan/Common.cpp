#include <Precomp.h>

#include <Core/Graphics/Vulkan/Common.h>
#include <Core/Graphics/Vulkan/Device.h>
#include <Core/Graphics/Vulkan/Presenter.h>

#include <Core/System/Log.h>
#include <Core/System/File.h>
#include <Core/System/Debug.h>

namespace AV {

using namespace std;

// =====================================================================================================================
// Helper functions.

const char* Vk::getResultString(VkResult result)
{
	switch (result)
	{
	case VK_SUCCESS:
		return "Success";
	case VK_NOT_READY:
		return "A fence or query has not yet completed";
	case VK_TIMEOUT:
		return "A wait operation has not completed in the specified time";
	case VK_EVENT_SET:
		return "An event is signaled";
	case VK_EVENT_RESET:
		return "An event is unsignaled";
	case VK_INCOMPLETE:
		return "A return array was too small for the result";
	case VK_ERROR_OUT_OF_HOST_MEMORY:
		return "A host memory allocation has failed";
	case VK_ERROR_OUT_OF_DEVICE_MEMORY:
		return "A device memory allocation has failed";
	case VK_ERROR_INITIALIZATION_FAILED:
		return "Initialization of an object could not be completed for implementation-specific reasons";
	case VK_ERROR_DEVICE_LOST:
		return "The logical or physical device has been lost";
	case VK_ERROR_MEMORY_MAP_FAILED:
		return "Mapping of a memory object has failed";
	case VK_ERROR_LAYER_NOT_PRESENT:
		return "A requested layer is not present or could not be loaded";
	case VK_ERROR_EXTENSION_NOT_PRESENT:
		return "A requested extension is not supported";
	case VK_ERROR_FEATURE_NOT_PRESENT:
		return "A requested feature is not supported";
	case VK_ERROR_INCOMPATIBLE_DRIVER:
		return "The requested version of Vulkan is not supported by the driver or is otherwise incompatible";
	case VK_ERROR_TOO_MANY_OBJECTS:
		return "Too many objects of the type have already been created";
	case VK_ERROR_FORMAT_NOT_SUPPORTED:
		return "A requested format is not supported on this device";
	case VK_ERROR_FRAGMENTED_POOL:
		return "A pool allocation has failed due to fragmentation of the pool's memory";
	case VK_ERROR_UNKNOWN:
		return "An unknown vulkan error occurred";

	case VK_ERROR_OUT_OF_POOL_MEMORY:
		return "A pool memory allocation has failed.";
	case VK_ERROR_VALIDATION_FAILED_EXT:
		return "A validation layer found an error";
	case VK_ERROR_SURFACE_LOST_KHR:
		return "A surface is no longer available";
	case VK_SUBOPTIMAL_KHR:
		return "A swap chain no longer matches the surface properties exactly, but can still be used";
	case VK_ERROR_OUT_OF_DATE_KHR:
		return "A surface has changed in such a way that it is no longer compatible with the swap chain";
	case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
		return "The display used by a swapChain does not use the same presentable image layout";
	case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
		return "The requested window is already connected to a VkSurfaceKHR, or to some other non-Vulkan API";

	default:
		return "Unknown VkResult";
	}
}

bool Vk::guardCheck(VkResult result)
{
	if (result == VK_SUCCESS)
		return false;

	Log::blockBegin("Vulkan operation failed");
	Log::error(format("Result: {}", Vk::getResultString(result)));
	return true;
}

bool Vk::guardLog(const char* exp, const std::source_location& loc)
{
	auto locStr = Debug::getLocationString(loc);
	Log::error(format("Location: {}", locStr));
	Log::error(format("Expression: {}", exp));
	Log::blockEnd();
	return true;
}

void Vk::logMissingProcedure(const char* name, bool required)
{
	if (required)
		Log::error(format("Missing required Vulkan procedure: {}", name));
	else
		Log::warning(format("Missing optional Vulkan procedure: {}", name));
}

static uint32_t FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeMask, VkMemoryPropertyFlags flagsMask)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
	{
		if ((typeMask & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & flagsMask) == flagsMask)
			return i;
	}
	return 0;
}

// =====================================================================================================================
// SingleUseCommandBuffer.

static VkCommandBuffer CreateSingleUseCommandBuffer()
{
	VkCommandBufferAllocateInfo allocInfo {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = Vk::transientCommandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1,
	};

	VkCommandBuffer buffer;
	vkAllocateCommandBuffers(Vk::device, &allocInfo, &buffer);
	return buffer;
}

Vk::SingleUseCommandBuffer::SingleUseCommandBuffer()
	: buffer(CreateSingleUseCommandBuffer())
{
	VkCommandBufferBeginInfo beginInfo {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};
	vkBeginCommandBuffer(buffer, &beginInfo);
}

Vk::SingleUseCommandBuffer::~SingleUseCommandBuffer()
{
	vkEndCommandBuffer(buffer);

	VkSubmitInfo submitInfo{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &buffer,
	};

	vkQueueSubmit(Vk::graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(Vk::graphicsQueue);
	vkFreeCommandBuffers(Vk::device, Vk::transientCommandPool, 1, &buffer);
}

// =====================================================================================================================
// Buffer.

Vk::Buffer::Buffer(VkBuffer buffer, VkDeviceMemory memory)
	: buffer(buffer)
	, memory(memory)
{
}

Vk::Buffer::~Buffer()
{
	vkDestroyBuffer(Vk::device, buffer, nullptr);
	vkFreeMemory(Vk::device, memory, nullptr);
}

unique_ptr<Vk::Buffer> Vk::Buffer::create(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
{
	VkBufferCreateInfo createInfo {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = size,
		.usage = usage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
	};

	VkBuffer buffer;
	if (VkGuard(vkCreateBuffer(Vk::device, &createInfo, nullptr, &buffer)))
		return nullptr;

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(Vk::device, buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memRequirements.size,
		.memoryTypeIndex = FindMemoryType(
			physicalDevice,
			memRequirements.memoryTypeBits,
			properties),
	};
	VkDeviceMemory memory;
	if (VkGuard(vkAllocateMemory(Vk::device, &allocInfo, nullptr, &memory)))
	{
		vkDestroyBuffer(Vk::device, buffer, nullptr);
		return nullptr;
	}

	vkBindBufferMemory(Vk::device, buffer, memory, 0);
	return make_unique<Vk::Buffer>(buffer, memory);
}

void Vk::Buffer::copyTo(VkBuffer target, VkDeviceSize size, VkDeviceSize srcOffset, VkDeviceSize dstOffset)
{
	VkBufferCopy copyRegion {
		.srcOffset = srcOffset,
		.dstOffset = dstOffset,
		.size = size,
	};

	Vk::SingleUseCommandBuffer cmd;
	vkCmdCopyBuffer(cmd.buffer, buffer, target, 1, &copyRegion);
}

// =====================================================================================================================
// Image.

Vk::Image::Image(VkFormat format, VkImage image, VkImageView view, VkDeviceMemory memory, VkDescriptorSet descriptor)
	: format(format)
	, image(image)
	, view(view)
	, memory(memory)
	, descriptor(descriptor)
{
}

Vk::Image::~Image()
{
	vkDestroyImage(Vk::device, image, nullptr);
	vkDestroyImageView(Vk::device, view, nullptr);
	vkFreeMemory(Vk::device, memory, nullptr);
}

unique_ptr<Vk::Image> Vk::Image::create(
	uint32_t width,
	uint32_t height,
	VkFormat format,
	VkImageTiling tiling,
	VkImageUsageFlags usage,
	VkMemoryPropertyFlags properties,
	VkDescriptorSet descriptor)
{
	VkImageCreateInfo imageInfo {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = format,
		.extent = {.width = width, .height = height, .depth = 1 },
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
	};

	VkImage image;
	if (VkGuard(vkCreateImage(device, &imageInfo, nullptr, &image)))
		return nullptr;

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(device, image, &memRequirements);

	VkMemoryAllocateInfo allocInfo {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memRequirements.size,
		.memoryTypeIndex = FindMemoryType(
			physicalDevice,
			memRequirements.memoryTypeBits,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
	};

	VkDeviceMemory memory;
	if (VkGuard(vkAllocateMemory(Vk::device, &allocInfo, nullptr, &memory)))
	{
		vkDestroyImage(Vk::device, image, nullptr);
		return nullptr;
	}

	vkBindImageMemory(Vk::device, image, memory, 0);

	VkImageViewCreateInfo viewInfo {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image = image,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = format,
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1,
		},
	};

	VkImageView view;
	if (VkGuard(vkCreateImageView(device, &viewInfo, nullptr, &view)))
	{
		vkFreeMemory(Vk::device, memory, nullptr);
		vkDestroyImage(Vk::device, image, nullptr);
		return nullptr;
	}

	return make_unique<Vk::Image>(format, image, view, memory, descriptor);
}

bool Vk::Image::transitionLayout(VkImageLayout oldLayout, VkImageLayout newLayout)
{
	VkImageMemoryBarrier barrier{
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.oldLayout = oldLayout,
		.newLayout = newLayout,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = image,
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1,
		},
	};

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;
	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else
	{
		Log::error("Vulkan: Unsupported layout transition");
		return false;
	}

	Vk::SingleUseCommandBuffer cmd;
	vkCmdPipelineBarrier(
		cmd.buffer,
		sourceStage,
		destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier);

	return true;
}

void Vk::Image::copyFromBuffer(VkBuffer buffer, uint32_t width, uint32_t height)
{
	VkBufferImageCopy region{
		.imageSubresource = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.mipLevel = 0,
			.baseArrayLayer = 0,
			.layerCount = 1,
		},
		.imageOffset = { 0, 0, 0 },
		.imageExtent = { width, height, 1 },
	};

	Vk::SingleUseCommandBuffer cmd;
	vkCmdCopyBufferToImage(
		cmd.buffer,
		buffer,
		image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&region);
}

// =====================================================================================================================
// Shader.

Vk::Shader::Shader(VkShaderModule vertexModule, VkShaderModule fragmentModule)
	: vertexModule(vertexModule), fragmentModule(fragmentModule)
{
}

Vk::Shader::~Shader()
{
	vkDestroyShaderModule(Vk::device, vertexModule, nullptr);
	vkDestroyShaderModule(Vk::device, fragmentModule, nullptr);
}

VkShaderModule CreateShaderModule(const DirectoryPath& dir, const char* name, const char* ext)
{
	auto path = FilePath(dir, name, ext);

	string text;
	if (!FileSystem::readText(path, text))
		return nullptr;

	VkShaderModuleCreateInfo createInfo{
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = text.length(),
		.pCode = (const uint32_t*)text.data(),
	};

	VkShaderModule shaderModule;
	if (VkGuard(vkCreateShaderModule(Vk::device, &createInfo, nullptr, &shaderModule)))
		return nullptr;

	return shaderModule;
}

unique_ptr<Vk::Shader> Vk::Shader::create(const char* name)
{
	auto dir = DirectoryPath("shaders");
	auto vertexModule = CreateShaderModule(dir, name, "vert.spv");
	auto fragmentModule = CreateShaderModule(dir, name, "frag.spv");
	if (vertexModule == nullptr || fragmentModule == nullptr)
		return nullptr;

	return make_unique<Shader>(vertexModule, fragmentModule);
}

// =====================================================================================================================
// Pipeline.

Vk::Pipeline::Pipeline(VkPipelineLayout layout, VkPipeline pipeline)
	: layout(layout), pipeline(pipeline)
{
}

Vk::Pipeline::~Pipeline()
{
	vkDestroyPipeline(Vk::device, pipeline, nullptr);
	vkDestroyPipelineLayout(Vk::device, layout, nullptr);
}

unique_ptr<Vk::Pipeline> Vk::Pipeline::create(
	Vk::Shader* shader,
	VkDescriptorSetLayout uboLayout,
	VkDescriptorSetLayout texLayout,
	VkVertexInputBindingDescription bindingDescription,
	std::span<VkVertexInputAttributeDescription> attributeDescriptions,
	VkRenderPass renderPass)
{
	VkPipelineShaderStageCreateInfo shaderStages[2] = {
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_VERTEX_BIT,
			.module = shader->vertexModule,
			.pName = "main",
		},
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
			.module = shader->fragmentModule,
			.pName = "main",
		},
	};

	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	VkDynamicState dynamicStates[] =
	{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
	};
	dynamicState.dynamicStateCount = uint32_t(size(dynamicStates));
	dynamicState.pDynamicStates = dynamicStates;

	VkPipelineVertexInputStateCreateInfo vertexInputInfo {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount = 1,
		.pVertexBindingDescriptions = &bindingDescription,
		.vertexAttributeDescriptionCount = (uint32_t)attributeDescriptions.size(),
		.pVertexAttributeDescriptions = attributeDescriptions.data(),
	};

	VkPipelineInputAssemblyStateCreateInfo inputAssembly {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.primitiveRestartEnable = VK_FALSE,
	};

	VkPipelineViewportStateCreateInfo viewportState {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.scissorCount = 1,
};

	VkPipelineRasterizationStateCreateInfo rasterizer {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.cullMode = VK_CULL_MODE_BACK_BIT,
		.frontFace = VK_FRONT_FACE_CLOCKWISE,
		.lineWidth = 1.0f,
	};

	VkPipelineMultisampleStateCreateInfo multisampling {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
		.sampleShadingEnable = VK_FALSE,
	};

	VkPipelineColorBlendAttachmentState colorBlendAttachment {
		.blendEnable = VK_TRUE,
		.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
		.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		.colorBlendOp = VK_BLEND_OP_ADD,
		.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
		.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
		.alphaBlendOp = VK_BLEND_OP_ADD,
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
			| VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
	};

	VkPipelineColorBlendStateCreateInfo colorBlending {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.logicOpEnable = VK_FALSE,
		.attachmentCount = 1,
		.pAttachments = &colorBlendAttachment,
	};

	VkDescriptorSetLayout layouts[] = { uboLayout, texLayout };

	VkPipelineLayoutCreateInfo layoutInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = (uint32_t)size(layouts),
		.pSetLayouts = layouts,
	};

	VkPipelineLayout layout;
	if (VkGuard(vkCreatePipelineLayout(Vk::device, &layoutInfo, nullptr, &layout)))
		return {};

	VkGraphicsPipelineCreateInfo pipelineInfo {
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount = 2,
		.pStages = shaderStages,
		.pVertexInputState = &vertexInputInfo,
		.pInputAssemblyState = &inputAssembly,
		.pViewportState = &viewportState,
		.pRasterizationState = &rasterizer,
		.pMultisampleState = &multisampling,
		.pColorBlendState = &colorBlending,
		.pDynamicState = &dynamicState,
		.layout = layout,
		.renderPass = renderPass,
		.subpass = 0,
	};

	VkPipeline pipeline;
	if (VkGuard(vkCreateGraphicsPipelines(Vk::device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline)))
	{
		vkDestroyPipelineLayout(Vk::device, layout, nullptr);
		return nullptr;
	}

	return make_unique<Vk::Pipeline>(layout, pipeline);
}

// =====================================================================================================================
// IndexedVertexBuffer.

Vk::IndexedVertexBuffer::IndexedVertexBuffer(VkBuffer vertexBuffer, VkBuffer indexBuffer, VkDeviceMemory memory)
	: vertexBuffer(vertexBuffer)
	, indexBuffer(indexBuffer)
	, memory(memory)
{
}

Vk::IndexedVertexBuffer::~IndexedVertexBuffer()
{
	vkDestroyBuffer(Vk::device, vertexBuffer, nullptr);
	vkDestroyBuffer(Vk::device, indexBuffer, nullptr);
	vkFreeMemory(Vk::device, memory, nullptr);
}

unique_ptr<Vk::IndexedVertexBuffer> Vk::IndexedVertexBuffer::create(VkDeviceSize vertexBufferSize, VkDeviceSize indexBufferSize)
{
	VkBufferCreateInfo createInfo {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = vertexBufferSize,
		.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
	};

	VkBuffer vertexBuffer;
	if (VkGuard(vkCreateBuffer(Vk::device, &createInfo, nullptr, &vertexBuffer)))
		return nullptr;

	VkMemoryRequirements vertexMemRequirements;
	vkGetBufferMemoryRequirements(Vk::device, vertexBuffer, &vertexMemRequirements);

	createInfo.size = indexBufferSize;
	createInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

	VkBuffer indexBuffer;
	if (VkGuard(vkCreateBuffer(Vk::device, &createInfo, nullptr, &indexBuffer)))
	{
		vkDestroyBuffer(Vk::device, vertexBuffer, nullptr);
		return nullptr;
	}

	VkMemoryRequirements indexMemRequirements;
	vkGetBufferMemoryRequirements(Vk::device, indexBuffer, &indexMemRequirements);

	VortexAssert(vertexMemRequirements.memoryTypeBits == indexMemRequirements.memoryTypeBits);

	VkMemoryAllocateInfo allocInfo {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = vertexMemRequirements.size + indexMemRequirements.size,
		.memoryTypeIndex = FindMemoryType(
			physicalDevice,
			vertexMemRequirements.memoryTypeBits,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
	};
	VkDeviceMemory memory;
	if (VkGuard(vkAllocateMemory(Vk::device, &allocInfo, nullptr, &memory)))
	{
		vkDestroyBuffer(Vk::device, vertexBuffer, nullptr);
		vkDestroyBuffer(Vk::device, indexBuffer, nullptr);
		return nullptr;
	}

	vkBindBufferMemory(Vk::device, vertexBuffer, memory, 0);
	vkBindBufferMemory(Vk::device, indexBuffer, memory, vertexMemRequirements.size);
	return make_unique<Vk::IndexedVertexBuffer>(vertexBuffer, indexBuffer, memory);
}

} // namespace AV