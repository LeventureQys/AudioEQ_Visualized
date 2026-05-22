#pragma once
#include <QObject>
#include <QThread>
#include <QVector>
#include <QPointF>
#include <atomic>
#include "AudioEQTypes.h"

class FilterAlgorithm;
class EqualizerModel;

class AUDIOEQ_EXPORT CurveEngine : public QObject {
    Q_OBJECT
public:
    explicit CurveEngine(FilterAlgorithm* algo, QObject* parent = nullptr);
    ~CurveEngine();

    void setPointCount(int count);
    int  pointCount() const;
    void setFreqRange(double minHz, double maxHz);
    void setGainRange(double minDb, double maxDb);

    void requestTotalCurve(const EqualizerModel& modelSnapshot);
    void requestSingleBandCurve(int bandIndex, const EqualizerModel& modelSnapshot);
    void cancelPending();

signals:
    void totalCurveReady(QVector<QPointF> points);
    void singleBandCurveReady(int bandIndex, QVector<QPointF> points);

private:
    QThread m_workerThread;
    FilterAlgorithm* m_filterAlgo = nullptr;
    std::atomic<bool> m_cancelPending{false};
    int m_pointCount = 500;
    double m_freqMin = 10.0, m_freqMax = 24000.0;
    double m_gainMin = -48.0, m_gainMax = 48.0;

    QVector<double> generateLogFrequencyPoints() const;
    QVector<QPointF> computeTotalCurveImpl(const QVector<EQBand>& bands, const ShelfBand& lpf,
                                            const ShelfBand& hpf, double sr) const;
    QVector<QPointF> computeSingleBandCurveImpl(int bandIndex, const QVector<EQBand>& bands,
                                                 const ShelfBand& lpf, const ShelfBand& hpf,
                                                 double sr) const;
};
