#pragma once

#include <QObject>
#include <QVector>
#include <QMap>
#include <QPair>
#include "AudioEQTypes.h"

#ifndef AUDIOEQ_EXPORT
  #ifdef _WIN32
    #define AUDIOEQ_EXPORT __declspec(dllimport)
  #else
    #define AUDIOEQ_EXPORT
  #endif
#endif

class AUDIOEQ_EXPORT EqualizerModel : public QObject {
    Q_OBJECT

public:
    explicit EqualizerModel(QObject* parent = nullptr);

    // ── Band CRUD ──────────────────────────────
    int  bandCount() const;
    ResultCode addBand(const EQBand& band, int* outIndex = nullptr);
    ResultCode removeBand(int index);
    EQBand band(int index) const;
    ResultCode setBand(int index, const EQBand& params);
    ResultCode setBandFrequency(int index, double freqHz);
    ResultCode setBandGain(int index, double gainDb);
    ResultCode setBandQ(int index, double q);
    ResultCode setBandType(int index, FilterType type);
    ResultCode setBandAlgorithm(int index, FilterAlgorithmType algo);
    ResultCode setBandBypass(int index, bool bypass);

    // ── LPF ─────────────────────────────────────
    ShelfBand lpf() const;
    ResultCode setLpfEnabled(bool enabled);
    ResultCode setLpfFrequency(double freqHz);
    ResultCode setLpfAlgorithm(FilterAlgorithmType algo);

    // ── HPF ─────────────────────────────────────
    ShelfBand hpf() const;
    ResultCode setHpfEnabled(bool enabled);
    ResultCode setHpfFrequency(double freqHz);
    ResultCode setHpfAlgorithm(FilterAlgorithmType algo);

    // ── Sample Rate ─────────────────────────────
    SampleRate sampleRate() const;
    ResultCode setSampleRate(SampleRate rate);

    // ── Focus ───────────────────────────────────
    int  focusedBandIndex() const;          // -1 = no focus
    void setFocusedBandIndex(int index);

    // ── Gain Range ──────────────────────────────
    double gainMin() const;
    double gainMax() const;
    ResultCode setGainRange(double minDb, double maxDb);

    // ── Q Range (per FilterType) ────────────────
    ResultCode setQRange(FilterType type, double minQ, double maxQ);
    double qMin(FilterType type) const;
    double qMax(FilterType type) const;

signals:
    void bandChanged(int index);
    void bandAdded(int index);
    void bandRemoved(int index);
    void lpfChanged();
    void hpfChanged();
    void sampleRateChanged(SampleRate rate);
    void focusedBandChanged(int index);
    void gainRangeChanged(double minDb, double maxDb);

private:
    int findFreeIndex() const;
    bool isValidIndex(int index) const;

    QVector<EQBand>  m_bands;
    ShelfBand        m_lpf;
    ShelfBand        m_hpf;
    SampleRate       m_sampleRate = SampleRate::SR_44100;
    int              m_focusedIdx = -1;
    double           m_gainMin    = -18.0;
    double           m_gainMax    = +12.0;
    QMap<FilterType, QPair<double, double>> m_qRanges;
};
