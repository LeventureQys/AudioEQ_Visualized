#include "VulkanSwapchain.h"
#include "VulkanContext.h"
#include <QDebug>
#include <algorithm>
#include <set>

VulkanSwapchain::VulkanSwapchain(VulkanContext* ctx) : m_ctx(ctx) {}

VulkanSwapchain::~VulkanSwapchain() { destroy(); }

VkSurfaceFormatKHR VulkanSwapchain::chooseSurfaceFormat(VkPhysicalDevice pd, VkSurfaceKHR surface) {
    uint32_t count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(pd, surface, &count, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(pd, surface, &count, formats.data());

    for (auto& f : formats) {
        if (f.format == VK_FORMAT_B8G8R8A8_SRGB && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return f;
    }
    return formats[0];
}

bool VulkanSwapchain::create(VkSurfaceKHR surface, QSize size) {
    m_surface = surface;
    VkPhysicalDevice pd = m_ctx->physicalDevice();

    auto surfaceFormat = chooseSurfaceFormat(pd, surface);
    m_format = surfaceFormat.format;

    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pd, surface, &caps);

    m_extent = {static_cast<uint32_t>(size.width()), static_cast<uint32_t>(size.height())};
    m_extent.width  = (std::max)(caps.minImageExtent.width,  (std::min)(m_extent.width,  caps.maxImageExtent.width));
    m_extent.height = (std::max)(caps.minImageExtent.height, (std::min)(m_extent.height, caps.maxImageExtent.height));

    uint32_t imageCount = (std::max)(2u, caps.minImageCount);
    if (caps.maxImageCount > 0) imageCount = (std::min)(imageCount, caps.maxImageCount);

    VkSwapchainCreateInfoKHR ci = {};
    ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    ci.surface = surface;
    ci.minImageCount = imageCount;
    ci.imageFormat = surfaceFormat.format;
    ci.imageColorSpace = surfaceFormat.colorSpace;
    ci.imageExtent = m_extent;
    ci.imageArrayLayers = 1;
    ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    ci.preTransform = caps.currentTransform;
    ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    ci.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    ci.clipped = VK_TRUE;

    if (vkCreateSwapchainKHR(m_ctx->device(), &ci, nullptr, &m_swapchain) != VK_SUCCESS) {
        qWarning("VulkanSwapchain: Failed to create swapchain");
        return false;
    }

    vkGetSwapchainImagesKHR(m_ctx->device(), m_swapchain, &imageCount, nullptr);
    m_images.resize(imageCount);
    vkGetSwapchainImagesKHR(m_ctx->device(), m_swapchain, &imageCount, m_images.data());

    m_imageViews.resize(imageCount);
    for (uint32_t i = 0; i < imageCount; ++i) {
        VkImageViewCreateInfo iv = {};
        iv.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        iv.image = m_images[i];
        iv.viewType = VK_IMAGE_VIEW_TYPE_2D;
        iv.format = m_format;
        iv.components = {VK_COMPONENT_SWIZZLE_IDENTITY};
        iv.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        vkCreateImageView(m_ctx->device(), &iv, nullptr, &m_imageViews[i]);
    }

    createMSAA();
    createRenderPass();
    createFramebuffers();

    qDebug() << "VulkanSwapchain: Created" << imageCount << "images," << m_extent.width << "x" << m_extent.height;
    return true;
}

void VulkanSwapchain::createMSAA() {
    VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_4_BIT;

    VkImageCreateInfo ici = {};
    ici.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ici.imageType = VK_IMAGE_TYPE_2D;
    ici.format = m_format;
    ici.extent = {m_extent.width, m_extent.height, 1};
    ici.mipLevels = 1;
    ici.arrayLayers = 1;
    ici.samples = sampleCount;
    ici.tiling = VK_IMAGE_TILING_OPTIMAL;
    ici.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    vkCreateImage(m_ctx->device(), &ici, nullptr, &m_msaaImage);

    VkMemoryRequirements memReq;
    vkGetImageMemoryRequirements(m_ctx->device(), m_msaaImage, &memReq);
    VkMemoryAllocateInfo mai = {};
    mai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mai.allocationSize = memReq.size;
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(m_ctx->physicalDevice(), &memProps);
    for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i) {
        if ((memReq.memoryTypeBits & (1<<i)) && (memProps.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
            mai.memoryTypeIndex = i;
            break;
        }
    }
    vkAllocateMemory(m_ctx->device(), &mai, nullptr, &m_msaaMemory);
    vkBindImageMemory(m_ctx->device(), m_msaaImage, m_msaaMemory, 0);

    VkImageViewCreateInfo iv = {};
    iv.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    iv.image = m_msaaImage;
    iv.viewType = VK_IMAGE_VIEW_TYPE_2D;
    iv.format = m_format;
    iv.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    vkCreateImageView(m_ctx->device(), &iv, nullptr, &m_msaaView);
}

void VulkanSwapchain::createRenderPass() {
    VkAttachmentDescription colorAttach = {};
    colorAttach.format = m_format;
    colorAttach.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttach.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttach.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttach.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttach.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription msaaAttach = {};
    msaaAttach.format = m_format;
    msaaAttach.samples = VK_SAMPLE_COUNT_4_BIT;
    msaaAttach.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    msaaAttach.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    msaaAttach.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    msaaAttach.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorRef = {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkAttachmentReference msaaRef  = {1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &msaaRef;
    subpass.pResolveAttachments = &colorRef;

    VkSubpassDependency dep = {};
    dep.srcSubpass = VK_SUBPASS_EXTERNAL;
    dep.dstSubpass = 0;
    dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.srcAccessMask = 0;
    dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    std::vector<VkAttachmentDescription> attachments = {colorAttach, msaaAttach};

    VkRenderPassCreateInfo rp = {};
    rp.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rp.attachmentCount = static_cast<uint32_t>(attachments.size());
    rp.pAttachments = attachments.data();
    rp.subpassCount = 1;
    rp.pSubpasses = &subpass;
    rp.dependencyCount = 1;
    rp.pDependencies = &dep;

    vkCreateRenderPass(m_ctx->device(), &rp, nullptr, &m_renderPass);
}

void VulkanSwapchain::createFramebuffers() {
    m_framebuffers.resize(m_imageViews.size());
    for (size_t i = 0; i < m_imageViews.size(); ++i) {
        std::vector<VkImageView> attachments = {m_imageViews[i], m_msaaView};
        VkFramebufferCreateInfo fb = {};
        fb.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fb.renderPass = m_renderPass;
        fb.attachmentCount = static_cast<uint32_t>(attachments.size());
        fb.pAttachments = attachments.data();
        fb.width = m_extent.width;
        fb.height = m_extent.height;
        fb.layers = 1;
        vkCreateFramebuffer(m_ctx->device(), &fb, nullptr, &m_framebuffers[i]);
    }
}

void VulkanSwapchain::destroy() {
    VkDevice dev = m_ctx ? m_ctx->device() : VK_NULL_HANDLE;
    if (!dev) return;

    for (auto fb : m_framebuffers) vkDestroyFramebuffer(dev, fb, nullptr);
    m_framebuffers.clear();
    if (m_renderPass) vkDestroyRenderPass(dev, m_renderPass, nullptr);
    if (m_msaaView) vkDestroyImageView(dev, m_msaaView, nullptr);
    if (m_msaaImage) vkDestroyImage(dev, m_msaaImage, nullptr);
    if (m_msaaMemory) vkFreeMemory(dev, m_msaaMemory, nullptr);
    for (auto iv : m_imageViews) vkDestroyImageView(dev, iv, nullptr);
    m_imageViews.clear();
    if (m_swapchain) vkDestroySwapchainKHR(dev, m_swapchain, nullptr);
    m_swapchain = VK_NULL_HANDLE;
}

bool VulkanSwapchain::resize(QSize newSize) {
    destroy();
    return create(m_surface, newSize);
}

bool VulkanSwapchain::acquireNextImage(VkSemaphore imageAvailable, uint32_t* outImageIndex) {
    return vkAcquireNextImageKHR(m_ctx->device(), m_swapchain, UINT64_MAX, imageAvailable, VK_NULL_HANDLE, outImageIndex) == VK_SUCCESS;
}

bool VulkanSwapchain::present(VkQueue queue, uint32_t imageIndex, VkSemaphore renderFinished) {
    VkPresentInfoKHR pi = {};
    pi.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    pi.waitSemaphoreCount = 1;
    pi.pWaitSemaphores = &renderFinished;
    pi.swapchainCount = 1;
    pi.pSwapchains = &m_swapchain;
    pi.pImageIndices = &imageIndex;
    return vkQueuePresentKHR(queue, &pi) == VK_SUCCESS;
}

VkFormat      VulkanSwapchain::format()      const { return m_format; }
VkExtent2D    VulkanSwapchain::extent()      const { return m_extent; }
uint32_t      VulkanSwapchain::imageCount()  const { return static_cast<uint32_t>(m_images.size()); }
VkImageView   VulkanSwapchain::imageView(int i) const { return (i>=0 && i<(int)m_imageViews.size()) ? m_imageViews[i] : VK_NULL_HANDLE; }
VkRenderPass  VulkanSwapchain::renderPass()  const { return m_renderPass; }
VkFramebuffer VulkanSwapchain::framebuffer(int i)const { return (i>=0 && i<(int)m_framebuffers.size())?m_framebuffers[i]:VK_NULL_HANDLE; }
