#pragma once

#include "volk.h"
#include <cstddef>
#include <cstdint>

class VulkanContext;

struct BufferAllocation {
    VkBuffer        buffer  = VK_NULL_HANDLE;
    VkDeviceMemory  memory  = VK_NULL_HANDLE;
    void*           mapped  = nullptr;
    VkDeviceSize    size    = 0;
};

class VulkanBufferPool {
public:
    explicit VulkanBufferPool(VulkanContext* ctx);
    ~VulkanBufferPool();

    BufferAllocation createVertexBuffer(VkDeviceSize size);
    BufferAllocation createIndexBuffer(VkDeviceSize size);
    BufferAllocation createUniformBuffer(VkDeviceSize size);

    void uploadData(const BufferAllocation& dst, const void* data, VkDeviceSize size);
    void free(BufferAllocation& alloc);

private:
    VulkanContext*  m_ctx;

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags props);
    BufferAllocation createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags props);

    // Command pool for staging uploads
    VkCommandPool    m_commandPool = VK_NULL_HANDLE;
    void ensureCommandPool();
    VkCommandBuffer beginOneShot();
    void endOneShot(VkCommandBuffer cmd);
};
