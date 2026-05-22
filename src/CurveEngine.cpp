#include "CurveEngine.h"
#include "EqualizerModel.h"
#include "CoordinateMapper.h"
#include "filter/FilterAlgorithm.h"
#include <cmath>
#include <algorithm>

CurveEngine::CurveEngine(FilterAlgorithm* algo, QObject* parent)
	: QObject(parent)
	, m_filterAlgo(algo)
{
	moveToThread(&m_workerThread);
	m_workerThread.start();
}

CurveEngine::~CurveEngine()
{
	cancelPending();
	m_workerThread.quit();
	m_workerThread.wait();
}

void CurveEngine::setPointCount(int count)
{
	if (count < 1)
		count = 1;
	m_pointCount = count;
}

int CurveEngine::pointCount() const
{
	return m_pointCount;
}

void CurveEngine::setFreqRange(double minHz, double maxHz)
{
	m_freqMin = minHz;
	m_freqMax = maxHz;
}

void CurveEngine::setGainRange(double minDb, double maxDb)
{
	m_gainMin = minDb;
	m_gainMax = maxDb;
}

void CurveEngine::requestTotalCurve(const EqualizerModel& modelSnapshot)
{
	QVector<EQBand> bands = modelSnapshot.allBands();
	ShelfBand lpf = modelSnapshot.lpf();
	ShelfBand hpf = modelSnapshot.hpf();
	double sr = static_cast<double>(static_cast<int>(modelSnapshot.sampleRate()));

	QMetaObject::invokeMethod(this, [this, bands, lpf, hpf, sr]() {
		if (m_cancelPending.load())
			return;
		auto points = computeTotalCurveImpl(bands, lpf, hpf, sr);
		if (!m_cancelPending.load())
			emit totalCurveReady(points);
	}, Qt::QueuedConnection);
}

void CurveEngine::requestSingleBandCurve(int bandIndex, const EqualizerModel& modelSnapshot)
{
	QVector<EQBand> bands = modelSnapshot.allBands();
	ShelfBand lpf = modelSnapshot.lpf();
	ShelfBand hpf = modelSnapshot.hpf();
	double sr = static_cast<double>(static_cast<int>(modelSnapshot.sampleRate()));

	QMetaObject::invokeMethod(this, [this, bandIndex, bands, lpf, hpf, sr]() {
		if (m_cancelPending.load())
			return;
		auto points = computeSingleBandCurveImpl(bandIndex, bands, lpf, hpf, sr);
		if (!m_cancelPending.load())
			emit singleBandCurveReady(bandIndex, points);
	}, Qt::QueuedConnection);
}

void CurveEngine::cancelPending()
{
	m_cancelPending.store(true);
}

QVector<double> CurveEngine::generateLogFrequencyPoints() const
{
	QVector<double> points;
	points.reserve(m_pointCount);

	if (m_pointCount == 1) {
		points.append(m_freqMin);
		return points;
	}

	double ratio = m_freqMax / m_freqMin;
	for (int i = 0; i < m_pointCount; ++i) {
		double t = static_cast<double>(i) / static_cast<double>(m_pointCount - 1);
		double freq = m_freqMin * std::pow(ratio, t);
		points.append(freq);
	}

	return points;
}

QVector<QPointF> CurveEngine::computeTotalCurveImpl(const QVector<EQBand>& bands, const ShelfBand& lpf,
                                                      const ShelfBand& hpf, double sr) const
{
	QVector<double> freqs = generateLogFrequencyPoints();
	QVector<QPointF> result;
	result.reserve(freqs.size());

	CoordinateMapper mapper(QRect(0, 0, 1, 1), m_freqMin, m_freqMax, m_gainMin, m_gainMax);

	int checkInterval = 16;
	for (int i = 0; i < freqs.size(); ++i) {
		if ((i % checkInterval) == 0 && m_cancelPending.load())
			return QVector<QPointF>();

		double freq = freqs[i];
		double gain = 0.0;

		for (const auto& band : bands) {
			if (!band.bypass)
				gain += m_filterAlgo->evaluateAt(freq, sr, band);
		}

		if (lpf.enabled) {
			EQBand lpfBand;
			lpfBand.type = FilterType::LowPass;
			lpfBand.frequency = lpf.frequency;
			lpfBand.algorithm = lpf.algorithm;
			lpfBand.gain = 0.0;
			lpfBand.q = 1.0;
			lpfBand.bypass = false;
			gain += m_filterAlgo->evaluateAt(freq, sr, lpfBand);
		}

		if (hpf.enabled) {
			EQBand hpfBand;
			hpfBand.type = FilterType::HighPass;
			hpfBand.frequency = hpf.frequency;
			hpfBand.algorithm = hpf.algorithm;
			hpfBand.gain = 0.0;
			hpfBand.q = 1.0;
			hpfBand.bypass = false;
			gain += m_filterAlgo->evaluateAt(freq, sr, hpfBand);
		}

		double clampedGain = std::clamp(gain, m_gainMin, m_gainMax);
		double x = mapper.freqToX(freq);
		double y = mapper.gainToY(clampedGain);
		result.append(QPointF(x, y));
	}

	return result;
}

QVector<QPointF> CurveEngine::computeSingleBandCurveImpl(int bandIndex, const QVector<EQBand>& bands,
                                                           const ShelfBand& lpf, const ShelfBand& hpf,
                                                           double sr) const
{
	QVector<double> freqs = generateLogFrequencyPoints();
	QVector<QPointF> result;
	result.reserve(freqs.size());

	CoordinateMapper mapper(QRect(0, 0, 1, 1), m_freqMin, m_freqMax, m_gainMin, m_gainMax);

	const EQBand* targetBand = nullptr;
	for (const auto& band : bands) {
		if (band.index == bandIndex) {
			targetBand = &band;
			break;
		}
	}

	if (targetBand && targetBand->bypass) {
		double yZero = mapper.gainToY(0.0);
		for (int i = 0; i < freqs.size(); ++i) {
			double x = mapper.freqToX(freqs[i]);
			result.append(QPointF(x, yZero));
		}
		return result;
	}

	if (bandIndex == -2) {
		if (!lpf.enabled) {
			double yZero = mapper.gainToY(0.0);
			for (int i = 0; i < freqs.size(); ++i) {
				double x = mapper.freqToX(freqs[i]);
				result.append(QPointF(x, yZero));
			}
			return result;
		}
		for (int i = 0; i < freqs.size(); ++i) {
			double freq = freqs[i];
			EQBand lpfBand;
			lpfBand.type = FilterType::LowPass;
			lpfBand.frequency = lpf.frequency;
			lpfBand.algorithm = lpf.algorithm;
			lpfBand.gain = 0.0;
			lpfBand.q = 1.0;
			lpfBand.bypass = false;
			double gain = m_filterAlgo->evaluateAt(freq, sr, lpfBand);
			double clampedGain = std::clamp(gain, m_gainMin, m_gainMax);
			double x = mapper.freqToX(freq);
			double y = mapper.gainToY(clampedGain);
			result.append(QPointF(x, y));
		}
		return result;
	}

	if (bandIndex == -1) {
		if (!hpf.enabled) {
			double yZero = mapper.gainToY(0.0);
			for (int i = 0; i < freqs.size(); ++i) {
				double x = mapper.freqToX(freqs[i]);
				result.append(QPointF(x, yZero));
			}
			return result;
		}
		for (int i = 0; i < freqs.size(); ++i) {
			double freq = freqs[i];
			EQBand hpfBand;
			hpfBand.type = FilterType::HighPass;
			hpfBand.frequency = hpf.frequency;
			hpfBand.algorithm = hpf.algorithm;
			hpfBand.gain = 0.0;
			hpfBand.q = 1.0;
			hpfBand.bypass = false;
			double gain = m_filterAlgo->evaluateAt(freq, sr, hpfBand);
			double clampedGain = std::clamp(gain, m_gainMin, m_gainMax);
			double x = mapper.freqToX(freq);
			double y = mapper.gainToY(clampedGain);
			result.append(QPointF(x, y));
		}
		return result;
	}

	if (!targetBand) {
		double yZero = mapper.gainToY(0.0);
		for (int i = 0; i < freqs.size(); ++i) {
			double x = mapper.freqToX(freqs[i]);
			result.append(QPointF(x, yZero));
		}
		return result;
	}

	int checkInterval = 16;
	for (int i = 0; i < freqs.size(); ++i) {
		if ((i % checkInterval) == 0 && m_cancelPending.load())
			return QVector<QPointF>();

		double freq = freqs[i];
		double gain = m_filterAlgo->evaluateAt(freq, sr, *targetBand);
		double clampedGain = std::clamp(gain, m_gainMin, m_gainMax);
		double x = mapper.freqToX(freq);
		double y = mapper.gainToY(clampedGain);
		result.append(QPointF(x, y));
	}

	return result;
}
