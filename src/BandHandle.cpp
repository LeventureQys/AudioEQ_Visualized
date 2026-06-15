#include "BandHandle.h"
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>

BandHandle::BandHandle(int bandIndex, QWidget* parent)
    : QWidget(parent)
    , m_bandIndex(bandIndex)
{
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_ShowWithoutActivating, true);
    setAttribute(Qt::WA_TransparentForMouseEvents, false);
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

bool BandHandle::isDragging() const {
    return m_dragging;
}

void BandHandle::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const int r = HANDLE_RADIUS - 2;            // inset 2px for stroke
    const QPoint c(width() / 2, height() / 2);
    QRect circle(c.x() - r, c.y() - r, 2 * r, 2 * r);

    // Filled circle (dark fill, similar to target sample)
    p.setBrush(QColor(0x3D, 0x44, 0x4F, 235));   // #3D444F, slightly translucent
    p.setPen(QPen(QColor(0x9A, 0xA2, 0xB0, 255), 1.4));
    p.drawEllipse(circle);

    // Index label centered
    p.setPen(QColor(0xE0, 0xE4, 0xEC, 255));
    QFont f = p.font();
    f.setPointSizeF(9.0);
    f.setBold(true);
    p.setFont(f);
    p.drawText(circle, Qt::AlignCenter, QString::number(m_bandIndex));
}

void BandHandle::mousePressEvent(QMouseEvent* event) {
    int dx = event->pos().x() - width() / 2;
    int dy = event->pos().y() - height() / 2;
    int distSq = dx * dx + dy * dy;

    if (distSq > HANDLE_RADIUS * HANDLE_RADIUS) {
        event->ignore();
        return;
    }

    m_dragging = true;
    m_dragStartPos = event->pos();
    m_lastGlobalPos = event->globalPos();
    m_hasPending = false;

    emit bandClicked(m_bandIndex);
    event->accept();
}

void BandHandle::mouseMoveEvent(QMouseEvent* event) {
    if (!m_dragging) return;

    QPoint globalCurrent = event->globalPos();
    QPoint delta = globalCurrent - m_lastGlobalPos;

    if (delta.x() == 0 && delta.y() == 0) return;

    m_lastGlobalPos = globalCurrent;

    if (!m_throttleTimer.isActive()) {
        QPoint curGlobal = mapToGlobal(QPoint(0, 0));
        move(curGlobal.x() + delta.x(), curGlobal.y() + delta.y());
        emit bandDragged(m_bandIndex, delta.x(), delta.y());
        m_throttleTimer.start(16);
        m_hasPending = false;
    } else {
        m_pendingDelta += delta;
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
        QPoint curGlobal = mapToGlobal(QPoint(0, 0));
        move(curGlobal.x() + m_pendingDelta.x(), curGlobal.y() + m_pendingDelta.y());
        emit bandDragged(m_bandIndex, m_pendingDelta.x(), m_pendingDelta.y());
        m_pendingDelta = QPoint(0, 0);
        m_hasPending = false;
    }
}
