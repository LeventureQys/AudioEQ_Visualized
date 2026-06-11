#pragma once

#include <QWidget>
#include <QVector>
#include "AudioEQTypes.h"

class VulkanContext;
class VulkanRenderer;
class VulkanQtWindow;
class CoordinateMapper;
class EqualizerModel;
class BandHandle;
class LpfHandle;
class HpfHandle;

#ifndef AUDIOEQ_EXPORT
  #ifdef _WIN32
    #define AUDIOEQ_EXPORT __declspec(dllimport)
  #else
    #define AUDIOEQ_EXPORT
  #endif
#endif

class AUDIOEQ_EXPORT ViewEqualizer : public QWidget {
    Q_OBJECT
public:
    explicit ViewEqualizer(QWidget* parent = nullptr);
    ~ViewEqualizer() override;

    void setVulkanContext(VulkanContext* ctx);
    void setRenderer(VulkanRenderer* renderer);
    void setCoordinateMapper(CoordinateMapper* mapper);
    void setModel(EqualizerModel* model);

    void syncBandHandles(int count);
    void updateBandHandlePosition(int index, QPoint pos);

    void setLpfHandleVisible(bool visible);
    void setHpfHandleVisible(bool visible);

    VulkanRenderer* vulkanRenderer() const;

    static constexpr int HANDLE_RADIUS = 20;

signals:
    void bandDragged(int index, double deltaX, double deltaY);
    void bandClicked(int index);
    void bandReleased(int index);
    void lpfDragged(double deltaX);
    void hpfDragged(double deltaX);

protected:
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void onBandChanged(int index);
    void onBandAdded(int index);
    void onBandRemoved(int index);
    void onLpfChanged();
    void onHpfChanged();
    void onSampleRateChanged(SampleRate rate);

private:
    void createVulkanWindow();
    void repositionAllHandles();
    void connectSignals();

    VulkanContext*      m_ctx      = nullptr;
    VulkanRenderer*     m_renderer = nullptr;
    CoordinateMapper*   m_mapper   = nullptr;
    EqualizerModel*     m_model    = nullptr;

    VulkanQtWindow*     m_window           = nullptr;
    QWidget*            m_vulkanContainer  = nullptr;

    QVector<BandHandle*> m_bandHandles;
    LpfHandle*           m_lpfHandle = nullptr;
    HpfHandle*           m_hpfHandle = nullptr;

    bool m_initialized = false;
};
