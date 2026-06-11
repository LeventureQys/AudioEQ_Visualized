#include "CoordinateMapper.h"
#include <cmath>
#include <algorithm>

CoordinateMapper::CoordinateMapper() = default;

void CoordinateMapper::setViewport(const QRectF& rect)
{
    m_viewport = rect;
}

QRectF CoordinateMapper::viewport() const
{
    return m_viewport;
}

void CoordinateMapper::setSampleRate(SampleRate rate)
{
    m_freqMax = static_cast<int>(rate) / 2.0;
}

double CoordinateMapper::freqToX(double freqHz) const
{
    double freq = std::clamp(freqHz, m_freqMin, m_freqMax);
    double logRatio = std::log(m_freqMax / m_freqMin);
    double t = std::log(freq / m_freqMin) / logRatio;
    return m_viewport.left() + t * m_viewport.width();
}

double CoordinateMapper::xToFreq(double x) const
{
    double cx = std::clamp(x, m_viewport.left(), m_viewport.right());
    double t = (cx - m_viewport.left()) / m_viewport.width();
    double logFreq = std::log(m_freqMin) + t * std::log(m_freqMax / m_freqMin);
    return std::exp(logFreq);
}

double CoordinateMapper::gainToY(double gainDb) const
{
    double gain = std::clamp(gainDb, m_gainMin, m_gainMax);
    double range = m_gainMax - m_gainMin;
    double t = (m_gainMax - gain) / range;
    return m_viewport.top() + t * m_viewport.height();
}

double CoordinateMapper::yToGain(double y) const
{
    double cy = std::clamp(y, m_viewport.top(), m_viewport.bottom());
    double t = (cy - m_viewport.top()) / m_viewport.height();
    return m_gainMax - t * (m_gainMax - m_gainMin);
}

QPointF CoordinateMapper::toPixel(double freqHz, double gainDb) const
{
    return QPointF(freqToX(freqHz), gainToY(gainDb));
}

void CoordinateMapper::setGainRange(double minDb, double maxDb)
{
    if (minDb < maxDb && maxDb - minDb > 0.1) {
        m_gainMin = minDb;
        m_gainMax = maxDb;
    }
}

double CoordinateMapper::gainMin() const { return m_gainMin; }
double CoordinateMapper::gainMax() const { return m_gainMax; }
double CoordinateMapper::freqMin() const { return m_freqMin; }
double CoordinateMapper::freqMax() const { return m_freqMax; }
