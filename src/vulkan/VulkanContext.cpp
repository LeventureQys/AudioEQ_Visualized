#include "VulkanContext.h"

#include <QDebug>
#include <set>
#include <vector>

VulkanContext::VulkanContext() = default;

VulkanContext::~VulkanContext() {
    destroy();
}

bool VulkanContext::initialize() {
    VkResult result = volkInitialize();
    if (result != VK_SUCCESS) {
        qWarning() << "VulkanContext: volkInitialize failed with" << result;
        return false;
    }

    m_qtInstance = new QVulkanInstance();
    m_qtInstance->setApiVersion(QVersionNumber(1, 3, 0));

#ifndef NDEBUG
    m_qtInstance->setLayers({"VK_LAYER_KHRONOS_validation"});
#endif

    if (!m_qtInstance->create()) {
        qWarning() << "VulkanContext: QVulkanInstance::create() failed";
        delete m_qtInstance;
        m_qtInstance = nullptr;
        return false;
    }

    volkLoadInstance(m_qtInstance->vkInstance());

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_qtInstance->vkInstance(), &deviceCount, nullptr);
    if (deviceCount == 0) {
        qWarning() << "VulkanContext: No physical devices found";
        destroy();
        return false;
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_qtInstance->vkInstance(), &deviceCount, devices.data());

    m_physicalDevice = devices[0];

    VkPhysicalDeviceProperties deviceProps;
    vkGetPhysicalDeviceProperties(m_physicalDevice, &deviceProps);
    qDebug() << "VulkanContext: Using physical device:" << deviceProps.deviceName;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount, queueFamilies.data());

    m_graphicsFamily = 0;
    bool found = false;
    for (uint32_t i = 0; i < queueFamilyCount; ++i) {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            m_graphicsFamily = i;
            found = true;
            break;
        }
    }

    if (!found) {
        qWarning() << "VulkanContext: No graphics queue family found";
        destroy();
        return false;
    }

    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = m_graphicsFamily;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };

    VkPhysicalDeviceFeatures deviceFeatures = {};
    deviceFeatures.wideLines = VK_TRUE;  // Required for curve rendering line width > 1.0

    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

    result = vkCreateDevice(m_physicalDevice, &deviceCreateInfo, nullptr, &m_device);
    if (result != VK_SUCCESS) {
        qWarning() << "VulkanContext: vkCreateDevice failed with" << result;
        destroy();
        return false;
    }

    volkLoadDevice(m_device);

    vkGetDeviceQueue(m_device, m_graphicsFamily, 0, &m_graphicsQueue);

    m_valid = true;
    qDebug() << "VulkanContext: Initialization complete";
    return true;
}

void VulkanContext::destroy() {
    if (m_device != VK_NULL_HANDLE) {
        vkDestroyDevice(m_device, nullptr);
        m_device = VK_NULL_HANDLE;
    }

    if (m_qtInstance) {
        m_qtInstance->destroy();
        delete m_qtInstance;
        m_qtInstance = nullptr;
    }

    m_physicalDevice = VK_NULL_HANDLE;
    m_graphicsQueue  = VK_NULL_HANDLE;
    m_graphicsFamily = 0;
    m_valid = false;
}

bool VulkanContext::isValid() const {
    return m_valid;
}

QVulkanInstance* VulkanContext::qtInstance() const { return m_qtInstance; }

VkInstance VulkanContext::vkInstance() const {
    return m_qtInstance ? m_qtInstance->vkInstance() : VK_NULL_HANDLE;
}

VkPhysicalDevice VulkanContext::physicalDevice() const { return m_physicalDevice; }
VkDevice         VulkanContext::device()         const { return m_device; }
VkQueue          VulkanContext::graphicsQueue()  const { return m_graphicsQueue; }
uint32_t         VulkanContext::graphicsFamily() const { return m_graphicsFamily; }

bool VulkanContext::isVulkanSupported() {
    if (volkInitialize() != VK_SUCCESS) {
        return false;
    }

    QVulkanInstance tempInstance;
    tempInstance.setApiVersion(QVersionNumber(1, 3, 0));

    if (!tempInstance.create()) {
        return false;
    }

    volkLoadInstance(tempInstance.vkInstance());

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(tempInstance.vkInstance(), &deviceCount, nullptr);

    tempInstance.destroy();
    return deviceCount > 0;
}
