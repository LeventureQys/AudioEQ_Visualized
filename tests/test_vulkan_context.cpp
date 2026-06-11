#include <QCoreApplication>
#include <QDebug>
#include "vulkan/VulkanContext.h"

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    qDebug() << "=== VulkanContext Runtime Test ===";

    bool supported = VulkanContext::isVulkanSupported();
    qDebug() << "isVulkanSupported():" << (supported ? "YES" : "NO");

    if (!supported) {
        qWarning() << "Vulkan not supported on this machine — exiting gracefully.";
        return 0;
    }

    VulkanContext ctx;
    bool ok = ctx.initialize();
    qDebug() << "initialize():" << (ok ? "SUCCESS" : "FAILED");

    if (ok) {
        qDebug() << "  VkInstance:" << ctx.vkInstance();
        qDebug() << "  PhysicalDevice:" << ctx.physicalDevice();
        qDebug() << "  Device:" << ctx.device();
        qDebug() << "  GraphicsQueue:" << ctx.graphicsQueue();
        qDebug() << "  GraphicsFamily:" << ctx.graphicsFamily();
    }

    ctx.destroy();
    qDebug() << "isValid() after destroy:" << ctx.isValid();

    qDebug() << "=== Test Complete ===";
    return ok ? 0 : 1;
}
