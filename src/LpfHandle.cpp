#include "LpfHandle.h"
#include <QMouseEvent>
#include <QPainter>

LpfHandle::LpfHandle(QWidget* parent)
    : BandHandle(-1, parent)
{
}

void LpfHandle::paintEvent(QPaintEvent*) {
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
    p.drawText(ellipse, Qt::AlignCenter, QStringLiteral("LPF"));
}

void LpfHandle::mousePressEvent(QMouseEvent* event) {
    double dx = event->pos().x() - width() / 2.0;
    double dy = event->pos().y() - height() / 2.0;
    double hx = dx / ELLIPSE_RX;
    double hy = dy / ELLIPSE_RY;
    if (hx * hx + hy * hy > 1.0) {
        event->ignore();
        return;
    }
    m_dragging = true;
    m_lastGlobalPos = event->globalPos();
    m_hasPending = false;
    emit bandClicked(bandIndex());
    event->accept();
}

void LpfHandle::mouseMoveEvent(QMouseEvent* event) {
    if (!m_dragging) return;
    QPoint globalCurrent = event->globalPos();
    QPoint delta = globalCurrent - m_lastGlobalPos;

    if (delta.x() == 0) return;

    if (!m_throttleTimer.isActive()) {
        QPoint curGlobal = mapToGlobal(QPoint(0, 0));
        move(curGlobal.x() + delta.x(), curGlobal.y() + delta.y());
        emit lpfDragged(delta.x());
        m_lastGlobalPos = globalCurrent;
        m_throttleTimer.start(16);
        m_hasPending = false;
    } else {
        m_pendingDelta += delta;
        m_lastGlobalPos = globalCurrent;
        m_hasPending = true;
    }
    event->accept();
}

void LpfHandle::mouseReleaseEvent(QMouseEvent* event) {
    if (!m_dragging) return;
    m_dragging = false;
    m_throttleTimer.stop();
    if (m_hasPending) {
        QPoint curGlobal = mapToGlobal(QPoint(0, 0));
        move(curGlobal.x() + m_pendingDelta.x(), curGlobal.y() + m_pendingDelta.y());
        emit lpfDragged(m_pendingDelta.x());
        m_pendingDelta = QPoint(0, 0);
        m_hasPending = false;
    }
    emit bandReleased(bandIndex());
    event->accept();
}
