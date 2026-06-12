#include "VulkanQtIntegration.h"
#include "VulkanRenderer.h"
#include "VulkanContext.h"
#include <QResizeEvent>

VulkanQtWindow::VulkanQtWindow(VulkanContext* ctx, VulkanRenderer* renderer, QWindow* parent)
    : QWindow(parent), m_ctx(ctx), m_renderer(renderer)
{
    setSurfaceType(QSurface::VulkanSurface);
    setVulkanInstance(ctx->qtInstance());
    create();
}

VulkanQtWindow::~VulkanQtWindow()
{
    shutdown();
}

void VulkanQtWindow::shutdown()
{
    m_initialized = false;
}

VkSurfaceKHR VulkanQtWindow::vkSurface() const {
    return QVulkanInstance::surfaceForWindow(const_cast<VulkanQtWindow*>(this));
}

void VulkanQtWindow::requestRender() {
    requestUpdate();
}

void VulkanQtWindow::exposeEvent(QExposeEvent* e) {
    QWindow::exposeEvent(e);
    if (!m_initialized && isExposed()) {
        VkSurfaceKHR surface = vkSurface();
        QSize sz = size();
        if (sz.isEmpty()) sz = QSize(800, 400);
        qDebug() << "VulkanQtWindow::exposeEvent: surface=" << (surface ? "valid" : "NULL") << "size=" << sz;
        if (surface && m_renderer->initialize(surface, sz)) {
            m_initialized = true;
            qDebug() << "VulkanQtWindow::exposeEvent: renderer initialized, requesting update";
            requestUpdate();
        } else {
            qWarning() << "VulkanQtWindow::exposeEvent: renderer initialization FAILED";
        }
    } else if (m_initialized && isExposed()) {
        requestUpdate();
    }
}

void VulkanQtWindow::resizeEvent(QResizeEvent* e) {
    QWindow::resizeEvent(e);
    if (m_initialized && handle()) {
        int w = static_cast<int>(e->size().width());
        int h = static_cast<int>(e->size().height());
        m_renderer->resize(QSize(w > 0 ? w : 1, h > 0 ? h : 1));
    }
}

bool VulkanQtWindow::event(QEvent* e) {
    if (e->type() == QEvent::UpdateRequest) {
        if (m_initialized && isExposed() && handle()) {
            m_renderer->renderFrame();
            requestUpdate();
        }
        return true;
    }
    return QWindow::event(e);
}
