#include "EqualizerModel.h"
#include <algorithm>

EqualizerModel::EqualizerModel(QObject* parent) : QObject(parent) {
    m_lpf.freqHz  = 20000.0;
    m_lpf.enabled = false;
    m_hpf.freqHz  = 20.0;
    m_hpf.enabled = false;

    m_qRanges[FilterType::Peak]      = {0.1, 10.0};
    m_qRanges[FilterType::LowShelf]  = {0.1, 10.0};
    m_qRanges[FilterType::HighShelf] = {0.1, 10.0};
    m_qRanges[FilterType::LowPass]   = {0.1, 10.0};
    m_qRanges[FilterType::HighPass]  = {0.1, 10.0};
}

// ── Private helpers ────────────────────────────
int EqualizerModel::findFreeIndex() const {
    return static_cast<int>(m_bands.size());
}

bool EqualizerModel::isValidIndex(int index) const {
    return index >= 0 && index < m_bands.size();
}

// ── Band CRUD ──────────────────────────────────
int EqualizerModel::bandCount() const {
    return static_cast<int>(m_bands.size());
}

ResultCode EqualizerModel::addBand(const EQBand& band, int* outIndex) {
    int idx = static_cast<int>(m_bands.size());
    m_bands.append(band);
    if (outIndex) *outIndex = idx;
    emit bandAdded(idx);
    return ResultCode::OK;
}

ResultCode EqualizerModel::removeBand(int index) {
    if (!isValidIndex(index)) return ResultCode::IndexOutOfRange;
    m_bands.removeAt(index);
    if (m_focusedIdx == index) {
        m_focusedIdx = -1;
        emit focusedBandChanged(-1);
    }
    emit bandRemoved(index);
    return ResultCode::OK;
}

EQBand EqualizerModel::band(int index) const {
    if (!isValidIndex(index)) return EQBand{};
    return m_bands.at(index);
}

ResultCode EqualizerModel::setBand(int index, const EQBand& params) {
    if (!isValidIndex(index)) return ResultCode::IndexOutOfRange;
    m_bands[index] = params;
    emit bandChanged(index);
    return ResultCode::OK;
}

ResultCode EqualizerModel::setBandFrequency(int index, double freqHz) {
    if (!isValidIndex(index)) return ResultCode::IndexOutOfRange;
    if (freqHz <= 0.0) return ResultCode::InvalidParameter;
    m_bands[index].freqHz = freqHz;
    emit bandChanged(index);
    return ResultCode::OK;
}

ResultCode EqualizerModel::setBandGain(int index, double gainDb) {
    if (!isValidIndex(index)) return ResultCode::IndexOutOfRange;
    m_bands[index].gainDb = gainDb;
    emit bandChanged(index);
    return ResultCode::OK;
}

ResultCode EqualizerModel::setBandQ(int index, double q) {
    if (!isValidIndex(index)) return ResultCode::IndexOutOfRange;
    if (q <= 0.0) return ResultCode::InvalidParameter;
    m_bands[index].q = q;
    emit bandChanged(index);
    return ResultCode::OK;
}

ResultCode EqualizerModel::setBandType(int index, FilterType type) {
    if (!isValidIndex(index)) return ResultCode::IndexOutOfRange;
    m_bands[index].type = type;
    emit bandChanged(index);
    return ResultCode::OK;
}

ResultCode EqualizerModel::setBandAlgorithm(int index, FilterAlgorithmType algo) {
    if (!isValidIndex(index)) return ResultCode::IndexOutOfRange;
    m_bands[index].algo = algo;
    emit bandChanged(index);
    return ResultCode::OK;
}

ResultCode EqualizerModel::setBandBypass(int index, bool bypass) {
    if (!isValidIndex(index)) return ResultCode::IndexOutOfRange;
    m_bands[index].bypass = bypass;
    emit bandChanged(index);
    return ResultCode::OK;
}

// ── LPF ─────────────────────────────────────────
ShelfBand EqualizerModel::lpf() const { return m_lpf; }

ResultCode EqualizerModel::setLpfEnabled(bool enabled) {
    m_lpf.enabled = enabled;
    emit lpfChanged();
    return ResultCode::OK;
}

ResultCode EqualizerModel::setLpfFrequency(double freqHz) {
    if (freqHz <= 0.0) return ResultCode::InvalidParameter;
    m_lpf.freqHz = freqHz;
    emit lpfChanged();
    return ResultCode::OK;
}

ResultCode EqualizerModel::setLpfAlgorithm(FilterAlgorithmType algo) {
    m_lpf.algo = algo;
    emit lpfChanged();
    return ResultCode::OK;
}

// ── HPF ─────────────────────────────────────────
ShelfBand EqualizerModel::hpf() const { return m_hpf; }

ResultCode EqualizerModel::setHpfEnabled(bool enabled) {
    m_hpf.enabled = enabled;
    emit hpfChanged();
    return ResultCode::OK;
}

ResultCode EqualizerModel::setHpfFrequency(double freqHz) {
    if (freqHz <= 0.0) return ResultCode::InvalidParameter;
    m_hpf.freqHz = freqHz;
    emit hpfChanged();
    return ResultCode::OK;
}

ResultCode EqualizerModel::setHpfAlgorithm(FilterAlgorithmType algo) {
    m_hpf.algo = algo;
    emit hpfChanged();
    return ResultCode::OK;
}

// ── Sample Rate ─────────────────────────────────
SampleRate EqualizerModel::sampleRate() const { return m_sampleRate; }

ResultCode EqualizerModel::setSampleRate(SampleRate rate) {
    m_sampleRate = rate;
    emit sampleRateChanged(rate);
    return ResultCode::OK;
}

// ── Focus ───────────────────────────────────────
int EqualizerModel::focusedBandIndex() const { return m_focusedIdx; }

void EqualizerModel::setFocusedBandIndex(int index) {
    if (m_focusedIdx != index) {
        m_focusedIdx = index;
        emit focusedBandChanged(index);
    }
}

// ── Gain Range ──────────────────────────────────
double EqualizerModel::gainMin() const { return m_gainMin; }
double EqualizerModel::gainMax() const { return m_gainMax; }

ResultCode EqualizerModel::setGainRange(double minDb, double maxDb) {
    if (minDb >= maxDb) return ResultCode::InvalidParameter;
    m_gainMin = minDb;
    m_gainMax = maxDb;
    emit gainRangeChanged(minDb, maxDb);
    return ResultCode::OK;
}

// ── Q Range ─────────────────────────────────────
ResultCode EqualizerModel::setQRange(FilterType type, double minQ, double maxQ) {
    if (minQ <= 0.0 || minQ >= maxQ) return ResultCode::InvalidParameter;
    m_qRanges[type] = {minQ, maxQ};
    for (int i = 0; i < m_bands.size(); ++i) {
        if (m_bands[i].type == type) {
            double clamped = std::clamp(m_bands[i].q, minQ, maxQ);
            if (m_bands[i].q != clamped) {
                m_bands[i].q = clamped;
                emit bandChanged(i);
            }
        }
    }
    return ResultCode::OK;
}

double EqualizerModel::qMin(FilterType type) const {
    return m_qRanges.value(type, {0.1, 10.0}).first;
}

double EqualizerModel::qMax(FilterType type) const {
    return m_qRanges.value(type, {0.1, 10.0}).second;
}
