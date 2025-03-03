#include <Precomp.h>

#include <bitset>
#include <cstddef>

#include <Core/Types/Vec2f.h>
#include <Core/Utils/Util.h>
#include <Core/Utils/String.h>

#include <Core/Graphics/Color.h>
#include <Core/Graphics/Texture.h>
#include <Core/Graphics/Vulkan/Device.h>
#include <Core/Graphics/Vulkan/Common.h>
#include <Core/Graphics/Vulkan/Presenter.h>
#include <Core/Graphics/Vulkan/TextureManager.h>

#include <Core/System/Log.h>
#include <Core/System/Window.h>
#include <Core/System/File.h>

namespace AV {

using namespace std;

// =====================================================================================================================
// Static data.

struct UniformData;

VkCommandPool Vk::transientCommandPool = nullptr;
VkCommandBuffer Vk::commandBuffer = nullptr;
VkQueue Vk::graphicsQueue = nullptr;

struct VulkanPresenterDemo
{
	VkDescriptorSetLayout uboLayout = nullptr;
	VkDescriptorSet uboDescriptorSet = nullptr;
	unique_ptr<Vk::Buffer> uboBuffer;
	UniformData* uboData = nullptr;
	unique_ptr<Vk::Shader> shader;
	unique_ptr<Vk::IndexedVertexBuffer> vertexBuffer;
	unique_ptr<Vk::Pipeline> pipeline;
	unique_ptr<Texture> texture;
	VkSampler sampler = nullptr;
};

namespace VulkanPresenter
{
	struct State
	{
		VkSwapchainKHR swapChain = nullptr;
		VkFormat swapChainFormat = VK_FORMAT_UNDEFINED;
		VkColorSpaceKHR swapChainColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
		vector<VkImageView> swapChainImages;
		VkRenderPass renderPass = nullptr;
		VkCommandPool commandPool = nullptr;
		VkDescriptorPool descriptorPool = nullptr;
		vector<VkFramebuffer> swapChainFramebuffers;
		optional<uint32_t> currentFrameIndex;
		VkExtent2D extent = {};
		VkSemaphore imageAvailable = nullptr;
		VkSemaphore renderFinished = nullptr;
		VkFence inFlight = nullptr;
		VulkanPresenterDemo demo;
	};
	static State* state = nullptr;
}
using VulkanPresenter::state;

// =====================================================================================================================
// Extensions.

#define PROC(name) static PFN_##name name = nullptr
#define GET_PROC(name, required) name = PFN_##name(GetDeviceProc(#name, required));

PROC(vkCreateSwapchainKHR);
PROC(vkDestroySwapchainKHR);
PROC(vkGetSwapchainImagesKHR);
PROC(vkAcquireNextImageKHR);
PROC(vkQueuePresentKHR);

static PFN_vkVoidFunction GetDeviceProc(const char* name, bool required)
{
	auto proc = vkGetDeviceProcAddr(Vk::device, name);
	if (!proc) Vk::logMissingProcedure(name, required);
	return proc;
}

static void InitializeDeviceProcedures()
{
	GET_PROC(vkCreateSwapchainKHR, true);
	GET_PROC(vkDestroySwapchainKHR, true);
	GET_PROC(vkGetSwapchainImagesKHR, true);
	GET_PROC(vkAcquireNextImageKHR, true);
	GET_PROC(vkQueuePresentKHR, true);
}

#undef PROC
#undef GET_DPROC

// =====================================================================================================================
// Swap chain creation.

static bool SetSwapChainFormat()
{
	uint32_t formatCount;

	if (VkGuard(vkGetPhysicalDeviceSurfaceFormatsKHR(Vk::physicalDevice, Vk::deviceSurface, &formatCount, nullptr)))
		return false;

	if (formatCount == 0)
	{
		Log::error("Vulkan: No surface formats found");
		return false;
	}

	vector<VkSurfaceFormatKHR> formats(formatCount);
	if (VkGuard(vkGetPhysicalDeviceSurfaceFormatsKHR(Vk::physicalDevice, Vk::deviceSurface, &formatCount, formats.data())))
		return false;

	state->swapChainFormat = (formatCount == 1 && formats[0].format == VK_FORMAT_UNDEFINED) ? VK_FORMAT_B8G8R8A8_UNORM : formats[0].format;
	state->swapChainColorSpace = formats[0].colorSpace;

	return true;
}

static bool CreateSwapChain(bool reportSuccess)
{
	VkSurfaceCapabilitiesKHR capabilities;
	if (VkGuard(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(Vk::physicalDevice, Vk::deviceSurface, &capabilities)))
		return false;

	auto extent = capabilities.currentExtent;
	if (extent.width == 0 && extent.height == 0)
		return false; // Occurs when window is minimized.

	if (extent.width == UINT32_MAX && extent.height == UINT32_MAX)
	{
		auto size = Window::getSize();
		extent.width = size.w;
		extent.height = size.h;
	}
	auto minExtent = capabilities.minImageExtent;
	auto maxExtent = capabilities.maxImageExtent;
	extent.width = clamp(extent.width, minExtent.width, maxExtent.width);
	extent.height = clamp(extent.height, minExtent.height, maxExtent.height);
	state->extent = extent;

	uint32_t targetImageCount = 2;
	if (capabilities.maxImageCount > 0 && targetImageCount > capabilities.maxImageCount)
		targetImageCount = capabilities.maxImageCount;

	VkSwapchainCreateInfoKHR createInfo {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = Vk::deviceSurface,
		.minImageCount = targetImageCount,
		.imageFormat = state->swapChainFormat,
		.imageColorSpace = state->swapChainColorSpace,
		.imageExtent = state->extent,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.preTransform = capabilities.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = VK_PRESENT_MODE_FIFO_KHR,
		.clipped = VK_TRUE,
	};

	if (VkGuard(vkCreateSwapchainKHR(Vk::device, &createInfo, nullptr, &state->swapChain)))
		return false;

	uint32_t imageCount;
	if (VkGuard(vkGetSwapchainImagesKHR(Vk::device, state->swapChain, &imageCount, nullptr)))
		return false;

	vector<VkImage> images(imageCount);
	if (VkGuard(vkGetSwapchainImagesKHR(Vk::device, state->swapChain, &imageCount, images.data())))
		return false;

	state->swapChainImages.resize(imageCount);
	for (size_t i = 0; i < imageCount; ++i)
	{
		VkImageViewCreateInfo createInfo {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = images[i],
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = state->swapChainFormat,
			.subresourceRange {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.levelCount = 1,
				.layerCount = 1,
			},
		};
		if (VkGuard(vkCreateImageView(Vk::device, &createInfo, nullptr, &state->swapChainImages[i])))
			return false;
	}

	if (reportSuccess)
		Log::info("Vulkan: Create swap chain :: OK");
	return true;
}

static bool CreateSwapChainFramebuffers()
{
	size_t numFrames = state->swapChainImages.size();
	state->swapChainFramebuffers.resize(numFrames);
	for (size_t i = 0; i < numFrames; ++i)
	{
		VkImageView attachments[] = { state->swapChainImages[i] };

		VkFramebufferCreateInfo fbInfo {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = state->renderPass,
			.attachmentCount = 1,
			.pAttachments = attachments,
			.width = state->extent.width,
			.height = state->extent.height,
			.layers = 1,
		};

		if (VkGuard(vkCreateFramebuffer(Vk::device, &fbInfo, nullptr, &state->swapChainFramebuffers[i])))
			return false;
	}

	return true;
}

static void DestroySwapChain()
{
	for (auto it : state->swapChainFramebuffers)
		vkDestroyFramebuffer(Vk::device, it, nullptr);

	for (auto it : state->swapChainImages)
		vkDestroyImageView(Vk::device, it, nullptr);

	if (state->swapChain)
		vkDestroySwapchainKHR(Vk::device, state->swapChain, nullptr);

	state->swapChainFramebuffers.clear();
	state->swapChainImages.clear();
	state->swapChain = nullptr;
	state->extent = {};
}

// =====================================================================================================================
// Render pass.

static bool CreateRenderPass()
{
	VkAttachmentDescription colorAttachment {
		.format = state->swapChainFormat,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
	};

	VkAttachmentReference colorAttachmentRef{
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	};

	VkSubpassDescription subpass {
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = 1,
		.pColorAttachments = &colorAttachmentRef,
	};

	VkSubpassDependency dependency {
		.srcSubpass = VK_SUBPASS_EXTERNAL,
		.dstSubpass = 0,
		.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.srcAccessMask = 0,
		.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
	};

	VkRenderPassCreateInfo renderPassInfo {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = 1,
		.pAttachments = &colorAttachment,
		.subpassCount = 1,
		.pSubpasses = &subpass,
		.dependencyCount = 1,
		.pDependencies = &dependency,
	};

	if (VkGuard(vkCreateRenderPass(Vk::device, &renderPassInfo, nullptr, &state->renderPass)))
		return false;

	return true;
}

// =====================================================================================================================
// Rendering buffers.

static bool CreateCommandBuffer()
{
	VkCommandPoolCreateInfo commandPoolInfo {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = Vk::deviceQueueFamily,
	};

	if (VkGuard(vkCreateCommandPool(Vk::device, &commandPoolInfo, nullptr, &state->commandPool)))
		return false;

	VkCommandPoolCreateInfo transientPoolInfo {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
		.queueFamilyIndex = Vk::deviceQueueFamily,
	};

	if (VkGuard(vkCreateCommandPool(Vk::device, &transientPoolInfo, nullptr, &Vk::transientCommandPool)))
		return false;

	VkCommandBufferAllocateInfo allocInfo {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = state->commandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1,
	};

	if (VkGuard(vkAllocateCommandBuffers(Vk::device, &allocInfo, &Vk::commandBuffer)))
		return false;

	VkDescriptorPoolSize descPoolSize {
		.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = 1,
	};

	VkDescriptorPoolCreateInfo descPoolInfo {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.maxSets = 1,
		.poolSizeCount = 1,
		.pPoolSizes = &descPoolSize,
	};

	if (VkGuard(vkCreateDescriptorPool(Vk::device, &descPoolInfo, nullptr, &state->descriptorPool)))
		return false;

	return true;
}

// =====================================================================================================================
// Swap chain synchronization.

static bool CreateSyncObjects()
{
	VkSemaphoreCreateInfo semaphoreInfo {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
	};

	VkFenceCreateInfo fenceInfo {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT,
	};

	bool failure = false;
	failure |= VkGuard(vkCreateSemaphore(Vk::device, &semaphoreInfo, nullptr, &state->imageAvailable));
	failure |= VkGuard(vkCreateSemaphore(Vk::device, &semaphoreInfo, nullptr, &state->renderFinished));
	failure |= VkGuard(vkCreateFence(Vk::device, &fenceInfo, nullptr, &state->inFlight));
	return !failure;
}

// =====================================================================================================================
// Demo pipeline.

struct UniformData
{
	Vec2f resolution;
};

struct ColorShaderVertex
{
	Vec2f pos;
	Color color;

	static VkVertexInputBindingDescription getBindingDescription()
	{
		VkVertexInputBindingDescription desc {
			.binding = 0,
			.stride = sizeof(ColorShaderVertex),
			.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
		};
		return desc;
	}

	static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 2> desc {
			VkVertexInputAttributeDescription {
				.location = 0,
				.binding = 0,
				.format = VK_FORMAT_R32G32_SFLOAT,
				.offset = offsetof(ColorShaderVertex, pos),
			},
			VkVertexInputAttributeDescription {
				.location = 1,
				.binding = 0,
				.format = VK_FORMAT_R8G8B8A8_UNORM,
				.offset = offsetof(ColorShaderVertex, color),
			},
		};

		return desc;
	}
};

struct TextureShaderVertex
{
	Vec2f pos;
	Vec2f uvs;

	static VkVertexInputBindingDescription getBindingDescription()
	{
		VkVertexInputBindingDescription desc {
			.binding = 0,
			.stride = sizeof(TextureShaderVertex),
			.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
		};
		return desc;
	}

	static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 2> desc
		{
			VkVertexInputAttributeDescription {
				.location = 0,
				.binding = 0,
				.format = VK_FORMAT_R32G32_SFLOAT,
				.offset = offsetof(TextureShaderVertex, pos),
			},
			VkVertexInputAttributeDescription {
				.location = 1,
				.binding = 0,
				.format = VK_FORMAT_R32G32_SFLOAT,
				.offset = offsetof(TextureShaderVertex, uvs),
			},
		};
		return desc;
	}
};

static bool CreateDemoVertexData()
{
	auto& demo = state->demo;

	demo.shader = Vk::Shader::create("texture");
	if (!demo.shader)
		return false;

	auto vertexSize = sizeof(TextureShaderVertex) * 4;
	auto indexSize = sizeof(uint16_t) * 6;

	demo.vertexBuffer = Vk::IndexedVertexBuffer::create(vertexSize, indexSize);
	if (!demo.vertexBuffer)
		return false;

	auto stagingBuffer = Vk::Buffer::create(
		vertexSize + indexSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	if (!stagingBuffer)
		return false;

	void* vertexData;
	vkMapMemory(Vk::device, stagingBuffer->memory, 0, vertexSize + indexSize, 0, &vertexData);
	auto vdata = (TextureShaderVertex*)vertexData;
	vdata[0].pos = { 10, 10 };
	vdata[1].pos = { 410, 10 };
	vdata[2].pos = { 410, 185 };
	vdata[3].pos = { 10, 185 };
	vdata[0].uvs = { 0, 0 };
	vdata[1].uvs = { 1, 0 };
	vdata[2].uvs = { 1, 1 };
	vdata[3].uvs = { 0, 1 };
	auto idata = (uint16_t*)(vdata + 4);
	idata[0] = 0;
	idata[1] = 1;
	idata[2] = 2;
	idata[3] = 2;
	idata[4] = 3;
	idata[5] = 0;
	vkUnmapMemory(Vk::device, stagingBuffer->memory);

	stagingBuffer->copyTo(demo.vertexBuffer->vertexBuffer, vertexSize);
	stagingBuffer->copyTo(demo.vertexBuffer->indexBuffer, indexSize, vertexSize, 0);

	demo.uboBuffer = Vk::Buffer::create(
		sizeof(UniformData),
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	if (!demo.uboBuffer)
		return false;

	void** pUboData = (void**)&demo.uboData;
	vkMapMemory(Vk::device, demo.uboBuffer->memory, 0, sizeof(UniformData), 0, pUboData);

	demo.texture = make_unique<Texture>(FilePath("assets/arrow vortex logo.png"));

	return true;
}

static bool CreateDemoPipeline()
{
	auto& demo = state->demo;

	VkSamplerCreateInfo samplerInfo {
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.magFilter = VK_FILTER_LINEAR,
		.minFilter = VK_FILTER_LINEAR,
		.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
	};

	if (VkGuard(vkCreateSampler(Vk::device, &samplerInfo, nullptr, &demo.sampler)))
		return false;

	VkDescriptorSetLayoutBinding uboLayoutBinding {
		.binding = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
	};
	VkDescriptorSetLayoutCreateInfo setLayoutInfo {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = 1,
		.pBindings = &uboLayoutBinding,
	};

	if (VkGuard(vkCreateDescriptorSetLayout(Vk::device, &setLayoutInfo, nullptr, &demo.uboLayout)))
		return false;

	VkDescriptorSetAllocateInfo descSetAllocInfo {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = state->descriptorPool,
		.descriptorSetCount = 1,
		.pSetLayouts = &demo.uboLayout,
	};

	if (VkGuard(vkAllocateDescriptorSets(Vk::device, &descSetAllocInfo, &demo.uboDescriptorSet)))
		return false;

	VkDescriptorBufferInfo descBufferInfo {
		.buffer = demo.uboBuffer->buffer,
		.offset = 0,
		.range = sizeof(UniformData),
	};
	VkWriteDescriptorSet descriptorWrite {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = demo.uboDescriptorSet,
		.dstBinding = 0,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.pBufferInfo = &descBufferInfo,
	};

	vkUpdateDescriptorSets(Vk::device, 1, &descriptorWrite, 0, nullptr);
	auto attr = TextureShaderVertex::getAttributeDescriptions();
	demo.pipeline = Vk::Pipeline::create(
		demo.shader.get(),
		demo.uboLayout,
		Vk::TextureManager::getLayout(),
		TextureShaderVertex::getBindingDescription(),
		{ attr.begin(), attr.end() },
		state->renderPass);

	if (!demo.pipeline)
		return false;

	return true;
}

// =====================================================================================================================
// Static data.

static bool RecreateSwapChain()
{
	vkDeviceWaitIdle(Vk::device);
	DestroySwapChain();
	return CreateSwapChain(false)
		&& CreateSwapChainFramebuffers();
}

bool Vk::initializePresenter()
{
	state = new VulkanPresenter::State();

	InitializeDeviceProcedures();

	vkGetDeviceQueue(Vk::device, Vk::deviceQueueFamily, 0, &Vk::graphicsQueue);

	return SetSwapChainFormat()
		&& CreateSwapChain(true)
		&& CreateCommandBuffer()
		&& CreateRenderPass()
		&& CreateSwapChainFramebuffers()
		&& CreateSyncObjects();
}

void Vk::deinitializePresenter()
{
	if (state->demo.uboLayout)
		vkDestroyDescriptorSetLayout(Vk::device, state->demo.uboLayout, nullptr);

	if (state->demo.sampler)
		vkDestroySampler(Vk::device, state->demo.sampler, nullptr);

	state->demo.pipeline.reset();
	state->demo.shader.reset();
	state->demo.vertexBuffer.reset();

	if (state->imageAvailable)
		vkDestroySemaphore(Vk::device, state->imageAvailable, nullptr);

	if (state->renderFinished)
		vkDestroySemaphore(Vk::device, state->renderFinished, nullptr);

	if (state->inFlight)
		vkDestroyFence(Vk::device, state->inFlight, nullptr);

	if (Vk::transientCommandPool)
		vkDestroyCommandPool(Vk::device, Vk::transientCommandPool, nullptr);

	if (state->descriptorPool)
		vkDestroyDescriptorPool(Vk::device, state->descriptorPool, nullptr);

	if (state->commandPool)
		vkDestroyCommandPool(Vk::device, state->commandPool, nullptr);

	if (state->renderPass)
		vkDestroyRenderPass(Vk::device, state->renderPass, nullptr);

	DestroySwapChain();

	Util::reset(state);
}

bool Vk::startFrame()
{
	vkWaitForFences(device, 1, &state->inFlight, VK_TRUE, UINT64_MAX);

	if (!state->swapChain && !RecreateSwapChain())
		return false;

	uint32_t imageIndex;
	auto result = vkAcquireNextImageKHR(device, state->swapChain, UINT64_MAX, state->imageAvailable, VK_NULL_HANDLE, &imageIndex);
	if ((result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) && !RecreateSwapChain())
		return false;

	if (VkGuard(result))
		return false;

	vkResetFences(device, 1, &state->inFlight);
	vkResetCommandBuffer(Vk::commandBuffer, 0);

	VkCommandBufferBeginInfo beginInfo {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	if (VkGuard(vkBeginCommandBuffer(Vk::commandBuffer, &beginInfo)))
		return false;

	VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };

	VkRenderPassBeginInfo renderPassInfo {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = state->renderPass,
		.framebuffer = state->swapChainFramebuffers[imageIndex],
		.renderArea = {
			.offset = { 0, 0 },
			.extent = state->extent,
		},
		.clearValueCount = 1,
		.pClearValues = &clearColor,
	};

	state->currentFrameIndex = imageIndex;

	vkCmdBeginRenderPass(Vk::commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	return true;
}

void Vk::finishFrame()
{
	if (!state->currentFrameIndex.has_value())
		return;

	vkCmdEndRenderPass(Vk::commandBuffer);
	if (VkGuard(vkEndCommandBuffer(Vk::commandBuffer)))
		return;

	VkSemaphore waitSemaphores[] = { state->imageAvailable };
	VkSemaphore signalSemaphores[] = { state->renderFinished };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	VkSubmitInfo submitInfo{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = waitSemaphores,
		.pWaitDstStageMask = waitStages,
		.commandBufferCount = 1,
		.pCommandBuffers = &Vk::commandBuffer,
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = signalSemaphores,
	};

	VkGuard(vkQueueSubmit(Vk::graphicsQueue, 1, &submitInfo, state->inFlight));

	uint32_t imageIndex = state->currentFrameIndex.value();
	VkSwapchainKHR swapChains[] = { state->swapChain };

	VkPresentInfoKHR presentInfo {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = signalSemaphores,
		.swapchainCount = 1,
		.pSwapchains = swapChains,
		.pImageIndices = &imageIndex,
	};

	auto result = vkQueuePresentKHR(Vk::graphicsQueue, &presentInfo);
	state->currentFrameIndex.reset();

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		RecreateSwapChain();
	}
	else
	{
		VkGuard(result);
	}
}

bool Vk::initializeDemo()
{
	return CreateDemoVertexData()
		&& CreateDemoPipeline();
}

void Vk::drawDemo()
{
	auto& demo = state->demo;

	demo.uboData->resolution =
	{
		(float)state->extent.width,
		(float)state->extent.height,
	};

	vkCmdBindPipeline(Vk::commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, demo.pipeline->pipeline);

	VkViewport viewport {
		.x = 0.0f,
		.y = 0.0f,
		.width = float(state->extent.width),
		.height = float(state->extent.height),
		.minDepth = 0.0f,
		.maxDepth = 1.0f,
	};
	vkCmdSetViewport(Vk::commandBuffer, 0, 1, &viewport);

	VkRect2D scissor {
		.offset = { 0, 0 },
		.extent = state->extent,
	};
	vkCmdSetScissor(Vk::commandBuffer, 0, 1, &scissor);

	VkBuffer vertexBuffers[] = { demo.vertexBuffer->vertexBuffer };
	VkDescriptorSet descriptorSets[] = { demo.uboDescriptorSet, demo.texture->image()->descriptor };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(Vk::commandBuffer, 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(Vk::commandBuffer, demo.vertexBuffer->indexBuffer, 0, VK_INDEX_TYPE_UINT16);
	vkCmdBindDescriptorSets(Vk::commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
		demo.pipeline->layout, 0, (uint32_t)size(descriptorSets), descriptorSets, 0, nullptr);

	vkCmdDrawIndexed(Vk::commandBuffer, 6, 1, 0, 0, 0);
}

} // namespace AV