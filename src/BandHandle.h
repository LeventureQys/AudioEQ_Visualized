#pragma once

#include <QWidget>
#include <QTimer>
#include <QPoint>

#ifndef AUDIOEQ_EXPORT
  #ifdef _WIN32
    #define AUDIOEQ_EXPORT __declspec(dllimport)
  #else
    #define AUDIOEQ_EXPORT
  #endif
#endif

class AUDIOEQ_EXPORT BandHandle : public QWidget {
    Q_OBJECT

public:
    explicit BandHandle(int bandIndex, QWidget* parent = nullptr);

    void setCenter(QPoint center);
    QPoint center() const;

    int bandIndex() const;
    bool isDragging() const;

    static constexpr int HANDLE_RADIUS = 20;

signals:
    void bandDragged(int bandIndex, double deltaX, double deltaY);
    void bandClicked(int bandIndex);
    void bandReleased(int bandIndex);

protected:
    void mousePressEvent(QMouseEvent* event)   override;
    void mouseMoveEvent(QMouseEvent* event)    override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event)        override;

    void firePendingDrag();

    QPoint  m_lastGlobalPos;
    bool    m_dragging    = false;
    QTimer  m_throttleTimer;
    QPoint  m_pendingDelta;
    bool    m_hasPending  = false;

private:
    int     m_bandIndex;
    QPoint  m_center;
    QPoint  m_dragStartPos;
};
