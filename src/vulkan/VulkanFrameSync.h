#pragma once

#include "volk.h"
#include <vector>

class VulkanContext;

class VulkanFrameSync {
public:
    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

    explicit VulkanFrameSync(VulkanContext* ctx);
    ~VulkanFrameSync();

    bool create();
    void destroy();

    VkSemaphore imageAvailableSemaphore(int frameIndex) const;
    VkSemaphore renderFinishedSemaphore(int frameIndex) const;
    VkFence     inFlightFence(int frameIndex) const;

    void waitAndResetFence(int frameIndex);

    int currentFrame() const;
    void advanceFrame();

private:
    VulkanContext*              m_ctx;
    std::vector<VkSemaphore>    m_imageAvailable;
    std::vector<VkSemaphore>    m_renderFinished;
    std::vector<VkFence>        m_inFlightFences;
    int                         m_currentFrame = 0;
};
