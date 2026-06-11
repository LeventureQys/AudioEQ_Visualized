#include "LpfHandle.h"
#include <QMouseEvent>

LpfHandle::LpfHandle(QWidget* parent)
    : BandHandle(-1, parent)
{
}

void LpfHandle::paintEvent(QPaintEvent*) {}

void LpfHandle::mousePressEvent(QMouseEvent* event) {
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

void LpfHandle::mouseMoveEvent(QMouseEvent* event) {
    if (!m_dragging) return;
    QPoint delta = event->pos() - m_lastEmitPos;
    m_lastEmitPos = event->pos();

    if (!m_throttleTimer.isActive()) {
        emit lpfDragged(delta.x());
        m_throttleTimer.start(16);
        m_hasPending = false;
    } else {
        m_pendingDelta += delta;
        m_hasPending = true;
    }
    event->accept();
}

void LpfHandle::mouseReleaseEvent(QMouseEvent* event) {
    if (!m_dragging) return;
    m_dragging = false;
    m_throttleTimer.stop();
    if (m_hasPending) {
        emit lpfDragged(m_pendingDelta.x());
        m_pendingDelta = QPoint(0, 0);
        m_hasPending = false;
    }
    emit bandReleased(bandIndex());
    event->accept();
}
