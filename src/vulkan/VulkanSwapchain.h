#pragma once
#include <volk.h>
#include <QSize>
#include <QVector>

class VulkanContext;

class VulkanSwapchain {
public:
	VulkanSwapchain();
	~VulkanSwapchain();

	VulkanSwapchain(const VulkanSwapchain&) = delete;
	VulkanSwapchain& operator=(const VulkanSwapchain&) = delete;

	bool initialize(VulkanContext* ctx, VkSurfaceKHR surface, QSize size, float devicePixelRatio = 1.0f);
	void cleanup();

	VkSwapchainKHR swapchain() const;
	VkFormat format() const;
	VkExtent2D extent() const;
	VkRenderPass renderPass() const;
	uint32_t imageCount() const;
	VkImageView imageView(uint32_t index) const;
	VkFramebuffer framebuffer(uint32_t index) const;
	VkSampleCountFlagBits msaaSamples() const;

	bool recreate(QSize newSize, float devicePixelRatio = 1.0f);

	void setRenderPassForTest(VkRenderPass rp) { m_renderPass = rp; }
	void setExtentForTest(VkExtent2D ext) { m_extent = ext; }
	void setMsaaSamplesForTest(VkSampleCountFlagBits s) { m_msaaSamples = s; }

private:
	VulkanContext* m_context = nullptr;
	VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
	VkRenderPass m_renderPass = VK_NULL_HANDLE;
	VkFormat m_format = VK_FORMAT_B8G8R8A8_UNORM;

	QVector<VkImage> m_images;
	QVector<VkImageView> m_imageViews;
	QVector<VkFramebuffer> m_framebuffers;

	VkImage m_msaaImage = VK_NULL_HANDLE;
	VkDeviceMemory m_msaaMemory = VK_NULL_HANDLE;
	VkImageView m_msaaImageView = VK_NULL_HANDLE;
	VkSampleCountFlagBits m_msaaSamples = VK_SAMPLE_COUNT_1_BIT;

	VkExtent2D m_extent{};
	VkSurfaceKHR m_surface = VK_NULL_HANDLE;
	float m_devicePixelRatio = 1.0f;

	bool createSwapchain(QSize size);
	void createImageViews();
	bool createMSAA();
	bool createRenderPass();
	void createFramebuffers();
	void cleanupSwapchainResources();
	VkSurfaceFormatKHR chooseSurfaceFormat(const QVector<VkSurfaceFormatKHR>& formats);
	VkPresentModeKHR choosePresentMode(const QVector<VkPresentModeKHR>& modes);
	VkExtent2D chooseExtent(const VkSurfaceCapabilitiesKHR& capabilities, QSize size);
};
