#pragma once

#include "volk.h"

#include <QVulkanInstance>
#include <QVector>

class AUDIOEQ_EXPORT VulkanContext {
public:
    VulkanContext();
    ~VulkanContext();

    bool initialize();

    void destroy();

    bool isValid() const;

    QVulkanInstance* qtInstance()     const;
    VkInstance       vkInstance()     const;
    VkPhysicalDevice physicalDevice() const;
    VkDevice         device()         const;
    VkQueue          graphicsQueue()  const;
    uint32_t         graphicsFamily() const;

    static bool isVulkanSupported();

private:
    QVulkanInstance* m_qtInstance     = nullptr;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice         m_device         = VK_NULL_HANDLE;
    VkQueue          m_graphicsQueue  = VK_NULL_HANDLE;
    uint32_t         m_graphicsFamily = 0;
    bool             m_valid          = false;
};
