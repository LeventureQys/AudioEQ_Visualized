#include "vulkan/VulkanContext.h"
#include <vector>
#include <cstring>

VulkanContext::VulkanContext()
{
	volkInitialize();
}

VulkanContext::~VulkanContext()
{
	cleanup();
}

bool VulkanContext::initialize()
{
	if (!createInstance()) {
		return false;
	}
	if (!pickPhysicalDevice()) {
		return false;
	}
	if (!createLogicalDevice()) {
		return false;
	}
	return true;
}

void VulkanContext::cleanup()
{
	if (m_device != VK_NULL_HANDLE) {
		vkDestroyDevice(m_device, nullptr);
		m_device = VK_NULL_HANDLE;
	}
	if (m_instance != VK_NULL_HANDLE) {
		vkDestroyInstance(m_instance, nullptr);
		m_instance = VK_NULL_HANDLE;
	}
	m_physicalDevice = VK_NULL_HANDLE;
	m_graphicsQueue = VK_NULL_HANDLE;
	m_graphicsQueueFamily = 0;
	m_msaaSamples = VK_SAMPLE_COUNT_1_BIT;
}

VkInstance VulkanContext::instance() const
{
	return m_instance;
}

VkPhysicalDevice VulkanContext::physicalDevice() const
{
	return m_physicalDevice;
}

VkDevice VulkanContext::device() const
{
	return m_device;
}

uint32_t VulkanContext::graphicsQueueFamily() const
{
	return m_graphicsQueueFamily;
}

VkQueue VulkanContext::graphicsQueue() const
{
	return m_graphicsQueue;
}

VkSampleCountFlagBits VulkanContext::msaaSamples() const
{
	return m_msaaSamples;
}

bool VulkanContext::isVulkanSupported()
{
	VkResult result = volkInitialize();
	if (result != VK_SUCCESS) {
		return false;
	}

	if (vkEnumerateInstanceVersion) {
		uint32_t apiVersion;
		result = vkEnumerateInstanceVersion(&apiVersion);
		return result == VK_SUCCESS;
	}

	uint32_t extensionCount = 0;
	result = vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	return result == VK_SUCCESS;
}

bool VulkanContext::createInstance()
{
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "AudioEQ";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "AudioEQ";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_1;

	std::vector<const char*> extensions;
#ifdef VK_USE_PLATFORM_WIN32_KHR
	extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
	extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#endif
	if (enableValidationLayers) {
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	if (enableValidationLayers) {
		const char* validationLayers[] = { "VK_LAYER_KHRONOS_validation" };
		createInfo.enabledLayerCount = 1;
		createInfo.ppEnabledLayerNames = validationLayers;
	}

	VkResult result = vkCreateInstance(&createInfo, nullptr, &m_instance);
	if (result != VK_SUCCESS && enableValidationLayers) {
		createInfo.enabledLayerCount = 0;
		createInfo.ppEnabledLayerNames = nullptr;
		result = vkCreateInstance(&createInfo, nullptr, &m_instance);
	}
	if (result != VK_SUCCESS) {
		return false;
	}

	volkLoadInstance(m_instance);
	return true;
}

bool VulkanContext::pickPhysicalDevice()
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);
	if (deviceCount == 0) {
		return false;
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

	for (const auto& device : devices) {
		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(device, &properties);

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		uint32_t graphicsFamily = 0;
		bool foundGraphics = false;
		for (uint32_t i = 0; i < queueFamilyCount; ++i) {
			if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				graphicsFamily = i;
				foundGraphics = true;
				break;
			}
		}

		if (!foundGraphics) {
			continue;
		}

		m_physicalDevice = device;
		m_graphicsQueueFamily = graphicsFamily;

		if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
			return true;
		}
	}

	if (m_physicalDevice != VK_NULL_HANDLE) {
		return true;
	}

	return false;
}

bool VulkanContext::createLogicalDevice()
{
	m_msaaSamples = getMaxUsableSampleCount();

	float queuePriority = 1.0f;
	VkDeviceQueueCreateInfo queueCreateInfo{};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.queueFamilyIndex = m_graphicsQueueFamily;
	queueCreateInfo.queueCount = 1;
	queueCreateInfo.pQueuePriorities = &queuePriority;

	VkPhysicalDeviceFeatures deviceFeatures{};

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount = 1;
	createInfo.pQueueCreateInfos = &queueCreateInfo;
	createInfo.pEnabledFeatures = &deviceFeatures;

	VkResult result = vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device);
	if (result != VK_SUCCESS) {
		return false;
	}

	volkLoadDevice(m_device);
	vkGetDeviceQueue(m_device, m_graphicsQueueFamily, 0, &m_graphicsQueue);
	return true;
}

VkSampleCountFlagBits VulkanContext::getMaxUsableSampleCount()
{
	VkPhysicalDeviceProperties properties;
	vkGetPhysicalDeviceProperties(m_physicalDevice, &properties);

	VkSampleCountFlags counts = properties.limits.framebufferColorSampleCounts
		& properties.limits.framebufferDepthSampleCounts;

	if (counts & VK_SAMPLE_COUNT_64_BIT) return VK_SAMPLE_COUNT_64_BIT;
	if (counts & VK_SAMPLE_COUNT_32_BIT) return VK_SAMPLE_COUNT_32_BIT;
	if (counts & VK_SAMPLE_COUNT_16_BIT) return VK_SAMPLE_COUNT_16_BIT;
	if (counts & VK_SAMPLE_COUNT_8_BIT)  return VK_SAMPLE_COUNT_8_BIT;
	if (counts & VK_SAMPLE_COUNT_4_BIT)  return VK_SAMPLE_COUNT_4_BIT;
	if (counts & VK_SAMPLE_COUNT_2_BIT)  return VK_SAMPLE_COUNT_2_BIT;

	return VK_SAMPLE_COUNT_1_BIT;
}
