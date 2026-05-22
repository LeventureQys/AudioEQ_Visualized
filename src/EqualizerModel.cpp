#include "EqualizerModel.h"
#include <cmath>

EqualizerModel::EqualizerModel(QObject* parent)
    : QObject(parent)
{
    setBandCount(5);
}

int EqualizerModel::bandCount() const
{
    return m_bands.size();
}

ResultCode EqualizerModel::setBandCount(int count)
{
    if (count < 0)
        return ResultCode::InvalidParameter;

    m_bands.clear();
    m_bands.reserve(count);

    for (int i = 0; i < count; ++i) {
        EQBand band;
        band.index = i;
        if (count > 1)
            band.frequency = 20.0 * std::pow(20000.0 / 20.0, double(i) / double(count - 1));
        else
            band.frequency = 1000.0;
        band.type = FilterType::Peak;
        band.algorithm = FilterAlgorithmType::ButterworthIIR;
        band.gain = 0.0;
        band.q = 1.0;
        m_bands.append(band);
    }

    m_nextIndex = count;
    if (m_focusedBandIndex >= count)
        m_focusedBandIndex = -1;

    emit bandCountChanged(count);
    emit modelReset();
    return ResultCode::OK;
}

ResultCode EqualizerModel::addBand(const EQBand& band, int* outIndex)
{
    int assignedIndex = m_nextIndex++;
    EQBand newBand = band;
    newBand.index = assignedIndex;
    m_bands.append(newBand);

    if (outIndex)
        *outIndex = assignedIndex;

    emit bandAdded(assignedIndex);
    emit bandCountChanged(m_bands.size());
    return ResultCode::OK;
}

ResultCode EqualizerModel::removeBand(int index)
{
    for (int i = 0; i < m_bands.size(); ++i) {
        if (m_bands[i].index == index) {
            m_bands.removeAt(i);
            if (m_focusedBandIndex == index)
                m_focusedBandIndex = -1;
            emit bandRemoved(index);
            emit bandCountChanged(m_bands.size());
            return ResultCode::OK;
        }
    }
    return ResultCode::IndexOutOfRange;
}

const EQBand& EqualizerModel::bandAt(int index) const
{
    for (const auto& band : m_bands) {
        if (band.index == index)
            return band;
    }
    static EQBand s_invalid;
    return s_invalid;
}

ResultCode EqualizerModel::setBandParams(int index, const EQBand& params)
{
    for (auto& band : m_bands) {
        if (band.index == index) {
            band.frequency  = params.frequency;
            band.gain       = params.gain;
            band.q          = params.q;
            band.type       = params.type;
            band.algorithm  = params.algorithm;
            band.bypass     = params.bypass;
            emit bandChanged(index);
            return ResultCode::OK;
        }
    }
    return ResultCode::IndexOutOfRange;
}

QVector<EQBand> EqualizerModel::allBands() const
{
    return m_bands;
}

int EqualizerModel::focusedBandIndex() const
{
    return m_focusedBandIndex;
}

void EqualizerModel::setFocusedBandIndex(int index)
{
    bool valid = false;
    for (const auto& band : m_bands) {
        if (band.index == index) {
            valid = true;
            break;
        }
    }
    if (index == -1)
        valid = true;

    if (valid) {
        m_focusedBandIndex = index;
        emit focusedBandChanged(index);
    }
}

ResultCode EqualizerModel::moveBandZOrder(int fromIndex, int toIndex)
{
    int fromPos = -1, toPos = -1;
    for (int i = 0; i < m_bands.size(); ++i) {
        if (m_bands[i].index == fromIndex)
            fromPos = i;
        if (m_bands[i].index == toIndex)
            toPos = i;
    }

    if (fromPos < 0 || toPos < 0)
        return ResultCode::IndexOutOfRange;

    m_bands.move(fromPos, toPos);

    for (int i = 0; i < m_bands.size(); ++i)
        m_bands[i].index = i;

    m_nextIndex = m_bands.size();
    if (m_focusedBandIndex == fromIndex)
        m_focusedBandIndex = toPos;
    else if (m_focusedBandIndex == toIndex)
        m_focusedBandIndex = fromPos;

    emit bandChanged(toPos);
    emit bandChanged(fromPos);
    return ResultCode::OK;
}

SampleRate EqualizerModel::sampleRate() const
{
    return m_sampleRate;
}

ResultCode EqualizerModel::setSampleRate(SampleRate rate)
{
    m_sampleRate = rate;
    emit sampleRateChanged(rate);
    return ResultCode::OK;
}

double EqualizerModel::nyquistFrequency() const
{
    return static_cast<double>(static_cast<int>(m_sampleRate)) / 2.0;
}

ShelfBand EqualizerModel::lpf() const
{
    return m_lpf;
}

ResultCode EqualizerModel::setLpf(const ShelfBand& lpf)
{
    m_lpf = lpf;
    emit lpfChanged();
    return ResultCode::OK;
}

ShelfBand EqualizerModel::hpf() const
{
    return m_hpf;
}

ResultCode EqualizerModel::setHpf(const ShelfBand& hpf)
{
    m_hpf = hpf;
    emit hpfChanged();
    return ResultCode::OK;
}

ResultCode EqualizerModel::setLpfEnabled(bool enabled)
{
    m_lpf.enabled = enabled;
    emit lpfChanged();
    return ResultCode::OK;
}

ResultCode EqualizerModel::setHpfEnabled(bool enabled)
{
    m_hpf.enabled = enabled;
    emit hpfChanged();
    return ResultCode::OK;
}
