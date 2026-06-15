#include "ViewEqualizer.h"
#include "vulkan/VulkanRenderer.h"
#include "vulkan/VulkanContext.h"
#include "vulkan/VulkanQtIntegration.h"
#include "BandHandle.h"
#include "LpfHandle.h"
#include "HpfHandle.h"
#include "EqualizerModel.h"
#include "CoordinateMapper.h"
#include <QVBoxLayout>
#include <QResizeEvent>
#include <QShowEvent>
#include <QMoveEvent>

ViewEqualizer::ViewEqualizer(QWidget* parent) : QWidget(parent) {}

ViewEqualizer::~ViewEqualizer()
{
    if (m_window) {
        m_window->shutdown();
        m_window = nullptr;
    }
}

void ViewEqualizer::setVulkanContext(VulkanContext* ctx) {
    m_ctx = ctx;
}

void ViewEqualizer::setRenderer(VulkanRenderer* renderer) {
    m_renderer = renderer;
    if (m_ctx && m_renderer) {
        createVulkanWindow();
    }
}

void ViewEqualizer::setCoordinateMapper(CoordinateMapper* mapper) {
    m_mapper = mapper;
}

void ViewEqualizer::setModel(EqualizerModel* model) {
    m_model = model;
    if (m_model) {
        connectSignals();
    }
}

void ViewEqualizer::createVulkanWindow() {
    if (!m_ctx || !m_renderer) return;

    qDebug() << "ViewEqualizer::createVulkanWindow: creating VulkanQtWindow...";
    m_window = new VulkanQtWindow(m_ctx, m_renderer);
    qDebug() << "ViewEqualizer::createVulkanWindow: VulkanQtWindow created, creating container...";

    m_vulkanContainer = QWidget::createWindowContainer(m_window, this);
    m_vulkanContainer->setGeometry(0, 0, width(), height());
    m_vulkanContainer->lower();

    if (m_mapper) {
        m_mapper->setViewport(QRectF(MARGIN_LEFT, MARGIN_TOP,
                                      width() - MARGIN_LEFT - MARGIN_RIGHT,
                                      height() - MARGIN_TOP - MARGIN_BOTTOM));
    }

    m_initialized = true;
    if (auto* w = window()) {
        w->installEventFilter(this);
    }
    qDebug() << "ViewEqualizer::createVulkanWindow: complete, m_initialized=true";
}

void ViewEqualizer::connectSignals() {
    if (!m_model) return;

    connect(m_model, &EqualizerModel::bandChanged, this, &ViewEqualizer::onBandChanged);
    connect(m_model, &EqualizerModel::bandAdded, this, &ViewEqualizer::onBandAdded);
    connect(m_model, &EqualizerModel::bandRemoved, this, &ViewEqualizer::onBandRemoved);
    connect(m_model, &EqualizerModel::lpfChanged, this, &ViewEqualizer::onLpfChanged);
    connect(m_model, &EqualizerModel::hpfChanged, this, &ViewEqualizer::onHpfChanged);
    connect(m_model, &EqualizerModel::sampleRateChanged, this, &ViewEqualizer::onSampleRateChanged);
}

void ViewEqualizer::syncBandHandles(int count) {
    while (m_bandHandles.size() < count) {
        int idx = static_cast<int>(m_bandHandles.size());
        auto* handle = new BandHandle(idx, this);
        handle->setGeometry(0, 0, HANDLE_RADIUS * 2, HANDLE_RADIUS * 2);
        handle->raise();
        handle->show();

        connect(handle, &BandHandle::bandDragged, this, &ViewEqualizer::bandDragged);
        connect(handle, &BandHandle::bandClicked, this, [this](int idx) {
            if (m_model) m_model->setFocusedBandIndex(idx);
            emit bandClicked(idx);
        });
        connect(handle, &BandHandle::bandReleased, this, &ViewEqualizer::bandReleased);

        m_bandHandles.append(handle);
    }

    while (m_bandHandles.size() > count) {
        delete m_bandHandles.takeLast();
    }

    if (!m_lpfHandle) {
        m_lpfHandle = new LpfHandle(this);
        m_lpfHandle->setGeometry(0, 0, 40, 24);
        m_lpfHandle->hide();
        connect(m_lpfHandle, &LpfHandle::lpfDragged, this, &ViewEqualizer::lpfDragged);
    }
    if (!m_hpfHandle) {
        m_hpfHandle = new HpfHandle(this);
        m_hpfHandle->setGeometry(0, 0, 40, 24);
        m_hpfHandle->hide();
        connect(m_hpfHandle, &HpfHandle::hpfDragged, this, &ViewEqualizer::hpfDragged);
    }

    repositionAllHandles();
}

void ViewEqualizer::updateBandHandlePosition(int index, QPoint pos) {
    if (index < 0 || index >= m_bandHandles.size()) return;
    auto* h = m_bandHandles[index];
    h->setCenter(pos);
    QPoint globalCenter = mapToGlobal(pos);
    h->move(globalCenter.x() - HANDLE_RADIUS, globalCenter.y() - HANDLE_RADIUS);
    h->resize(HANDLE_RADIUS * 2, HANDLE_RADIUS * 2);
}

void ViewEqualizer::setLpfHandleVisible(bool visible) {
    if (m_lpfHandle) m_lpfHandle->setVisible(visible);
}

void ViewEqualizer::setHpfHandleVisible(bool visible) {
    if (m_hpfHandle) m_hpfHandle->setVisible(visible);
}

VulkanRenderer* ViewEqualizer::vulkanRenderer() const { return m_renderer; }

void ViewEqualizer::onBandChanged(int index) {
    if (!m_model || !m_mapper) return;
    if (index >= 0 && index < m_bandHandles.size() && m_bandHandles[index]->isDragging()) {
        if (m_window) m_window->requestRender();
        return;
    }
    EQBand b = m_model->band(index);
    QPointF pixel = m_mapper->toPixel(b.freqHz, b.gainDb);
    updateBandHandlePosition(index, pixel.toPoint());
    if (m_window) m_window->requestRender();
}

void ViewEqualizer::onBandAdded(int index) {
    Q_UNUSED(index);
    syncBandHandles(m_model->bandCount());
}

void ViewEqualizer::onBandRemoved(int index) {
    Q_UNUSED(index);
    syncBandHandles(m_model->bandCount());
}

void ViewEqualizer::onLpfChanged() {
    if (!m_model || !m_mapper || !m_lpfHandle) return;
    ShelfBand lpf = m_model->lpf();
    setLpfHandleVisible(lpf.enabled);
    if (lpf.enabled) {
        if (!m_lpfHandle->isDragging()) {
            double x = m_mapper->freqToX(lpf.freqHz);
            double y = m_mapper->gainToY(0.0);
            m_lpfHandle->setCenter(QPoint(static_cast<int>(x), static_cast<int>(y)));
            QPoint g = mapToGlobal(QPoint(static_cast<int>(x), static_cast<int>(y)));
            m_lpfHandle->move(g.x() - 20, g.y() - 12);
        }
        m_lpfHandle->raise();
    }
    if (m_window) m_window->requestRender();
}

void ViewEqualizer::onHpfChanged() {
    if (!m_model || !m_mapper || !m_hpfHandle) return;
    ShelfBand hpf = m_model->hpf();
    setHpfHandleVisible(hpf.enabled);
    if (hpf.enabled) {
        if (!m_hpfHandle->isDragging()) {
            double x = m_mapper->freqToX(hpf.freqHz);
            double y = m_mapper->gainToY(0.0);
            m_hpfHandle->setCenter(QPoint(static_cast<int>(x), static_cast<int>(y)));
            QPoint g = mapToGlobal(QPoint(static_cast<int>(x), static_cast<int>(y)));
            m_hpfHandle->move(g.x() - 20, g.y() - 12);
        }
        m_hpfHandle->raise();
    }
    if (m_window) m_window->requestRender();
}

void ViewEqualizer::onSampleRateChanged(SampleRate rate) {
    if (m_mapper) {
        m_mapper->setSampleRate(rate);
        repositionAllHandles();
    }
    if (m_window) m_window->requestRender();
}

void ViewEqualizer::setVisible(bool visible)
{
    if (!visible && m_initialized) {
        if (m_window) {
            m_window->shutdown();
        }
        if (m_renderer) {
            m_renderer->destroy();
        }
    }
    QWidget::setVisible(visible);
}

void ViewEqualizer::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    if (m_mapper) {
        QRectF innerRect(MARGIN_LEFT, MARGIN_TOP,
                         width() - MARGIN_LEFT - MARGIN_RIGHT,
                         height() - MARGIN_TOP - MARGIN_BOTTOM);
        m_mapper->setViewport(innerRect);
    }
    if (m_vulkanContainer) {
        m_vulkanContainer->resize(size());
    }
    repositionAllHandles();
}

void ViewEqualizer::moveEvent(QMoveEvent* event) {
    QWidget::moveEvent(event);
    repositionAllHandles();
}

bool ViewEqualizer::eventFilter(QObject* obj, QEvent* event) {
    if (event->type() == QEvent::Move) {
        repositionAllHandles();
    }
    return QWidget::eventFilter(obj, event);
}

void ViewEqualizer::repositionAllHandles() {
    if (!m_model || !m_mapper) return;
    for (int i = 0; i < m_model->bandCount(); ++i) {
        EQBand b = m_model->band(i);
        QPointF pixel = m_mapper->toPixel(b.freqHz, b.gainDb);
        updateBandHandlePosition(i, pixel.toPoint());
    }
    onLpfChanged();
    onHpfChanged();
}
