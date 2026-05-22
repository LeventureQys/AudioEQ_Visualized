#include "vulkan/VulkanSwapchain.h"
#include "vulkan/VulkanContext.h"
#include <algorithm>

VulkanSwapchain::VulkanSwapchain()
{
}

VulkanSwapchain::~VulkanSwapchain()
{
	cleanup();
}

bool VulkanSwapchain::initialize(VulkanContext* ctx, VkSurfaceKHR surface, QSize size, float devicePixelRatio)
{
	m_context = ctx;
	m_surface = surface;
	m_devicePixelRatio = devicePixelRatio;

	if (!createSwapchain(size)) {
		return false;
	}
	createImageViews();
	if (!createMSAA()) {
		return false;
	}
	if (!createRenderPass()) {
		return false;
	}
	createFramebuffers();
	return true;
}

void VulkanSwapchain::cleanup()
{
	cleanupSwapchainResources();
	m_context = nullptr;
	m_surface = VK_NULL_HANDLE;
	m_format = VK_FORMAT_B8G8R8A8_UNORM;
	m_extent = {};
	m_devicePixelRatio = 1.0f;
}

VkSwapchainKHR VulkanSwapchain::swapchain() const
{
	return m_swapchain;
}

VkFormat VulkanSwapchain::format() const
{
	return m_format;
}

VkExtent2D VulkanSwapchain::extent() const
{
	return m_extent;
}

VkRenderPass VulkanSwapchain::renderPass() const
{
	return m_renderPass;
}

uint32_t VulkanSwapchain::imageCount() const
{
	return static_cast<uint32_t>(m_images.size());
}

VkImageView VulkanSwapchain::imageView(uint32_t index) const
{
	if (index < static_cast<uint32_t>(m_imageViews.size())) {
		return m_imageViews[index];
	}
	return VK_NULL_HANDLE;
}

VkFramebuffer VulkanSwapchain::framebuffer(uint32_t index) const
{
	if (index < static_cast<uint32_t>(m_framebuffers.size())) {
		return m_framebuffers[index];
	}
	return VK_NULL_HANDLE;
}

VkSampleCountFlagBits VulkanSwapchain::msaaSamples() const
{
	return m_msaaSamples;
}

bool VulkanSwapchain::recreate(QSize newSize, float devicePixelRatio)
{
	m_devicePixelRatio = devicePixelRatio;

	VkDevice device = m_context ? m_context->device() : VK_NULL_HANDLE;
	if (device != VK_NULL_HANDLE) {
		vkDeviceWaitIdle(device);
	}

	cleanupSwapchainResources();

	if (!createSwapchain(newSize)) {
		return false;
	}
	createImageViews();
	if (!createMSAA()) {
		return false;
	}
	if (!createRenderPass()) {
		return false;
	}
	createFramebuffers();
	return true;
}

void VulkanSwapchain::cleanupSwapchainResources()
{
	VkDevice device = m_context ? m_context->device() : VK_NULL_HANDLE;
	if (device == VK_NULL_HANDLE) {
		return;
	}

	for (VkFramebuffer fb : m_framebuffers) {
		vkDestroyFramebuffer(device, fb, nullptr);
	}
	m_framebuffers.clear();

	if (m_renderPass != VK_NULL_HANDLE) {
		vkDestroyRenderPass(device, m_renderPass, nullptr);
		m_renderPass = VK_NULL_HANDLE;
	}

	if (m_msaaImageView != VK_NULL_HANDLE) {
		vkDestroyImageView(device, m_msaaImageView, nullptr);
		m_msaaImageView = VK_NULL_HANDLE;
	}
	if (m_msaaImage != VK_NULL_HANDLE) {
		vkDestroyImage(device, m_msaaImage, nullptr);
		m_msaaImage = VK_NULL_HANDLE;
	}
	if (m_msaaMemory != VK_NULL_HANDLE) {
		vkFreeMemory(device, m_msaaMemory, nullptr);
		m_msaaMemory = VK_NULL_HANDLE;
	}

	for (VkImageView view : m_imageViews) {
		vkDestroyImageView(device, view, nullptr);
	}
	m_imageViews.clear();
	m_images.clear();

	if (m_swapchain != VK_NULL_HANDLE) {
		vkDestroySwapchainKHR(device, m_swapchain, nullptr);
		m_swapchain = VK_NULL_HANDLE;
	}
}

bool VulkanSwapchain::createSwapchain(QSize size)
{
	VkPhysicalDevice physicalDevice = m_context->physicalDevice();
	VkSurfaceKHR surface = m_surface;

	VkSurfaceCapabilitiesKHR capabilities;
	if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities) != VK_SUCCESS) {
		return false;
	}

	uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
	if (formatCount == 0) {
		return false;
	}
	QVector<VkSurfaceFormatKHR> formats(formatCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats.data());

	uint32_t presentModeCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
	if (presentModeCount == 0) {
		return false;
	}
	QVector<VkPresentModeKHR> presentModes(presentModeCount);
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.data());

	VkSurfaceFormatKHR surfaceFormat = chooseSurfaceFormat(formats);
	VkPresentModeKHR presentMode = choosePresentMode(presentModes);
	VkExtent2D extent = chooseExtent(capabilities, size);

	m_format = surfaceFormat.format;
	m_extent = extent;

	uint32_t imageCount = capabilities.minImageCount + 1;
	if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
		imageCount = capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.preTransform = capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	VkDevice device = m_context->device();
	if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &m_swapchain) != VK_SUCCESS) {
		return false;
	}

	uint32_t count = 0;
	vkGetSwapchainImagesKHR(device, m_swapchain, &count, nullptr);
	m_images.resize(count);
	vkGetSwapchainImagesKHR(device, m_swapchain, &count, m_images.data());

	return true;
}

void VulkanSwapchain::createImageViews()
{
	VkDevice device = m_context->device();
	m_imageViews.resize(m_images.size());

	for (int i = 0; i < m_images.size(); ++i) {
		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = m_images[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = m_format;
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		vkCreateImageView(device, &createInfo, nullptr, &m_imageViews[i]);
	}
}

static uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
		if ((typeFilter & (1 << i)) &&
			(memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}
	return 0;
}

bool VulkanSwapchain::createMSAA()
{
	m_msaaSamples = m_context->msaaSamples();

	if (m_msaaSamples == VK_SAMPLE_COUNT_1_BIT) {
		return true;
	}

	VkDevice device = m_context->device();

	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = m_extent.width;
	imageInfo.extent.height = m_extent.height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = m_format;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = m_msaaSamples;
	imageInfo.flags = 0;

	if (vkCreateImage(device, &imageInfo, nullptr, &m_msaaImage) != VK_SUCCESS) {
		return false;
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(device, m_msaaImage, &memRequirements);

	uint32_t memoryTypeIndex = findMemoryType(m_context->physicalDevice(),
		memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = memoryTypeIndex;

	if (vkAllocateMemory(device, &allocInfo, nullptr, &m_msaaMemory) != VK_SUCCESS) {
		return false;
	}

	vkBindImageMemory(device, m_msaaImage, m_msaaMemory, 0);

	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = m_msaaImage;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = m_format;
	viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	if (vkCreateImageView(device, &viewInfo, nullptr, &m_msaaImageView) != VK_SUCCESS) {
		return false;
	}

	return true;
}

bool VulkanSwapchain::createRenderPass()
{
	VkDevice device = m_context->device();

	if (m_msaaSamples == VK_SAMPLE_COUNT_1_BIT) {
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = m_format;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;

		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &colorAttachment;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		return vkCreateRenderPass(device, &renderPassInfo, nullptr, &m_renderPass) == VK_SUCCESS;
	}

	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = m_format;
	colorAttachment.samples = m_msaaSamples;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription resolveAttachment{};
	resolveAttachment.format = m_format;
	resolveAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	resolveAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	resolveAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	resolveAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	resolveAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	resolveAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	resolveAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference resolveAttachmentRef{};
	resolveAttachmentRef.attachment = 1;
	resolveAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pResolveAttachments = &resolveAttachmentRef;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkAttachmentDescription attachments[] = { colorAttachment, resolveAttachment };

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 2;
	renderPassInfo.pAttachments = attachments;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	return vkCreateRenderPass(device, &renderPassInfo, nullptr, &m_renderPass) == VK_SUCCESS;
}

void VulkanSwapchain::createFramebuffers()
{
	VkDevice device = m_context->device();
	m_framebuffers.resize(m_imageViews.size());

	for (int i = 0; i < m_imageViews.size(); ++i) {
		QVector<VkImageView> attachments;
		if (m_msaaSamples != VK_SAMPLE_COUNT_1_BIT) {
			attachments.append(m_msaaImageView);
		}
		attachments.append(m_imageViews[i]);

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = m_renderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = m_extent.width;
		framebufferInfo.height = m_extent.height;
		framebufferInfo.layers = 1;

		vkCreateFramebuffer(device, &framebufferInfo, nullptr, &m_framebuffers[i]);
	}
}

VkSurfaceFormatKHR VulkanSwapchain::chooseSurfaceFormat(const QVector<VkSurfaceFormatKHR>& formats)
{
	for (const auto& fmt : formats) {
		if (fmt.format == VK_FORMAT_B8G8R8A8_SRGB &&
			fmt.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return fmt;
		}
	}
	return formats[0];
}

VkPresentModeKHR VulkanSwapchain::choosePresentMode(const QVector<VkPresentModeKHR>& modes)
{
	Q_UNUSED(modes);
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanSwapchain::chooseExtent(const VkSurfaceCapabilitiesKHR& capabilities, QSize size)
{
	uint32_t w = static_cast<uint32_t>(size.width() * m_devicePixelRatio);
	uint32_t h = static_cast<uint32_t>(size.height() * m_devicePixelRatio);

	w = qMax(capabilities.minImageExtent.width, qMin(w, capabilities.maxImageExtent.width));
	h = qMax(capabilities.minImageExtent.height, qMin(h, capabilities.maxImageExtent.height));

	VkExtent2D extent = { w, h };
	return extent;
}
