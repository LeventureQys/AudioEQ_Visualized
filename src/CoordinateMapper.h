#pragma once
#include <QRect>
#include <QPointF>

class CoordinateMapper {
public:
	CoordinateMapper(QRect viewport, double freqMin, double freqMax, double gainMin, double gainMax);

	double freqToX(double freqHz) const;
	double gainToY(double gainDb) const;
	double xToFreq(double x) const;
	double yToGain(double y) const;

	void setViewport(QRect viewport);
	void setFreqRange(double min, double max);
	void setGainRange(double min, double max);

	double freqMin() const;
	double freqMax() const;
	double gainMin() const;
	double gainMax() const;
	QRect viewport() const;

private:
	QRect m_viewport;
	double m_freqMin;
	double m_freqMax;
	double m_gainMin;
	double m_gainMax;
};
