#pragma once
#include <QObject>
#include <QVector>
#include "AudioEQTypes.h"

class AUDIOEQ_EXPORT EqualizerModel : public QObject {
    Q_OBJECT
public:
    explicit EqualizerModel(QObject* parent = nullptr);

    int  bandCount() const;
    ResultCode setBandCount(int count);

    ResultCode addBand(const EQBand& band, int* outIndex = nullptr);
    ResultCode removeBand(int index);

    const EQBand& bandAt(int index) const;
    ResultCode setBandParams(int index, const EQBand& params);
    QVector<EQBand> allBands() const;

    int  focusedBandIndex() const;
    void setFocusedBandIndex(int index);
    ResultCode moveBandZOrder(int fromIndex, int toIndex);

    SampleRate sampleRate() const;
    ResultCode setSampleRate(SampleRate rate);
    double nyquistFrequency() const;

    ShelfBand lpf() const;
    ResultCode setLpf(const ShelfBand& lpf);
    ShelfBand hpf() const;
    ResultCode setHpf(const ShelfBand& hpf);

    ResultCode setLpfEnabled(bool enabled);
    ResultCode setHpfEnabled(bool enabled);

signals:
    void bandChanged(int index);
    void bandAdded(int index);
    void bandRemoved(int index);
    void bandCountChanged(int newCount);
    void focusedBandChanged(int index);
    void sampleRateChanged(SampleRate rate);
    void lpfChanged();
    void hpfChanged();
    void modelReset();

private:
    QVector<EQBand> m_bands;
    int m_focusedBandIndex = -1;
    SampleRate m_sampleRate = SampleRate::SR_44100;
    ShelfBand m_lpf;
    ShelfBand m_hpf;
    int m_nextIndex = 0;
};
