#include "CurveEngine.h"
#include "filter/FilterAlgorithm.h"
#include "filter/FilterAlgorithmFactory.h"
#include <cmath>
#include <algorithm>

CurveEngine::CurveEngine(QObject* parent) : QObject(parent) {
    startWorker();
}

CurveEngine::~CurveEngine() {
    stopWorker();
}

void CurveEngine::setPointCount(int count) {
    if (count > 0) m_pointCount = count;
}

void CurveEngine::requestTotalCurve(const CurveRequest& request) {
    cancelPending();
    {
        QMutexLocker lock(&m_mutex);
        m_request = request;
        m_request.pointCount = m_pointCount;
        m_isTotalCurve = true;
        m_hasTask = true;
    }
    m_condition.wakeOne();
}

void CurveEngine::requestBandCurve(const CurveRequest& request, int bandIndex) {
    cancelPending();
    {
        QMutexLocker lock(&m_mutex);
        m_request = request;
        m_request.pointCount = m_pointCount;
        m_isTotalCurve = false;
        m_bandIndex = bandIndex;
        m_hasTask = true;
    }
    m_condition.wakeOne();
}

void CurveEngine::cancelPending() {
    m_cancelPending.store(true, std::memory_order_release);
}

void CurveEngine::startWorker() {
    m_running = true;
    QObject::connect(&m_workerThread, &QThread::started, this, [this]() {
        workerLoop();
    });
    moveToThread(&m_workerThread);
    m_workerThread.start();
}

void CurveEngine::stopWorker() {
    m_running = false;
    cancelPending();
    m_condition.wakeOne();
    m_workerThread.quit();
    m_workerThread.wait();
}

void CurveEngine::workerLoop() {
    while (m_running) {
        {
            QMutexLocker lock(&m_mutex);
            while (m_running && !m_hasTask) {
                m_condition.wait(&m_mutex);
            }
            if (!m_running) break;
        }

        m_cancelPending.store(false, std::memory_order_release);

        CurveRequest req;
        bool isTotal;
        int bandIdx;
        {
            QMutexLocker lock(&m_mutex);
            req = m_request;
            isTotal = m_isTotalCurve;
            bandIdx = m_bandIndex;
            m_hasTask = false;
        }

        QVector<QPointF> points;
        if (isTotal) {
            points = computeTotalCurve(req);
            if (!m_cancelPending.load(std::memory_order_acquire)) {
                emit totalCurveReady(points);
            }
        } else {
            points = computeBandCurve(req, bandIdx);
            if (!m_cancelPending.load(std::memory_order_acquire)) {
                emit bandCurveReady(bandIdx, points);
            }
        }
    }
}

QVector<QPointF> CurveEngine::computeTotalCurve(const CurveRequest& req) {
    QVector<QPointF> points;
    points.reserve(req.pointCount);

    double logMin = std::log(req.freqMin);
    double logMax = std::log(req.freqMax);
    double logRange = logMax - logMin;

    for (int i = 0; i < req.pointCount; ++i) {
        if (i % 10 == 0 && m_cancelPending.load(std::memory_order_acquire))
            return {};

        double t = static_cast<double>(i) / (req.pointCount - 1);
        double freq = req.freqMin * std::exp(t * logRange);

        double x = (std::log(freq) - logMin) / logRange;

        double totalGain = 0.0;
        for (const auto& band : req.bands) {
            if (band.bypass) continue;
            auto algo = FilterAlgorithmFactory::create(band.type, band.algo, band.freqHz, band.gainDb, band.q);
            if (algo) {
                totalGain += algo->evaluateAt(freq, static_cast<double>(static_cast<int>(req.sampleRate)));
            }
        }

        if (req.lpf.enabled && !req.lpf.bypass) {
            auto algo = FilterAlgorithmFactory::create(FilterType::LowPass, req.lpf.algo, req.lpf.freqHz, 0.0, 0.7071);
            if (algo) totalGain += algo->evaluateAt(freq, static_cast<double>(static_cast<int>(req.sampleRate)));
        }

        if (req.hpf.enabled && !req.hpf.bypass) {
            auto algo = FilterAlgorithmFactory::create(FilterType::HighPass, req.hpf.algo, req.hpf.freqHz, 0.0, 0.7071);
            if (algo) totalGain += algo->evaluateAt(freq, static_cast<double>(static_cast<int>(req.sampleRate)));
        }

        points.append(QPointF(x, totalGain));
    }

    return points;
}

QVector<QPointF> CurveEngine::computeBandCurve(const CurveRequest& req, int bandIndex) {
    QVector<QPointF> points;
    points.reserve(req.pointCount);

    if (bandIndex < 0 || bandIndex >= req.bands.size()) return points;

    const EQBand& band = req.bands[bandIndex];

    double logMin = std::log(req.freqMin);
    double logMax = std::log(req.freqMax);
    double logRange = logMax - logMin;

    auto algo = FilterAlgorithmFactory::create(band.type, band.algo, band.freqHz, band.gainDb, band.q);
    if (!algo) return points;

    for (int i = 0; i < req.pointCount; ++i) {
        if (i % 10 == 0 && m_cancelPending.load(std::memory_order_acquire))
            return {};

        double t = static_cast<double>(i) / (req.pointCount - 1);
        double freq = req.freqMin * std::exp(t * logRange);
        double x = (std::log(freq) - logMin) / logRange;
        double gain = algo->evaluateAt(freq, static_cast<double>(static_cast<int>(req.sampleRate)));
        points.append(QPointF(x, gain));
    }

    return points;
}
