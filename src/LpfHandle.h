#pragma once
#include <QWidget>
#include <QPoint>
#include <QTimer>

#if defined(AUDIOEQ_LIBRARY)
#  define AUDIOEQ_EXPORT Q_DECL_EXPORT
#else
#  define AUDIOEQ_EXPORT Q_DECL_IMPORT
#endif

class AUDIOEQ_EXPORT LpfHandle : public QWidget {
    Q_OBJECT
public:
    explicit LpfHandle(QWidget* parent = nullptr);

    void setPosition(const QPoint& viewportPos);
    bool isDragging() const;
    bool isEnabled() const;
    void setEnabled(bool enabled);

    QSize sizeHint() const override;

signals:
    void lpfDragged(int pixelX);
    void lpfPressed();
    void lpfDeselected();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    bool m_enabled = false;
    bool m_dragging = false;
    QPoint m_centerPos;
    QPoint m_dragStartPos;
    int m_initialCenterX = 0;
    QTimer m_dragTimer;

    static constexpr int kWidth = 36;
    static constexpr int kHeight = 24;
    static constexpr int kHitMargin = 6;
    static constexpr int kWidgetW = kWidth + 2 * kHitMargin;
    static constexpr int kWidgetH = kHeight + 2 * kHitMargin;

    void processDrag();
};
