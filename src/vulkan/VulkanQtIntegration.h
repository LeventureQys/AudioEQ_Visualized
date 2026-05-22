#pragma once
#include <QWidget>
#include <memory>

class VulkanContext;
class VulkanSwapchain;
class VulkanPipeline;
class VulkanBufferPool;
class VulkanFontAtlas;
class VulkanFrameSync;
class VulkanRenderer;

class VulkanQtIntegration {
public:
	VulkanQtIntegration();
	~VulkanQtIntegration();

	bool initialize(QWidget* parentWidget);
	void cleanup();

	VulkanContext* context() const;
	VulkanRenderer* renderer() const;
	VulkanSwapchain* swapchain() const;

private:
	std::unique_ptr<VulkanContext> m_context;
	std::unique_ptr<VulkanSwapchain> m_swapchain;
	std::unique_ptr<VulkanPipeline> m_pipeline;
	std::unique_ptr<VulkanBufferPool> m_bufferPool;
	std::unique_ptr<VulkanFontAtlas> m_fontAtlas;
	std::unique_ptr<VulkanFrameSync> m_frameSync;
	std::unique_ptr<VulkanRenderer> m_renderer;
	QWidget* m_widget = nullptr;
};
