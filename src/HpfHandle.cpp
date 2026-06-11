#include "HpfHandle.h"
#include <QMouseEvent>

HpfHandle::HpfHandle(QWidget* parent)
    : BandHandle(-2, parent)
{
}

void HpfHandle::paintEvent(QPaintEvent*) {}

void HpfHandle::mousePressEvent(QMouseEvent* event) {
    double dx = event->pos().x() - center().x();
    double dy = event->pos().y() - center().y();
    double hx = dx / ELLIPSE_RX;
    double hy = dy / ELLIPSE_RY;
    if (hx * hx + hy * hy > 1.0) {
        event->ignore();
        return;
    }
    m_dragging = true;
    m_lastEmitPos = event->pos();
    m_hasPending = false;
    emit bandClicked(bandIndex());
    event->accept();
}

void HpfHandle::mouseMoveEvent(QMouseEvent* event) {
    if (!m_dragging) return;
    QPoint delta = event->pos() - m_lastEmitPos;
    m_lastEmitPos = event->pos();

    if (!m_throttleTimer.isActive()) {
        emit hpfDragged(delta.x());
        m_throttleTimer.start(16);
        m_hasPending = false;
    } else {
        m_pendingDelta += delta;
        m_hasPending = true;
    }
    event->accept();
}

void HpfHandle::mouseReleaseEvent(QMouseEvent* event) {
    if (!m_dragging) return;
    m_dragging = false;
    m_throttleTimer.stop();
    if (m_hasPending) {
        emit hpfDragged(m_pendingDelta.x());
        m_pendingDelta = QPoint(0, 0);
        m_hasPending = false;
    }
    emit bandReleased(bandIndex());
    event->accept();
}
