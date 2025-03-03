#include <Precomp.h>

#include <bitset>

#include <Core/Utils/Util.h>
#include <Core/Utils/String.h>

#include <Core/Graphics/Vulkan/Common.h>
#include <Core/Graphics/Vulkan/Device.h>

#include <Core/System/Log.h>
#include <Core/System/Window.h>
#include <Core/System/File.h>

namespace AV {

using namespace std;

#define VULKAN_ENABLE_VALIDATION
// #define VULKAN_LOG_EXTENSIONS
// #define VULKAN_DEBUG_LOGGING

// =====================================================================================================================
// Static data.

VkInstance Vk::instance = nullptr;
VkDevice Vk::device = nullptr;
VkPhysicalDevice Vk::physicalDevice = nullptr;
uint32_t Vk::deviceQueueFamily = 0;
VkSurfaceKHR Vk::deviceSurface = nullptr;

namespace VulkanDevice
{
	struct State
	{
		uint32_t optionalExtensionBits = 0;
		VkDebugUtilsMessengerEXT debugMessenger = nullptr;
		VkQueue graphicsQueue = nullptr;
	};
	static State* state = nullptr;
}
using VulkanDevice::state;

// =====================================================================================================================
// Extensions.

struct OptionalExtension { const char* name = nullptr; uint32_t bit = 0; };

static constexpr uint32_t DeviceExtensionsDriverPropertiesBit = 1 << 0;

static const char* RequiredInstanceExtensions[] =
{
	VK_KHR_SURFACE_EXTENSION_NAME,
	VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#ifdef VULKAN_ENABLE_VALIDATION
	VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
};

static const char* RequiredDeviceExtensions[] =
{
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

static OptionalExtension OptionalDeviceExtensions[] =
{
	{ VK_KHR_DRIVER_PROPERTIES_EXTENSION_NAME, DeviceExtensionsDriverPropertiesBit },
};

// =====================================================================================================================
// Procedures.

#define PROC(name) static PFN_##name name = nullptr
#define GET_PROC(name, required) name = PFN_##name(GetInstanceProc(#name, required));

PROC(vkCreateDebugUtilsMessengerEXT);
PROC(vkDestroyDebugUtilsMessengerEXT);

PROC(vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
PROC(vkGetPhysicalDeviceSurfaceFormatsKHR);
PROC(vkGetPhysicalDeviceSurfacePresentModesKHR);

static PFN_vkVoidFunction GetInstanceProc(const char* name, bool required)
{
	auto proc = vkGetInstanceProcAddr(Vk::instance, name);
	if (!proc) Vk::logMissingProcedure(name, required);
	return proc;
}

static void InitializeInstanceProcedures()
{
#ifdef VULKAN_ENABLE_VALIDATION
	GET_PROC(vkCreateDebugUtilsMessengerEXT, false);
	GET_PROC(vkDestroyDebugUtilsMessengerEXT, false);
#endif

	GET_PROC(vkGetPhysicalDeviceSurfaceCapabilitiesKHR, true);
	GET_PROC(vkGetPhysicalDeviceSurfaceFormatsKHR, true);
	GET_PROC(vkGetPhysicalDeviceSurfacePresentModesKHR, true);
}

#undef PROC
#undef GET_DPROC

// =====================================================================================================================
// Debugging and validation layers.

static const char* ValidationLayers[] =
{
	"VK_LAYER_KHRONOS_validation",
};

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT severity,
	VkDebugUtilsMessageTypeFlagsEXT type,
	const VkDebugUtilsMessengerCallbackDataEXT* data,
	void* userdata)
{
	if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
		Log::error(format("Vulkan validation: {}", data->pMessage));
	else if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		Log::warning(format("Vulkan validation: {}", data->pMessage));

	return VK_FALSE;
}

static VkDebugUtilsMessengerEXT CreateDebugMessenger()
{
	if (!vkCreateDebugUtilsMessengerEXT || !vkDestroyDebugUtilsMessengerEXT)
	{
		Log::warning("Vulkan: Debug messenger extensions unsupported");
		return nullptr;
	}

	VkDebugUtilsMessengerCreateInfoEXT createInfo {
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
		.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
		.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
		.pfnUserCallback = DebugCallback,
	};

	VkDebugUtilsMessengerEXT messenger;
	if (VkGuard(vkCreateDebugUtilsMessengerEXT(Vk::instance, &createInfo, nullptr, &messenger)))
		return nullptr;
	
	return messenger;
}

static void AddValidationLayers(VkInstanceCreateInfo& createInfo)
{
	uint32_t count = 0;
	if (VkGuard(vkEnumerateInstanceLayerProperties(&count, nullptr)))
		return;

	vector<VkLayerProperties> layers(count);
	if (VkGuard(vkEnumerateInstanceLayerProperties(&count, layers.data())))
		return;

	constexpr size_t numRequired = size(ValidationLayers);
	bitset<numRequired> found;
	for (auto& it : layers)
	{
		for (size_t i = 0; i < numRequired; ++i)
		{
			if (strcmp(it.layerName, ValidationLayers[i]) == 0)
				found.set(i);
		}
	}

	if (found.count() == numRequired)
	{
		createInfo.enabledLayerCount = uint32_t(size(ValidationLayers));
		createInfo.ppEnabledLayerNames = ValidationLayers;
		Log::info("Vulkan: Adding validation layers");
	}
	else
	{
		Log::info("Vulkan: Validation layers not supported");
	}
}

// =====================================================================================================================
// Instance creation.

static void LogInstanceExtensions()
{
	uint32_t count = 0;
	if (VkGuard(vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr)))
		return;

	vector<VkExtensionProperties> extensions(count);
	if (VkGuard(vkEnumerateInstanceExtensionProperties(nullptr, &count, extensions.data())))
		return;

	Log::lineBreak();
	Log::info(format("Vulkan: Found {} instance extension(s):", count));
	for (auto& it : extensions)
		Log::info(format(" {}", it.extensionName));
	Log::lineBreak();
}


static bool CreateInstance()
{
	VkApplicationInfo appInfo = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = "Arrow Vortex",
		.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
		.pEngineName = "No Engine",
		.engineVersion = VK_MAKE_VERSION(1, 0, 0),
		.apiVersion = VK_API_VERSION_1_3,
	};

	VkInstanceCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &appInfo,
		.enabledExtensionCount = uint32_t(size(RequiredInstanceExtensions)),
		.ppEnabledExtensionNames = RequiredInstanceExtensions,
	};

#ifdef VULKAN_LOG_EXTENSIONS
	LogInstanceExtensions();
#endif

#ifdef VULKAN_ENABLE_VALIDATION
	AddValidationLayers(createInfo);
#endif

	if (VkGuard(vkCreateInstance(&createInfo, nullptr, &Vk::instance)))
		return false;

	InitializeInstanceProcedures();

#ifdef VULKAN_ENABLE_VALIDATION
	state->debugMessenger = CreateDebugMessenger();
#endif

	Log::info("Vulkan: Create instance :: OK");
	return true;
}

// =====================================================================================================================
// Device creation.

enum class KnownVendorId
{
	Amd      = 0x1002,
	ImgTec   = 0x1010,
	Nvidia   = 0x10DE,
	Arm      = 0x13B5,
	Qualcomm = 0x5143,
	Intel    = 0x8086,
};

static const char* GetVendorFromId(uint32_t vendorId)
{
	switch ((KnownVendorId)vendorId)
	{
	case KnownVendorId::Amd:
		return "AMD";
	case KnownVendorId::ImgTec:
		return "ImgTec";
	case KnownVendorId::Nvidia:
		return "NVIDIA";
	case KnownVendorId::Arm:
		return "ARM";
	case KnownVendorId::Qualcomm:
		return "Qualcomm";
	case KnownVendorId::Intel:
		return "Intel";
	default:
		return "Unknown vendor";
	}
}

static const char* GetDeviceTypeString(VkPhysicalDeviceType deviceType)
{
	switch (deviceType)
	{
	case VK_PHYSICAL_DEVICE_TYPE_OTHER:
		return "Other";
	case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
		return "Integrated GPU";
	case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
		return "Discrete GPU";
	case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
		return "Virtual GPU";
	case VK_PHYSICAL_DEVICE_TYPE_CPU:
		return "CPU";
	default:
		return "Unknown VkPhysicalDeviceType";
	}
}

static bool CreateSurface(HINSTANCE hInstance, HWND hWnd)
{
	VkWin32SurfaceCreateInfoKHR createInfo = {
		.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
		.hinstance = hInstance,
		.hwnd = hWnd,
	};

	if (VkGuard(vkCreateWin32SurfaceKHR(Vk::instance, &createInfo, nullptr, &Vk::deviceSurface)))
		return false;

	Log::info("Vulkan: Create surface :: OK");
	return true;
}

struct RankedPhysicalDevice
{
	RankedPhysicalDevice(VkPhysicalDevice device)
		: device(device)
	{
		memset(&properties, 0, sizeof(properties));
		vkGetPhysicalDeviceProperties(device, &properties);
		details = GetDeviceTypeString(properties.deviceType);
	}
	void addDetail(const char* text)
	{
		details.append(", ");
		details.append(text);
	}
	VkPhysicalDevice device;
	VkPhysicalDeviceProperties properties;
	string details;
	uint32_t queueFamily = 0;
	uint32_t score = 0;
	uint32_t optionalExtensionBits = 0;
};

static void LogDeviceProperties(RankedPhysicalDevice& it)
{
#ifndef VULKAN_DEBUG_LOGGING
	Log::info(format("- {} ({})", it.properties.deviceName, it.details));
#else
	auto device = it.device;
	auto props = it.properties;

	Log::info("- Device name: %s", props.deviceName);
	Log::info("  Device type: %u (%s)", props.deviceType, GetDeviceTypeString(props.deviceType));
	Log::info("  Device id: %u", props.deviceID);
	Log::info("  Vendor id: %u (%s)", props.vendorID, GetVendorFromId(props.vendorID));
	Log::info("  API version: %u.%u.%u",
		VK_API_VERSION_MAJOR(props.apiVersion),
		VK_API_VERSION_MINOR(props.apiVersion),
		VK_API_VERSION_PATCH(props.apiVersion));

	uint32_t driver = props.driverVersion;
	switch ((KnownVendorId)props.vendorID)
	{
	case KnownVendorId::Nvidia:
		Log::info("  Driver version: %u (%u.%u.%u.%u)",
			driver,
			(driver >> 22) & 0x3ff,
			(driver >> 14) & 0xff,
			(driver >> 6) & 0xff,
			driver & 0x3f);
		break;
	case KnownVendorId::Intel:
		Log::info("  Driver version: %u (%u.%u)",
			driver,
			driver >> 14,
			driver & 0x3fff);
		break;
	default:
		Log::info("  Driver version: %u", driver);
		break;
	}

	if (Bits::all(it.optionalExtensionBits, DeviceExtensionsDriverPropertiesBit))
	{
		VkPhysicalDeviceDriverProperties driverProperties = {};
		driverProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES;

		VkPhysicalDeviceProperties2 deviceProperties2 = {};
		deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
		deviceProperties2.pNext = &driverProperties,
		driverProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES,
		vkGetPhysicalDeviceProperties2(device, &deviceProperties2),

		Log::info("  Driver id: %u", driverProperties.driverID);
		Log::info("  Driver info: %s", driverProperties.driverInfo);
		Log::info("  Driver name: %s", driverProperties.driverName);

		auto cv = driverProperties.conformanceVersion;
		Log::info("  Driver conformance version: %u.%u.%u.%u", cv.major, cv.minor, cv.patch, cv.subminor);
	}
#endif

#ifdef VULKAN_LOG_EXTENSIONS
	uint32_t count = 0;
	if (VkGuard(vkEnumerateDeviceExtensionProperties(it.device, nullptr, &count, nullptr)))
		return;

	vector<VkExtensionProperties> extensions(count);
	if (VkGuard(vkEnumerateDeviceExtensionProperties(it.device, nullptr, &count, extensions.data())))
		return;

	Log::info("  Device extensions:");
	for (auto& it : extensions)
		Log::info("   %s", it.extensionName);
	Log::lineBreak();
#endif

#ifdef VULKAN_DEBUG_LOGGING
	Log::lineBreak();
#endif
}

static bool AssignCompatibleQueueFamily(RankedPhysicalDevice& entry, VkSurfaceKHR surface)
{
	uint32_t count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(entry.device, &count, nullptr);

	vector<VkQueueFamilyProperties> families(count);
	vkGetPhysicalDeviceQueueFamilyProperties(entry.device, &count, families.data());

	for (uint32_t i = 0; i < count; ++i)
	{
		if ((families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0)
			continue;

		VkBool32 supportsSurface = VK_FALSE;
		auto res = vkGetPhysicalDeviceSurfaceSupportKHR(entry.device, i, surface, &supportsSurface);
		if (VkGuard(res) || supportsSurface == VK_FALSE)
			continue;

		entry.queueFamily = i;
		return true;
	}

	entry.addDetail("no compatible queue family");
	return false;
}

static bool CheckExtensionSupport(RankedPhysicalDevice& entry)
{
	uint32_t count;
	vkEnumerateDeviceExtensionProperties(entry.device, nullptr, &count, nullptr);

	vector<VkExtensionProperties> extensions(count);
	vkEnumerateDeviceExtensionProperties(entry.device, nullptr, &count, extensions.data());

	constexpr size_t numRequired = size(RequiredDeviceExtensions);
	constexpr size_t numOptional = size(OptionalDeviceExtensions);
	bitset<numRequired> found;
	for (auto& it : extensions)
	{
		for (size_t i = 0; i < numRequired; ++i)
		{
			if (strcmp(it.extensionName, RequiredDeviceExtensions[i]) == 0)
				found.set(i);
		}
		for (size_t i = 0; i < numOptional; ++i)
		{
			if (strcmp(it.extensionName, OptionalDeviceExtensions[i].name) == 0)
				entry.optionalExtensionBits |= OptionalDeviceExtensions[i].bit;
		}
	}

	if (found.count() == numRequired)
		return true;

	string message;
	for (size_t i = 0; i < numRequired; ++i)
	{
		if (found.test(i)) continue;
		if (!message.empty()) message.push_back(' ');
		message.append(RequiredDeviceExtensions[i]);
	}
	message.insert(0, "missing extensions: ");
	entry.addDetail(message.data());
	return false;
}

static vector<RankedPhysicalDevice> GetRankedPhysicalDevices(VkInstance instance, VkSurfaceKHR surface)
{
	vector<RankedPhysicalDevice> result;

	uint32_t count = 0;
	if (VkGuard(vkEnumeratePhysicalDevices(instance, &count, NULL)))
		return result;

	vector<VkPhysicalDevice> devices(count);
	if (VkGuard(vkEnumeratePhysicalDevices(instance, &count, devices.data())))
		return result;

	for (auto device : devices)
	{
		auto& entry = result.emplace_back(device);

		if (!AssignCompatibleQueueFamily(entry, surface))
			continue;

		if (!CheckExtensionSupport(entry))
			continue;

		entry.score += 1;

		if (entry.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			entry.score += 1;
	}

	stable_sort(result.begin(), result.end(),
		[](auto& a, auto& b) { return a.score > b.score; });

	return result;
}

static bool CreateDevice()
{
	auto phDevices = GetRankedPhysicalDevices(Vk::instance, Vk::deviceSurface);
	if (phDevices.empty())
	{
		Log::info("Vulkan: Did not find any physical devices");
		return false;
	}

	Log::info(format("Vulkan: Found {} physical device(s):", int(phDevices.size())));
	for (auto& it : phDevices)
		LogDeviceProperties(it);

	auto& phDevice = phDevices[0];
	Vk::physicalDevice = phDevice.device;
	Vk::deviceQueueFamily = phDevice.queueFamily;
	state->optionalExtensionBits = phDevice.optionalExtensionBits;

	float queuePriorities[] = { 1.0f };
	VkDeviceQueueCreateInfo queueInfo {
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.queueFamilyIndex = phDevice.queueFamily,
		.queueCount = 1,
		.pQueuePriorities = queuePriorities,
	};

	VkPhysicalDeviceFeatures deviceFeatures {};

	VkDeviceCreateInfo createInfo {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = &queueInfo,
		.enabledExtensionCount = uint32_t(size(RequiredDeviceExtensions)),
		.ppEnabledExtensionNames = RequiredDeviceExtensions,
		.pEnabledFeatures = &deviceFeatures,
	};

	if (VkGuard(vkCreateDevice(phDevice.device, &createInfo, nullptr, &Vk::device)))
		return false;

	Log::info("Vulkan: Create device :: OK");
	return true;
}

// =====================================================================================================================
// Vulkan.

bool Vk::initializeDevice(void* hInstance, void* hWnd)
{
	state = new VulkanDevice::State();

	return CreateInstance()
		&& CreateSurface((HINSTANCE)hInstance, (HWND)hWnd)
		&& CreateDevice();
}

void Vk::deinitializeDevice()
{
	if (Vk::device)
		vkDestroyDevice(Vk::device, nullptr);

	if (Vk::deviceSurface)
		vkDestroySurfaceKHR(Vk::instance, Vk::deviceSurface, nullptr);

	if (state->debugMessenger)
		vkDestroyDebugUtilsMessengerEXT(Vk::instance, state->debugMessenger, nullptr);

	if (Vk::instance)
		vkDestroyInstance(Vk::instance, nullptr);

	Util::reset(state);
}

} // namespace AV