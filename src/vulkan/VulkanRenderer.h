#pragma once
#include <volk.h>
#include <QObject>
#include <QColor>
#include <QVector>

class VulkanContext;
class VulkanSwapchain;
class VulkanPipeline;
class VulkanBufferPool;
class VulkanFontAtlas;
class VulkanFrameSync;

class VulkanRenderer : public QObject {
	Q_OBJECT
public:
	VulkanRenderer();
	~VulkanRenderer();
	VulkanRenderer(const VulkanRenderer&) = delete;
	VulkanRenderer& operator=(const VulkanRenderer&) = delete;

	bool initialize(VulkanContext* ctx, VulkanSwapchain* swapchain,
					VulkanPipeline* pipeline, VulkanBufferPool* bufferPool,
					VulkanFontAtlas* fontAtlas, VulkanFrameSync* frameSync);
	void cleanup();

	void updateGridVertices(const QVector<float>& vertices);
	void updateCurveVertices(const QVector<float>& vertices);
	void updateFillVertices(const QVector<float>& vertices);
	void updateGlyphVertices(const QVector<float>& vertices);

	void setCurveColor(const QColor& color);
	void setBackgroundColor(const QColor& color);

	VkCommandBuffer beginFrame(uint32_t* outImageIndex);
	void endFrame(uint32_t imageIndex, VkQueue presentQueue);

private:
	VulkanContext* m_context = nullptr;
	VulkanSwapchain* m_swapchain = nullptr;
	VulkanPipeline* m_pipeline = nullptr;
	VulkanBufferPool* m_bufferPool = nullptr;
	VulkanFontAtlas* m_fontAtlas = nullptr;
	VulkanFrameSync* m_frameSync = nullptr;

	VkCommandPool m_commandPool = VK_NULL_HANDLE;
	QVector<VkCommandBuffer> m_commandBuffers;

	struct VboResource {
		VkBuffer buffer = VK_NULL_HANDLE;
		VkDeviceMemory memory = VK_NULL_HANDLE;
		VkDeviceSize size = 0;
	};
	VboResource m_gridVbo, m_curveVbo, m_fillVbo, m_glyphVbo;

	QColor m_curveColor{0, 255, 0};
	QColor m_backgroundColor{26, 26, 26};

	bool createCommandPool();
	void recordCommandBuffer(VkCommandBuffer cmd, uint32_t imageIndex);
	void ensureVBO(VboResource& vbo, const QVector<float>& vertices,
				   VkBufferUsageFlags usage);
	void destroyVboResource(VboResource& vbo);
	void createVboResource(VboResource& vbo, VkDeviceSize size,
						   const void* data, VkBufferUsageFlags usage);
	static uint32_t findMemoryType(VkPhysicalDevice physicalDevice,
								   uint32_t typeFilter,
								   VkMemoryPropertyFlags properties);
};
