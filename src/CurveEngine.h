#pragma once

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QVector>
#include <QPointF>
#include <atomic>
#include <memory>
#include "AudioEQTypes.h"

class FilterAlgorithm;

struct CurveRequest {
    QVector<EQBand> bands;
    ShelfBand       lpf;
    ShelfBand       hpf;
    SampleRate      sampleRate = SampleRate::SR_44100;
    double          freqMin    = 20.0;
    double          freqMax    = 22050.0;
    int             pointCount = 500;
};

class AUDIOEQ_EXPORT CurveEngine : public QObject {
    Q_OBJECT
public:
    explicit CurveEngine(QObject* parent = nullptr);
    ~CurveEngine() override;

    void setPointCount(int count);

    void requestTotalCurve(const CurveRequest& request);

    void requestBandCurve(const CurveRequest& request, int bandIndex);

    void cancelPending();

signals:
    void totalCurveReady(QVector<QPointF> points);
    void bandCurveReady(int bandIndex, QVector<QPointF> points);

private:
    void startWorker();
    void stopWorker();
    void workerLoop();
    QVector<QPointF> computeTotalCurve(const CurveRequest& req);
    QVector<QPointF> computeBandCurve(const CurveRequest& req, int bandIndex);

    QThread             m_workerThread;
    QMutex              m_mutex;
    QWaitCondition      m_condition;
    bool                m_running = false;

    bool                m_hasTask = false;
    bool                m_isTotalCurve = true;
    CurveRequest        m_request;
    int                 m_bandIndex = -1;

    std::atomic<bool>   m_cancelPending{false};
    int                 m_pointCount = 500;
};
