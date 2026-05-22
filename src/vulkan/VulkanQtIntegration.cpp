#include "vulkan/VulkanQtIntegration.h"
#include "vulkan/VulkanContext.h"
#include "vulkan/VulkanSwapchain.h"
#include "vulkan/VulkanPipeline.h"
#include "vulkan/VulkanBufferPool.h"
#include "vulkan/VulkanFontAtlas.h"
#include "vulkan/VulkanFrameSync.h"
#include "vulkan/VulkanRenderer.h"

VulkanQtIntegration::VulkanQtIntegration() = default;

VulkanQtIntegration::~VulkanQtIntegration()
{
	cleanup();
}

bool VulkanQtIntegration::initialize(QWidget* parentWidget)
{
	m_widget = parentWidget;

	m_context = std::make_unique<VulkanContext>();
	if (!m_context->initialize()) return false;

	m_swapchain = std::make_unique<VulkanSwapchain>();

	m_pipeline = std::make_unique<VulkanPipeline>();

	m_bufferPool = std::make_unique<VulkanBufferPool>();
	if (!m_bufferPool->initialize(m_context.get())) return false;

	m_fontAtlas = std::make_unique<VulkanFontAtlas>();

	m_frameSync = std::make_unique<VulkanFrameSync>();

	m_renderer = std::make_unique<VulkanRenderer>();

	return true;
}

void VulkanQtIntegration::cleanup()
{
	if (m_renderer) { m_renderer->cleanup(); m_renderer.reset(); }
	if (m_frameSync) { m_frameSync->cleanup(); m_frameSync.reset(); }
	if (m_fontAtlas) { m_fontAtlas->cleanup(); m_fontAtlas.reset(); }
	if (m_bufferPool) { m_bufferPool->cleanup(); m_bufferPool.reset(); }
	if (m_pipeline) { m_pipeline->cleanup(); m_pipeline.reset(); }
	if (m_swapchain) { m_swapchain->cleanup(); m_swapchain.reset(); }
	if (m_context) { m_context->cleanup(); m_context.reset(); }
}

VulkanContext* VulkanQtIntegration::context() const
{
	return m_context.get();
}

VulkanRenderer* VulkanQtIntegration::renderer() const
{
	return m_renderer.get();
}

VulkanSwapchain* VulkanQtIntegration::swapchain() const
{
	return m_swapchain.get();
}
