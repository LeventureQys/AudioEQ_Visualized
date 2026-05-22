#include "vulkan/VulkanFrameSync.h"
#include "vulkan/VulkanContext.h"
#include "vulkan/VulkanSwapchain.h"

VulkanFrameSync::VulkanFrameSync() = default;

VulkanFrameSync::~VulkanFrameSync()
{
	cleanup();
}

bool VulkanFrameSync::initialize(VulkanContext* ctx, VulkanSwapchain* swapchain)
{
	m_context = ctx;
	m_swapchain = swapchain;

	VkDevice device = m_context->device();
	uint32_t imageCount = m_swapchain->imageCount();

	m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
	m_imagesInFlight.resize(imageCount);
	for (uint32_t i = 0; i < imageCount; ++i) {
		m_imagesInFlight[i] = VK_NULL_HANDLE;
	}

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS) {
			return false;
		}
		if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS) {
			return false;
		}
		if (vkCreateFence(device, &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS) {
			return false;
		}
	}

	m_currentFrame = 0;
	return true;
}

void VulkanFrameSync::beginFrame(uint32_t* imageIndex)
{
	VkDevice device = m_context->device();

	vkWaitForFences(device, 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);

	vkAcquireNextImageKHR(device, m_swapchain->swapchain(), UINT64_MAX,
		m_imageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE, imageIndex);

	if (m_imagesInFlight[*imageIndex] != VK_NULL_HANDLE) {
		vkWaitForFences(device, 1, &m_imagesInFlight[*imageIndex], VK_TRUE, UINT64_MAX);
	}

	m_imagesInFlight[*imageIndex] = m_inFlightFences[m_currentFrame];
}

void VulkanFrameSync::submitFrame(uint32_t imageIndex, VkCommandBuffer cmd, VkQueue presentQueue)
{
	VkDevice device = m_context->device();
	VkQueue graphicsQueue = m_context->graphicsQueue();

	vkResetFences(device, 1, &m_inFlightFences[m_currentFrame]);

	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &m_imageAvailableSemaphores[m_currentFrame];
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &m_renderFinishedSemaphores[m_currentFrame];
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmd;
	vkQueueSubmit(graphicsQueue, 1, &submitInfo, m_inFlightFences[m_currentFrame]);

	VkSwapchainKHR swapchain = m_swapchain->swapchain();
	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &m_renderFinishedSemaphores[m_currentFrame];
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &swapchain;
	presentInfo.pImageIndices = &imageIndex;
	vkQueuePresentKHR(presentQueue, &presentInfo);

	m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VulkanFrameSync::cleanup()
{
	if (m_context && m_context->device() != VK_NULL_HANDLE) {
		vkDeviceWaitIdle(m_context->device());
	}

	VkDevice device = m_context ? m_context->device() : VK_NULL_HANDLE;
	if (device == VK_NULL_HANDLE)
		return;

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		if (m_imageAvailableSemaphores.size() > i && m_imageAvailableSemaphores[i] != VK_NULL_HANDLE) {
			vkDestroySemaphore(device, m_imageAvailableSemaphores[i], nullptr);
			m_imageAvailableSemaphores[i] = VK_NULL_HANDLE;
		}
		if (m_renderFinishedSemaphores.size() > i && m_renderFinishedSemaphores[i] != VK_NULL_HANDLE) {
			vkDestroySemaphore(device, m_renderFinishedSemaphores[i], nullptr);
			m_renderFinishedSemaphores[i] = VK_NULL_HANDLE;
		}
		if (m_inFlightFences.size() > i && m_inFlightFences[i] != VK_NULL_HANDLE) {
			vkDestroyFence(device, m_inFlightFences[i], nullptr);
			m_inFlightFences[i] = VK_NULL_HANDLE;
		}
	}
}
