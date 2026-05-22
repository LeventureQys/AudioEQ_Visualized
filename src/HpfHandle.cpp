#include "HpfHandle.h"
#include <QMouseEvent>
#include <QPainter>
#include <QtMath>
#include <QCursor>

HpfHandle::HpfHandle(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents, false);
    setMouseTracking(true);
    setFixedSize(kWidgetW, kWidgetH);

    m_dragTimer.setInterval(16);
    m_dragTimer.setSingleShot(false);
    connect(&m_dragTimer, &QTimer::timeout, this, &HpfHandle::processDrag);
}

void HpfHandle::setPosition(const QPoint& viewportPos)
{
    m_centerPos = viewportPos;
    move(viewportPos.x() - kWidgetW / 2, viewportPos.y() - kWidgetH / 2);
}

bool HpfHandle::isDragging() const
{
    return m_dragging;
}

bool HpfHandle::isEnabled() const
{
    return m_enabled;
}

void HpfHandle::setEnabled(bool enabled)
{
    m_enabled = enabled;
    update();
}

QSize HpfHandle::sizeHint() const
{
    return QSize(kWidgetW, kWidgetH);
}

void HpfHandle::mousePressEvent(QMouseEvent* event)
{
    QPoint center(kWidgetW / 2, kWidgetH / 2);
    QPoint diff = event->pos() - center;
    double a = static_cast<double>(kWidth) / 2.0 + kHitMargin;
    double b = static_cast<double>(kHeight) / 2.0 + kHitMargin;
    double ellipseTest = qPow(diff.x() / a, 2) + qPow(diff.y() / b, 2);

    if (ellipseTest <= 1.0) {
        m_dragging = true;
        m_dragStartPos = event->globalPos();
        m_initialCenterX = m_centerPos.x();
        emit hpfPressed();
        m_dragTimer.start();
        event->accept();
    }
}

void HpfHandle::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragging) {
        event->accept();
    }
}

void HpfHandle::mouseReleaseEvent(QMouseEvent* event)
{
    if (m_dragging) {
        m_dragging = false;
        m_dragTimer.stop();
        event->accept();
    }
}

void HpfHandle::processDrag()
{
    QPoint currentPos = QCursor::pos();
    int deltaX = currentPos.x() - m_dragStartPos.x();
    int newX = m_initialCenterX + deltaX;

    emit hpfDragged(newX);
}

void HpfHandle::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QColor fillColor(0x1A, 0x1A, 0x1A);
    QColor borderColor = m_enabled
        ? QColor(0x00, 0xFF, 0x00)
        : QColor::fromRgbF(1.0f, 1.0f, 1.0f, 0.6f);
    QColor textColor = m_enabled
        ? QColor(0x00, 0xFF, 0x00)
        : QColor::fromRgbF(1.0f, 1.0f, 1.0f, 0.9f);

    int cx = kWidgetW / 2;
    int cy = kWidgetH / 2;
    int rx = kWidth / 2;
    int ry = kHeight / 2;

    painter.setBrush(fillColor);
    painter.setPen(QPen(borderColor, 1));
    painter.drawEllipse(QPoint(cx, cy), rx, ry);

    painter.setPen(textColor);
    QFont font = painter.font();
    font.setPixelSize(10);
    painter.setFont(font);
    painter.drawText(QRect(cx - rx, cy - ry, 2 * rx, 2 * ry), Qt::AlignCenter, QStringLiteral("HPF"));
}
