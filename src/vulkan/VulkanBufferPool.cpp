#include "vulkan/VulkanBufferPool.h"
#include "vulkan/VulkanContext.h"
#include <cstring>

VulkanBufferPool::VulkanBufferPool()
{
}

VulkanBufferPool::~VulkanBufferPool()
{
	cleanup();
}

bool VulkanBufferPool::initialize(VulkanContext* ctx)
{
	if (!ctx || ctx->device() == VK_NULL_HANDLE) {
		return false;
	}
	m_context = ctx;
	return true;
}

void VulkanBufferPool::cleanup()
{
	if (!m_context) {
		return;
	}

	for (VkBuffer buffer : m_buffers) {
		vkDestroyBuffer(m_context->device(), buffer, nullptr);
	}
	m_buffers.clear();

	for (VkDeviceMemory memory : m_memories) {
		vkFreeMemory(m_context->device(), memory, nullptr);
	}
	m_memories.clear();

	m_context = nullptr;
}

void VulkanBufferPool::freeBuffer(VkBuffer buffer)
{
	if (!m_context || buffer == VK_NULL_HANDLE) {
		return;
	}

	int index = m_buffers.indexOf(buffer);
	if (index < 0) {
		return;
	}

	vkDestroyBuffer(m_context->device(), buffer, nullptr);
	vkFreeMemory(m_context->device(), m_memories[index], nullptr);

	m_buffers.removeAt(index);
	m_memories.removeAt(index);
}

VulkanBufferPool::BufferAllocation VulkanBufferPool::createBuffer(VkDeviceSize size,
	VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
{
	BufferAllocation result{};

	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(m_context->device(), &bufferInfo, nullptr, &result.buffer) != VK_SUCCESS) {
		return { VK_NULL_HANDLE, VK_NULL_HANDLE };
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(m_context->device(), result.buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(m_context->device(), &allocInfo, nullptr, &result.memory) != VK_SUCCESS) {
		vkDestroyBuffer(m_context->device(), result.buffer, nullptr);
		return { VK_NULL_HANDLE, VK_NULL_HANDLE };
	}

	vkBindBufferMemory(m_context->device(), result.buffer, result.memory, 0);

	m_buffers.append(result.buffer);
	m_memories.append(result.memory);

	return result;
}

VkBuffer VulkanBufferPool::allocateVertexBuffer(VkDeviceSize size, const void* data)
{
	if (!m_context) {
		return VK_NULL_HANDLE;
	}

	BufferAllocation alloc = createBuffer(size,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	if (alloc.buffer == VK_NULL_HANDLE) {
		return VK_NULL_HANDLE;
	}

	if (data) {
		copyDataToBuffer(alloc.buffer, size, data);
	}

	return alloc.buffer;
}

VkBuffer VulkanBufferPool::allocateIndexBuffer(VkDeviceSize size, const void* data)
{
	if (!m_context) {
		return VK_NULL_HANDLE;
	}

	BufferAllocation alloc = createBuffer(size,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	if (alloc.buffer == VK_NULL_HANDLE) {
		return VK_NULL_HANDLE;
	}

	if (data) {
		copyDataToBuffer(alloc.buffer, size, data);
	}

	return alloc.buffer;
}

VkBuffer VulkanBufferPool::allocateUniformBuffer(VkDeviceSize size)
{
	if (!m_context) {
		return VK_NULL_HANDLE;
	}

	BufferAllocation alloc = createBuffer(size,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	return alloc.buffer;
}

void VulkanBufferPool::updateUniformBuffer(VkBuffer buffer, VkDeviceSize size, const void* data)
{
	if (!m_context || buffer == VK_NULL_HANDLE || !data) {
		return;
	}

	int index = m_buffers.indexOf(buffer);
	if (index < 0) {
		return;
	}

	void* mapped = nullptr;
	vkMapMemory(m_context->device(), m_memories[index], 0, size, 0, &mapped);
	if (mapped) {
		std::memcpy(mapped, data, static_cast<size_t>(size));
		vkUnmapMemory(m_context->device(), m_memories[index]);
	}
}

void VulkanBufferPool::copyDataToBuffer(VkBuffer buffer, VkDeviceSize size, const void* data)
{
	if (!m_context || buffer == VK_NULL_HANDLE || !data) {
		return;
	}

	int index = m_buffers.indexOf(buffer);
	if (index < 0) {
		return;
	}

	void* mapped = nullptr;
	vkMapMemory(m_context->device(), m_memories[index], 0, size, 0, &mapped);
	if (mapped) {
		std::memcpy(mapped, data, static_cast<size_t>(size));
		vkUnmapMemory(m_context->device(), m_memories[index]);
	}
}

uint32_t VulkanBufferPool::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(m_context->physicalDevice(), &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
		if ((typeFilter & (1u << i)) &&
			(memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	return 0;
}
