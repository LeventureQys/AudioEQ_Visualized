#include "CoordinateMapper.h"
#include <QRectF>
#include <algorithm>
#include <cmath>

CoordinateMapper::CoordinateMapper(QRect viewport, double freqMin, double freqMax, double gainMin, double gainMax)
	: m_viewport(viewport)
	, m_freqMin(freqMin)
	, m_freqMax(freqMax)
	, m_gainMin(gainMin)
	, m_gainMax(gainMax)
{
}

double CoordinateMapper::freqToX(double freqHz) const
{
	double clampedFreq = std::clamp(freqHz, m_freqMin, m_freqMax);
	QRectF vp(m_viewport);
	double ratio = clampedFreq / m_freqMin;
	double logRatio = std::log10(m_freqMax / m_freqMin);
	return vp.left() + vp.width() * std::log10(ratio) / logRatio;
}

double CoordinateMapper::gainToY(double gainDb) const
{
	double clampedGain = std::clamp(gainDb, m_gainMin, m_gainMax);
	QRectF vp(m_viewport);
	double normalized = (clampedGain - m_gainMin) / (m_gainMax - m_gainMin);
	return vp.bottom() - vp.height() * normalized;
}

double CoordinateMapper::xToFreq(double x) const
{
	QRectF vp(m_viewport);
	double normalized = (x - vp.left()) / vp.width();
	double clampedNormalized = std::clamp(normalized, 0.0, 1.0);
	return m_freqMin * std::pow(m_freqMax / m_freqMin, clampedNormalized);
}

double CoordinateMapper::yToGain(double y) const
{
	QRectF vp(m_viewport);
	double normalized = (vp.bottom() - y) / vp.height();
	double clampedNormalized = std::clamp(normalized, 0.0, 1.0);
	return m_gainMin + clampedNormalized * (m_gainMax - m_gainMin);
}

void CoordinateMapper::setViewport(QRect viewport)
{
	m_viewport = viewport;
}

void CoordinateMapper::setFreqRange(double min, double max)
{
	m_freqMin = min;
	m_freqMax = max;
}

void CoordinateMapper::setGainRange(double min, double max)
{
	m_gainMin = min;
	m_gainMax = max;
}

double CoordinateMapper::freqMin() const
{
	return m_freqMin;
}

double CoordinateMapper::freqMax() const
{
	return m_freqMax;
}

double CoordinateMapper::gainMin() const
{
	return m_gainMin;
}

double CoordinateMapper::gainMax() const
{
	return m_gainMax;
}

QRect CoordinateMapper::viewport() const
{
	return m_viewport;
}
