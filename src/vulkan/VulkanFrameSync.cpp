#include "VulkanFrameSync.h"
#include "VulkanContext.h"

VulkanFrameSync::VulkanFrameSync(VulkanContext* ctx) : m_ctx(ctx) {}
VulkanFrameSync::~VulkanFrameSync() { destroy(); }

bool VulkanFrameSync::create() {
    VkDevice dev = m_ctx->device();

    VkSemaphoreCreateInfo sci = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    VkFenceCreateInfo fci = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fci.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        VkSemaphore sem;
        if (vkCreateSemaphore(dev, &sci, nullptr, &sem) != VK_SUCCESS) return false;
        m_imageAvailable.push_back(sem);

        if (vkCreateSemaphore(dev, &sci, nullptr, &sem) != VK_SUCCESS) return false;
        m_renderFinished.push_back(sem);

        VkFence fence;
        if (vkCreateFence(dev, &fci, nullptr, &fence) != VK_SUCCESS) return false;
        m_inFlightFences.push_back(fence);
    }
    return true;
}

void VulkanFrameSync::destroy() {
    VkDevice dev = m_ctx ? m_ctx->device() : VK_NULL_HANDLE;
    if (!dev) return;
    for (auto s : m_imageAvailable) vkDestroySemaphore(dev, s, nullptr);
    for (auto s : m_renderFinished) vkDestroySemaphore(dev, s, nullptr);
    for (auto f : m_inFlightFences) vkDestroyFence(dev, f, nullptr);
    m_imageAvailable.clear();
    m_renderFinished.clear();
    m_inFlightFences.clear();
}

VkSemaphore VulkanFrameSync::imageAvailableSemaphore(int i) const { return m_imageAvailable[i]; }
VkSemaphore VulkanFrameSync::renderFinishedSemaphore(int i) const { return m_renderFinished[i]; }
VkFence     VulkanFrameSync::inFlightFence(int i) const { return m_inFlightFences[i]; }

void VulkanFrameSync::waitAndResetFence(int i) {
    vkWaitForFences(m_ctx->device(), 1, &m_inFlightFences[i], VK_TRUE, UINT64_MAX);
    vkResetFences(m_ctx->device(), 1, &m_inFlightFences[i]);
}

int  VulkanFrameSync::currentFrame() const { return m_currentFrame; }
void VulkanFrameSync::advanceFrame() { m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT; }
