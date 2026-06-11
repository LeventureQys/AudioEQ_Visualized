#pragma once

#include "volk.h"
#include <QSize>
#include <vector>

class VulkanContext;

class VulkanSwapchain {
public:
    explicit VulkanSwapchain(VulkanContext* ctx);
    ~VulkanSwapchain();

    bool create(VkSurfaceKHR surface, QSize size);
    void destroy();
    bool resize(QSize newSize);

    bool acquireNextImage(VkSemaphore imageAvailable, uint32_t* outImageIndex);
    bool present(VkQueue queue, uint32_t imageIndex, VkSemaphore renderFinished);

    VkFormat          format()         const;
    VkExtent2D        extent()         const;
    uint32_t          imageCount()     const;
    VkImageView       imageView(int i) const;
    VkRenderPass      renderPass()     const;
    VkFramebuffer     framebuffer(int i) const;

private:
    void createRenderPass();
    void createFramebuffers();
    void createMSAA();
    VkSurfaceFormatKHR chooseSurfaceFormat(VkPhysicalDevice pd, VkSurfaceKHR surface);

    VulkanContext*          m_ctx;
    VkSurfaceKHR            m_surface       = VK_NULL_HANDLE;
    VkSwapchainKHR          m_swapchain     = VK_NULL_HANDLE;
    VkRenderPass            m_renderPass    = VK_NULL_HANDLE;
    std::vector<VkImage>    m_images;
    std::vector<VkImageView> m_imageViews;
    std::vector<VkFramebuffer> m_framebuffers;
    VkFormat                m_format        = VK_FORMAT_UNDEFINED;
    VkExtent2D              m_extent        = {0, 0};

    VkImage                 m_msaaImage     = VK_NULL_HANDLE;
    VkDeviceMemory          m_msaaMemory    = VK_NULL_HANDLE;
    VkImageView             m_msaaView      = VK_NULL_HANDLE;
};
