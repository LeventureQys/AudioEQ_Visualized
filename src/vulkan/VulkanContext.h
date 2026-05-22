#pragma once
#include <volk.h>
#include <QVector>

class VulkanContext {
public:
	VulkanContext();
	~VulkanContext();

	VulkanContext(const VulkanContext&) = delete;
	VulkanContext& operator=(const VulkanContext&) = delete;

	bool initialize();
	void cleanup();

	VkInstance instance() const;
	VkPhysicalDevice physicalDevice() const;
	VkDevice device() const;
	uint32_t graphicsQueueFamily() const;
	VkQueue graphicsQueue() const;
	VkSampleCountFlagBits msaaSamples() const;

	static bool isVulkanSupported();

private:
	VkInstance m_instance = VK_NULL_HANDLE;
	VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
	VkDevice m_device = VK_NULL_HANDLE;
	uint32_t m_graphicsQueueFamily = 0;
	VkQueue m_graphicsQueue = VK_NULL_HANDLE;
	VkSampleCountFlagBits m_msaaSamples = VK_SAMPLE_COUNT_1_BIT;

	bool createInstance();
	bool pickPhysicalDevice();
	bool createLogicalDevice();
	VkSampleCountFlagBits getMaxUsableSampleCount();

#ifdef _DEBUG
	bool enableValidationLayers = true;
#else
	bool enableValidationLayers = false;
#endif
};
