#pragma once

#include <QWindow>
#include <QVulkanInstance>
#include "volk.h"

class VulkanContext;
class VulkanRenderer;

class VulkanQtWindow : public QWindow {
    Q_OBJECT
public:
    explicit VulkanQtWindow(VulkanContext* ctx, VulkanRenderer* renderer, QWindow* parent = nullptr);
    ~VulkanQtWindow() override;

    VkSurfaceKHR vkSurface() const;
    void requestRender();

protected:
    void exposeEvent(QExposeEvent* e) override;
    void resizeEvent(QResizeEvent* e) override;
    bool event(QEvent* e) override;

private:
    VulkanContext*  m_ctx;
    VulkanRenderer* m_renderer;
    bool            m_initialized = false;
};
