#pragma once

#include "BandHandle.h"

#ifndef AUDIOEQ_EXPORT
  #ifdef _WIN32
    #define AUDIOEQ_EXPORT __declspec(dllimport)
  #else
    #define AUDIOEQ_EXPORT
  #endif
#endif

class AUDIOEQ_EXPORT HpfHandle : public BandHandle {
    Q_OBJECT
public:
    explicit HpfHandle(QWidget* parent = nullptr);

    static constexpr int ELLIPSE_RX = 20;
    static constexpr int ELLIPSE_RY = 12;

signals:
    void hpfDragged(double deltaX);

protected:
    void mousePressEvent(QMouseEvent* event)   override;
    void mouseMoveEvent(QMouseEvent* event)    override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event)        override;
};
