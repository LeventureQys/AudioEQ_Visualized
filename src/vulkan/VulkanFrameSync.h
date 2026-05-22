#pragma once
#include <volk.h>
#include <QVector>

class VulkanContext;
class VulkanSwapchain;

class VulkanFrameSync {
public:
	static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

	VulkanFrameSync();
	~VulkanFrameSync();

	VulkanFrameSync(const VulkanFrameSync&) = delete;
	VulkanFrameSync& operator=(const VulkanFrameSync&) = delete;

	bool initialize(VulkanContext* ctx, VulkanSwapchain* swapchain);
	void cleanup();

	void beginFrame(uint32_t* imageIndex);
	void submitFrame(uint32_t imageIndex, VkCommandBuffer cmd, VkQueue presentQueue);

private:
	VulkanContext* m_context = nullptr;
	VulkanSwapchain* m_swapchain = nullptr;

	QVector<VkSemaphore> m_imageAvailableSemaphores;
	QVector<VkSemaphore> m_renderFinishedSemaphores;
	QVector<VkFence> m_inFlightFences;
	QVector<VkFence> m_imagesInFlight;
	int m_currentFrame = 0;
};
