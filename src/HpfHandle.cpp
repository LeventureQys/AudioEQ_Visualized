#include "HpfHandle.h"
#include <QMouseEvent>
#include <QPainter>

HpfHandle::HpfHandle(QWidget* parent)
    : BandHandle(-2, parent)
{
}

void HpfHandle::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const QPoint c(width() / 2, height() / 2);
    QRect ellipse(c.x() - ELLIPSE_RX, c.y() - ELLIPSE_RY, 2 * ELLIPSE_RX, 2 * ELLIPSE_RY);

    p.setBrush(QColor(0x3D, 0x44, 0x4F, 235));
    p.setPen(QPen(QColor(0x9A, 0xA2, 0xB0, 255), 1.4));
    p.drawEllipse(ellipse);

    p.setPen(QColor(0xE0, 0xE4, 0xEC, 255));
    QFont f = p.font();
    f.setPointSizeF(8.0);
    f.setBold(true);
    p.setFont(f);
    p.drawText(ellipse, Qt::AlignCenter, QStringLiteral("HPF"));
}

void HpfHandle::mousePressEvent(QMouseEvent* event) {
    double dx = event->pos().x() - width() / 2.0;
    double dy = event->pos().y() - height() / 2.0;
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
