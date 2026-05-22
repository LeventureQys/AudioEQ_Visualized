#include "vulkan/VulkanRenderer.h"
#include "vulkan/VulkanContext.h"
#include "vulkan/VulkanSwapchain.h"
#include "vulkan/VulkanPipeline.h"
#include "vulkan/VulkanBufferPool.h"
#include "vulkan/VulkanFontAtlas.h"
#include "vulkan/VulkanFrameSync.h"
#include <cstring>
#include <cmath>

VulkanRenderer::VulkanRenderer() = default;

VulkanRenderer::~VulkanRenderer()
{
	cleanup();
}

bool VulkanRenderer::initialize(VulkanContext* ctx, VulkanSwapchain* swapchain,
								VulkanPipeline* pipeline, VulkanBufferPool* bufferPool,
								VulkanFontAtlas* fontAtlas, VulkanFrameSync* frameSync)
{
	if (!ctx || !swapchain || !pipeline || !bufferPool || !fontAtlas || !frameSync) {
		return false;
	}

	m_context = ctx;
	m_swapchain = swapchain;
	m_pipeline = pipeline;
	m_bufferPool = bufferPool;
	m_fontAtlas = fontAtlas;
	m_frameSync = frameSync;

	if (!createCommandPool()) {
		cleanup();
		return false;
	}

	uint32_t imageCount = m_swapchain->imageCount();
	m_commandBuffers.resize(imageCount);

	if (imageCount > 0) {
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = m_commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = imageCount;

		if (vkAllocateCommandBuffers(m_context->device(), &allocInfo,
									  m_commandBuffers.data()) != VK_SUCCESS) {
			cleanup();
			return false;
		}
	}

	return true;
}

void VulkanRenderer::cleanup()
{
	destroyVboResource(m_gridVbo);
	destroyVboResource(m_curveVbo);
	destroyVboResource(m_fillVbo);
	destroyVboResource(m_glyphVbo);

	if (m_context && m_context->device() != VK_NULL_HANDLE) {
		if (m_commandPool != VK_NULL_HANDLE) {
			vkDestroyCommandPool(m_context->device(), m_commandPool, nullptr);
			m_commandPool = VK_NULL_HANDLE;
		}
	}

	m_commandBuffers.clear();
	m_context = nullptr;
	m_swapchain = nullptr;
	m_pipeline = nullptr;
	m_bufferPool = nullptr;
	m_fontAtlas = nullptr;
	m_frameSync = nullptr;
}

bool VulkanRenderer::createCommandPool()
{
	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = m_context->graphicsQueueFamily();

	return vkCreateCommandPool(m_context->device(), &poolInfo, nullptr,
							   &m_commandPool) == VK_SUCCESS;
}

void VulkanRenderer::recordCommandBuffer(VkCommandBuffer cmd, uint32_t imageIndex)
{
	VkRenderPass renderPass = m_swapchain->renderPass();
	if (renderPass == VK_NULL_HANDLE) {
		return;
	}

	VkFramebuffer framebuffer = m_swapchain->framebuffer(imageIndex);
	if (framebuffer == VK_NULL_HANDLE) {
		return;
	}

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(cmd, &beginInfo);

	float r = static_cast<float>(m_backgroundColor.redF());
	float g = static_cast<float>(m_backgroundColor.greenF());
	float b = static_cast<float>(m_backgroundColor.blueF());

	VkClearValue clearValues[2]{};
	clearValues[0].color = {{r, g, b, 1.0f}};
	clearValues[1].color = {{0.0f, 0.0f, 0.0f, 0.0f}};

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = renderPass;
	renderPassInfo.framebuffer = framebuffer;
	renderPassInfo.renderArea.offset = {0, 0};
	renderPassInfo.renderArea.extent = m_swapchain->extent();
	renderPassInfo.clearValueCount = (m_swapchain->msaaSamples() != VK_SAMPLE_COUNT_1_BIT) ? 2 : 1;
	renderPassInfo.pClearValues = clearValues;

	vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(m_swapchain->extent().width);
	viewport.height = static_cast<float>(m_swapchain->extent().height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(cmd, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = {0, 0};
	scissor.extent = m_swapchain->extent();
	vkCmdSetScissor(cmd, 0, 1, &scissor);

	if (m_fillVbo.buffer != VK_NULL_HANDLE) {
		VkPipeline fillPipe = m_pipeline->pipeline(PipelineType::Fill);
		VkPipelineLayout fillLayout = m_pipeline->pipelineLayout(PipelineType::Fill);
		if (fillPipe != VK_NULL_HANDLE && fillLayout != VK_NULL_HANDLE) {
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, fillPipe);
			VkDeviceSize offset = 0;
			vkCmdBindVertexBuffers(cmd, 0, 1, &m_fillVbo.buffer, &offset);
			uint32_t vertexCount = static_cast<uint32_t>(m_fillVbo.size / (2 * sizeof(float)));
			if (vertexCount > 0) {
				vkCmdDraw(cmd, vertexCount, 1, 0, 0);
			}
		}
	}

	if (m_gridVbo.buffer != VK_NULL_HANDLE) {
		VkPipeline gridPipe = m_pipeline->pipeline(PipelineType::Grid);
		VkPipelineLayout gridLayout = m_pipeline->pipelineLayout(PipelineType::Grid);
		if (gridPipe != VK_NULL_HANDLE && gridLayout != VK_NULL_HANDLE) {
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, gridPipe);
			VkDeviceSize offset = 0;
			vkCmdBindVertexBuffers(cmd, 0, 1, &m_gridVbo.buffer, &offset);
			uint32_t vertexCount = static_cast<uint32_t>(m_gridVbo.size / (5 * sizeof(float)));
			if (vertexCount > 0) {
				vkCmdDraw(cmd, vertexCount, 1, 0, 0);
			}
		}
	}

	if (m_curveVbo.buffer != VK_NULL_HANDLE) {
		VkPipeline curvePipe = m_pipeline->pipeline(PipelineType::Curve);
		VkPipelineLayout curveLayout = m_pipeline->pipelineLayout(PipelineType::Curve);
		if (curvePipe != VK_NULL_HANDLE && curveLayout != VK_NULL_HANDLE) {
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, curvePipe);
			VkDeviceSize offset = 0;
			vkCmdBindVertexBuffers(cmd, 0, 1, &m_curveVbo.buffer, &offset);
			uint32_t vertexCount = static_cast<uint32_t>(m_curveVbo.size / (3 * sizeof(float)));
			if (vertexCount > 0) {
				vkCmdDraw(cmd, vertexCount, 1, 0, 0);
			}
		}
	}

	if (m_glyphVbo.buffer != VK_NULL_HANDLE) {
		VkPipeline glyphPipe = m_pipeline->pipeline(PipelineType::Glyph);
		VkPipelineLayout glyphLayout = m_pipeline->pipelineLayout(PipelineType::Glyph);
		if (glyphPipe != VK_NULL_HANDLE && glyphLayout != VK_NULL_HANDLE) {
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, glyphPipe);
			VkDeviceSize offset = 0;
			vkCmdBindVertexBuffers(cmd, 0, 1, &m_glyphVbo.buffer, &offset);
			uint32_t vertexCount = static_cast<uint32_t>(m_glyphVbo.size / (4 * sizeof(float)));
			if (vertexCount > 0) {
				vkCmdDraw(cmd, vertexCount, 1, 0, 0);
			}
		}
	}

	vkCmdEndRenderPass(cmd);
	vkEndCommandBuffer(cmd);
}

VkCommandBuffer VulkanRenderer::beginFrame(uint32_t* outImageIndex)
{
	m_frameSync->beginFrame(outImageIndex);
	if (*outImageIndex >= static_cast<uint32_t>(m_commandBuffers.size())) {
		return VK_NULL_HANDLE;
	}
	VkCommandBuffer cmd = m_commandBuffers[*outImageIndex];
	vkResetCommandBuffer(cmd, 0);
	recordCommandBuffer(cmd, *outImageIndex);
	return cmd;
}

void VulkanRenderer::endFrame(uint32_t imageIndex, VkQueue presentQueue)
{
	if (imageIndex >= static_cast<uint32_t>(m_commandBuffers.size())) {
		return;
	}
	m_frameSync->submitFrame(imageIndex, m_commandBuffers[imageIndex], presentQueue);
}

void VulkanRenderer::updateGridVertices(const QVector<float>& vertices)
{
	if (!m_context) return;
	ensureVBO(m_gridVbo, vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
}

void VulkanRenderer::updateCurveVertices(const QVector<float>& vertices)
{
	if (!m_context) return;
	ensureVBO(m_curveVbo, vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
}

void VulkanRenderer::updateFillVertices(const QVector<float>& vertices)
{
	if (!m_context) return;
	ensureVBO(m_fillVbo, vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
}

void VulkanRenderer::updateGlyphVertices(const QVector<float>& vertices)
{
	if (!m_context) return;
	ensureVBO(m_glyphVbo, vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
}

void VulkanRenderer::setCurveColor(const QColor& color)
{
	m_curveColor = color;
}

void VulkanRenderer::setBackgroundColor(const QColor& color)
{
	m_backgroundColor = color;
}

void VulkanRenderer::ensureVBO(VboResource& vbo, const QVector<float>& vertices,
							   VkBufferUsageFlags usage)
{
	VkDeviceSize newSize = vertices.size() * sizeof(float);
	if (newSize == 0) {
		destroyVboResource(vbo);
		return;
	}

	if (vbo.buffer != VK_NULL_HANDLE && vbo.size == newSize) {
		void* mapped = nullptr;
		vkMapMemory(m_context->device(), vbo.memory, 0, newSize, 0, &mapped);
		if (mapped) {
			std::memcpy(mapped, vertices.constData(), static_cast<size_t>(newSize));
			vkUnmapMemory(m_context->device(), vbo.memory);
		}
	} else {
		destroyVboResource(vbo);
		createVboResource(vbo, newSize, vertices.constData(), usage);
	}
}

void VulkanRenderer::destroyVboResource(VboResource& vbo)
{
	if (!m_context) return;
	VkDevice device = m_context->device();
	if (device == VK_NULL_HANDLE) return;

	if (vbo.buffer != VK_NULL_HANDLE) {
		vkDestroyBuffer(device, vbo.buffer, nullptr);
		vbo.buffer = VK_NULL_HANDLE;
	}
	if (vbo.memory != VK_NULL_HANDLE) {
		vkFreeMemory(device, vbo.memory, nullptr);
		vbo.memory = VK_NULL_HANDLE;
	}
	vbo.size = 0;
}

void VulkanRenderer::createVboResource(VboResource& vbo, VkDeviceSize size,
									   const void* data, VkBufferUsageFlags usage)
{
	VkDevice device = m_context->device();

	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(device, &bufferInfo, nullptr, &vbo.buffer) != VK_SUCCESS) {
		vbo.buffer = VK_NULL_HANDLE;
		return;
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device, vbo.buffer, &memRequirements);

	uint32_t memoryTypeIndex = findMemoryType(m_context->physicalDevice(),
		memRequirements.memoryTypeBits,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = memoryTypeIndex;

	if (vkAllocateMemory(device, &allocInfo, nullptr, &vbo.memory) != VK_SUCCESS) {
		vkDestroyBuffer(device, vbo.buffer, nullptr);
		vbo.buffer = VK_NULL_HANDLE;
		return;
	}

	vkBindBufferMemory(device, vbo.buffer, vbo.memory, 0);

	vbo.size = size;

	if (data) {
		void* mapped = nullptr;
		vkMapMemory(device, vbo.memory, 0, size, 0, &mapped);
		if (mapped) {
			std::memcpy(mapped, data, static_cast<size_t>(size));
			vkUnmapMemory(device, vbo.memory);
		}
	}
}

uint32_t VulkanRenderer::findMemoryType(VkPhysicalDevice physicalDevice,
										uint32_t typeFilter,
										VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
		if ((typeFilter & (1u << i)) &&
			(memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}
	return 0;
}
