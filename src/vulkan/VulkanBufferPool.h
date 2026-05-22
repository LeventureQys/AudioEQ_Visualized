#pragma once
#include <volk.h>
#include <QVector>
#include <QHash>

class VulkanContext;

class VulkanBufferPool {
public:
	VulkanBufferPool();
	~VulkanBufferPool();

	VulkanBufferPool(const VulkanBufferPool&) = delete;
	VulkanBufferPool& operator=(const VulkanBufferPool&) = delete;

	bool initialize(VulkanContext* ctx);
	void cleanup();

	VkBuffer allocateVertexBuffer(VkDeviceSize size, const void* data);
	VkBuffer allocateIndexBuffer(VkDeviceSize size, const void* data);
	VkBuffer allocateUniformBuffer(VkDeviceSize size);
	void updateUniformBuffer(VkBuffer buffer, VkDeviceSize size, const void* data);

	void freeBuffer(VkBuffer buffer);

private:
	VulkanContext* m_context = nullptr;
	QVector<VkBuffer> m_buffers;
	QVector<VkDeviceMemory> m_memories;

	struct BufferAllocation {
		VkBuffer buffer = VK_NULL_HANDLE;
		VkDeviceMemory memory = VK_NULL_HANDLE;
	};

	BufferAllocation createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
		VkMemoryPropertyFlags properties);
	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	void copyDataToBuffer(VkBuffer buffer, VkDeviceSize size, const void* data);
};
