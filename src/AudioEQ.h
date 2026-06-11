#pragma once

#include <QWidget>
#include <QColor>
#include <QPair>
#include <QVector>
#include <QPointF>
#include "AudioEQTypes.h"

#ifndef AUDIOEQ_EXPORT
  #ifdef _WIN32
    #define AUDIOEQ_EXPORT __declspec(dllimport)
  #else
    #define AUDIOEQ_EXPORT
  #endif
#endif

class EqualizerModel;
class CurveEngine;
class CoordinateMapper;
class ViewEqualizer;
class VulkanContext;
class VulkanRenderer;

class AUDIOEQ_EXPORT AudioEQ : public QWidget {
    Q_OBJECT
public:
    static bool isVulkanSupported();

    explicit AudioEQ(QWidget* parent = nullptr);
    ~AudioEQ() override;

    int        bandCount() const;
    ResultCode setBandCount(int count);
    ResultCode addBand(const EQBand& band, int* outIndex = nullptr);
    ResultCode removeBand(int index);
    EQBand     bandParams(int index) const;
    ResultCode setBandParams(int index, const EQBand& params);
    ResultCode setBandFrequency(int index, double freqHz);
    ResultCode setBandGain(int index, double gainDb);
    ResultCode setBandQ(int index, double q);
    ResultCode setBandType(int index, FilterType type);
    ResultCode setBandAlgorithm(int index, FilterAlgorithmType algo);
    ResultCode setBandBypass(int index, bool bypass);

    int  focusedBandIndex() const;
    void setFocusedBandIndex(int index);

    SampleRate sampleRate() const;
    ResultCode setSampleRate(SampleRate rate);

    ResultCode setLpfEnabled(bool enabled);
    bool       isLpfEnabled() const;
    ResultCode setLpfFrequency(double freqHz);
    ResultCode setLpfAlgorithm(FilterAlgorithmType algo);

    ResultCode setHpfEnabled(bool enabled);
    bool       isHpfEnabled() const;
    ResultCode setHpfFrequency(double freqHz);
    ResultCode setHpfAlgorithm(FilterAlgorithmType algo);

    ResultCode setQRange(FilterType type, double minQ, double maxQ);
    QPair<double,double> qRange(FilterType type) const;
    ResultCode setGainRange(double minDb, double maxDb);
    double gainMin() const;
    double gainMax() const;

    void setPointCount(int count);
    int  pointCount() const;

    void setCurveColor(QColor color);
    void setBackgroundColor(QColor color);

signals:
    void bandParamsChanged(int index);
    void focusedBandChanged(int index);
    void sampleRateChanged(SampleRate rate);

private slots:
    void onBandChanged(int index);
    void onBandAdded(int index);
    void onBandRemoved(int index);
    void onLpfChanged();
    void onHpfChanged();
    void onSampleRateChanged(SampleRate rate);
    void onFocusedBandChanged(int index);
    void onGainRangeChanged(double min, double max);

    void onBandDragged(int index, double deltaX, double deltaY);
    void onBandClicked(int index);
    void onBandReleased(int index);
    void onLpfDragged(double deltaX);
    void onHpfDragged(double deltaX);

    void onTotalCurveReady(QVector<QPointF> points);
    void onBandCurveReady(int bandIndex, QVector<QPointF> points);

private:
    void initializeComponents();
    void requestCurveUpdate();
    void syncRendererBands();

    VulkanContext*    m_ctx      = nullptr;
    EqualizerModel*   m_model    = nullptr;
    CurveEngine*      m_engine   = nullptr;
    CoordinateMapper* m_mapper   = nullptr;
    ViewEqualizer*    m_view     = nullptr;
    VulkanRenderer*   m_renderer = nullptr;

    int  m_pointCount  = 500;
    bool m_initialized = false;
};
