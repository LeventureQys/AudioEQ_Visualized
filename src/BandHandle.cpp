#include "BandHandle.h"
#include <QMouseEvent>
#include <QPainter>
#include <QtMath>
#include <QCursor>
#include <QGuiApplication>

BandHandle::BandHandle(int bandIndex, QWidget* parent)
    : QWidget(parent)
    , m_bandIndex(bandIndex)
{
    setAttribute(Qt::WA_TransparentForMouseEvents, false);
    setMouseTracking(true);
    setFixedSize(kBandRadius * 2, kBandRadius * 2);

    m_dragTimer.setInterval(16);
    m_dragTimer.setSingleShot(false);
    connect(&m_dragTimer, &QTimer::timeout, this, &BandHandle::processDrag);
}

int BandHandle::bandIndex() const
{
    return m_bandIndex;
}

void BandHandle::setPosition(const QPoint& viewportPos)
{
    m_centerPos = viewportPos;
    move(viewportPos.x() - kBandRadius, viewportPos.y() - kBandRadius);
}

bool BandHandle::isDragging() const
{
    return m_dragging;
}

bool BandHandle::isFocused() const
{
    return m_focused;
}

void BandHandle::setFocused(bool focused)
{
    m_focused = focused;
    update();
}

QSize BandHandle::sizeHint() const
{
    return QSize(kBandRadius * 2, kBandRadius * 2);
}

void BandHandle::mousePressEvent(QMouseEvent* event)
{
    QPoint center(kBandRadius, kBandRadius);
    QPoint diff = event->pos() - center;
    double dist = qSqrt(qPow(diff.x(), 2) + qPow(diff.y(), 2));

    if (dist <= kHitRadius) {
        m_dragging = true;
        m_dragStartPos = event->globalPos();
        emit bandPressed(m_bandIndex);
        m_dragTimer.start();
        event->accept();
    }
}

void BandHandle::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragging) {
        event->accept();
    }
}

void BandHandle::mouseReleaseEvent(QMouseEvent* event)
{
    if (m_dragging) {
        m_dragging = false;
        m_dragTimer.stop();
        event->accept();
    }
}

void BandHandle::processDrag()
{
    QPoint currentPos = QCursor::pos();
    QPoint delta = currentPos - m_dragStartPos;
    m_dragStartPos = currentPos;

    emit bandDragged(m_bandIndex, delta);
}

void BandHandle::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QColor fillColor = m_focused ? QColor(0x3A, 0x3A, 0x3A) : QColor(0x1A, 0x1A, 0x1A);
    QColor borderColor = QColor::fromRgbF(1.0f, 1.0f, 1.0f, 0.9f);

    painter.setBrush(fillColor);
    painter.setPen(QPen(borderColor, 1));
    painter.drawEllipse(QPoint(kBandRadius, kBandRadius), kBandRadius, kBandRadius);
}
