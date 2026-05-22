#pragma once
#include <QWidget>
#include <memory>
#include "AudioEQTypes.h"

class EqualizerModel;
class CurveEngine;
class FilterAlgorithm;
class ViewEqualizer;
class BandHandle;
class LpfHandle;
class HpfHandle;
class VulkanQtIntegration;
class CoordinateMapper;

class AUDIOEQ_EXPORT AudioEQ : public QWidget {
    Q_OBJECT
public:
    explicit AudioEQ(QWidget* parent = nullptr);
    ~AudioEQ();

    static bool isVulkanSupported();

    int  bandCount() const;
    ResultCode setBandCount(int count);
    ResultCode addBand(const EQBand& band, int* outIndex = nullptr);
    ResultCode removeBand(int index);
    EQBand bandParams(int index) const;
    ResultCode setBandParams(int index, const EQBand& params);

    int  focusedBandIndex() const;
    void setFocusedBandIndex(int index);

    SampleRate sampleRate() const;
    ResultCode setSampleRate(SampleRate rate);

    ResultCode setLpfEnabled(bool enabled);
    bool isLpfEnabled() const;
    ResultCode setHpfEnabled(bool enabled);
    bool isHpfEnabled() const;

    ResultCode setCurvePointCount(int count);

signals:
    void bandChanged(int index);
    void bandAdded(int index);
    void bandRemoved(int index);

private:
    std::unique_ptr<EqualizerModel> m_model;
    std::unique_ptr<CurveEngine> m_curveEngine;
    std::unique_ptr<FilterAlgorithm> m_filterAlgo;
    std::unique_ptr<ViewEqualizer> m_view;
    std::unique_ptr<VulkanQtIntegration> m_vulkan;
    std::unique_ptr<CoordinateMapper> m_mapper;

    void initModel();
    void initEngine();
    void initView();
    void connectSignals();
};
