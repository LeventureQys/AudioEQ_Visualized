#include "VulkanBufferPool.h"
#include "VulkanContext.h"
#include <cstring>

VulkanBufferPool::VulkanBufferPool(VulkanContext* ctx) : m_ctx(ctx) {}
VulkanBufferPool::~VulkanBufferPool() {
    if (m_commandPool) {
        vkDestroyCommandPool(m_ctx->device(), m_commandPool, nullptr);
    }
}

void VulkanBufferPool::ensureCommandPool() {
    if (m_commandPool) return;
    VkCommandPoolCreateInfo ci = {};
    ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    ci.queueFamilyIndex = m_ctx->graphicsFamily();
    ci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    vkCreateCommandPool(m_ctx->device(), &ci, nullptr, &m_commandPool);
}

VkCommandBuffer VulkanBufferPool::beginOneShot() {
    ensureCommandPool();
    VkCommandBufferAllocateInfo ai = {};
    ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    ai.commandPool = m_commandPool;
    ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    ai.commandBufferCount = 1;
    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(m_ctx->device(), &ai, &cmd);

    VkCommandBufferBeginInfo bi = {};
    bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &bi);
    return cmd;
}

void VulkanBufferPool::endOneShot(VkCommandBuffer cmd) {
    vkEndCommandBuffer(cmd);
    VkSubmitInfo si = {};
    si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.commandBufferCount = 1;
    si.pCommandBuffers = &cmd;
    vkQueueSubmit(m_ctx->graphicsQueue(), 1, &si, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_ctx->graphicsQueue());
    vkFreeCommandBuffers(m_ctx->device(), m_commandPool, 1, &cmd);
}

uint32_t VulkanBufferPool::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags props) {
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(m_ctx->physicalDevice(), &memProps);
    for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i) {
        if ((typeFilter & (1 << i)) && (memProps.memoryTypes[i].propertyFlags & props) == props)
            return i;
    }
    return 0;
}

BufferAllocation VulkanBufferPool::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags props) {
    BufferAllocation alloc;
    alloc.size = size;

    VkBufferCreateInfo bi = {};
    bi.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bi.size = size;
    bi.usage = usage;
    bi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vkCreateBuffer(m_ctx->device(), &bi, nullptr, &alloc.buffer);

    VkMemoryRequirements memReq;
    vkGetBufferMemoryRequirements(m_ctx->device(), alloc.buffer, &memReq);

    VkMemoryAllocateInfo ai = {};
    ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    ai.allocationSize = memReq.size;
    ai.memoryTypeIndex = findMemoryType(memReq.memoryTypeBits, props);
    vkAllocateMemory(m_ctx->device(), &ai, nullptr, &alloc.memory);
    vkBindBufferMemory(m_ctx->device(), alloc.buffer, alloc.memory, 0);

    if (props & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
        vkMapMemory(m_ctx->device(), alloc.memory, 0, size, 0, &alloc.mapped);
    }

    return alloc;
}

BufferAllocation VulkanBufferPool::createVertexBuffer(VkDeviceSize size) {
    return createBuffer(size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

BufferAllocation VulkanBufferPool::createIndexBuffer(VkDeviceSize size) {
    return createBuffer(size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

BufferAllocation VulkanBufferPool::createUniformBuffer(VkDeviceSize size) {
    return createBuffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}

void VulkanBufferPool::uploadData(const BufferAllocation& dst, const void* data, VkDeviceSize size) {
    // Create staging buffer
    BufferAllocation staging = createBuffer(
        size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );

    // Copy data to staging
    memcpy(staging.mapped, data, size);

    // Copy from staging to device-local buffer
    VkCommandBuffer cmd = beginOneShot();
    VkBufferCopy copyRegion = {0, 0, size};
    vkCmdCopyBuffer(cmd, staging.buffer, dst.buffer, 1, &copyRegion);
    endOneShot(cmd);

    // Free staging
    free(staging);
}

void VulkanBufferPool::free(BufferAllocation& alloc) {
    VkDevice dev = m_ctx->device();
    if (alloc.mapped) {
        vkUnmapMemory(dev, alloc.memory);
        alloc.mapped = nullptr;
    }
    if (alloc.buffer) {
        vkDestroyBuffer(dev, alloc.buffer, nullptr);
        alloc.buffer = VK_NULL_HANDLE;
    }
    if (alloc.memory) {
        vkFreeMemory(dev, alloc.memory, nullptr);
        alloc.memory = VK_NULL_HANDLE;
    }
    alloc.size = 0;
}
