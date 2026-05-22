#pragma once
#include <QWidget>
#include <QPoint>
#include <QTimer>

#if defined(AUDIOEQ_LIBRARY)
#  define AUDIOEQ_EXPORT Q_DECL_EXPORT
#else
#  define AUDIOEQ_EXPORT Q_DECL_IMPORT
#endif

class AUDIOEQ_EXPORT BandHandle : public QWidget {
    Q_OBJECT
public:
    explicit BandHandle(int bandIndex, QWidget* parent = nullptr);

    int bandIndex() const;
    void setPosition(const QPoint& viewportPos);

    bool isDragging() const;
    bool isFocused() const;
    void setFocused(bool focused);

    QSize sizeHint() const override;

signals:
    void bandDragged(int index, QPoint deltaPixels);
    void bandPressed(int index);
    void bandDeselected();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    int m_bandIndex;
    bool m_focused = false;
    bool m_dragging = false;
    QPoint m_centerPos;
    QPoint m_dragStartPos;
    QTimer m_dragTimer;

    static constexpr int kBandRadius = 12;
    static constexpr int kHitRadius = 16;

    void processDrag();
};
