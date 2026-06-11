#include "BandHandle.h"
#include <QMouseEvent>
#include <QPainter>

BandHandle::BandHandle(int bandIndex, QWidget* parent)
    : QWidget(parent)
    , m_bandIndex(bandIndex)
{
    setAttribute(Qt::WA_TransparentForMouseEvents, false);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setMouseTracking(false);

    m_throttleTimer.setSingleShot(true);
    connect(&m_throttleTimer, &QTimer::timeout, this, &BandHandle::firePendingDrag);
}

void BandHandle::setCenter(QPoint center) {
    m_center = center;
}

QPoint BandHandle::center() const {
    return m_center;
}

int BandHandle::bandIndex() const {
    return m_bandIndex;
}

void BandHandle::paintEvent(QPaintEvent*) {
}

void BandHandle::mousePressEvent(QMouseEvent* event) {
    int dx = event->pos().x() - m_center.x();
    int dy = event->pos().y() - m_center.y();
    int distSq = dx * dx + dy * dy;

    if (distSq > HANDLE_RADIUS * HANDLE_RADIUS) {
        event->ignore();
        return;
    }

    m_dragging = true;
    m_dragStartPos = event->pos();
    m_lastEmitPos = event->pos();
    m_hasPending = false;

    emit bandClicked(m_bandIndex);
    event->accept();
}

void BandHandle::mouseMoveEvent(QMouseEvent* event) {
    if (!m_dragging) return;

    QPoint currentPos = event->pos();
    QPoint delta = currentPos - m_lastEmitPos;

    if (!m_throttleTimer.isActive()) {
        emit bandDragged(m_bandIndex, delta.x(), delta.y());
        m_lastEmitPos = currentPos;
        m_throttleTimer.start(16);
        m_hasPending = false;
    } else {
        m_pendingDelta += (currentPos - m_lastEmitPos);
        m_lastEmitPos = currentPos;
        m_hasPending = true;
    }

    event->accept();
}

void BandHandle::mouseReleaseEvent(QMouseEvent* event) {
    if (!m_dragging) return;

    m_dragging = false;
    m_throttleTimer.stop();

    if (m_hasPending) {
        firePendingDrag();
    }

    emit bandReleased(m_bandIndex);
    event->accept();
}

void BandHandle::firePendingDrag() {
    if (m_hasPending) {
        emit bandDragged(m_bandIndex, m_pendingDelta.x(), m_pendingDelta.y());
        m_pendingDelta = QPoint(0, 0);
        m_hasPending = false;
    }
}
