#pragma once

#include <QRectF>
#include <QPointF>
#include "AudioEQTypes.h"

class AUDIOEQ_EXPORT CoordinateMapper {
public:
    CoordinateMapper();

    void setViewport(const QRectF& rect);
    QRectF viewport() const;

    void setSampleRate(SampleRate rate);

    double freqToX(double freqHz) const;
    double xToFreq(double x) const;

    double gainToY(double gainDb) const;
    double yToGain(double y) const;

    QPointF toPixel(double freqHz, double gainDb) const;

    void setGainRange(double minDb, double maxDb);
    double gainMin() const;
    double gainMax() const;

    double freqMin() const;
    double freqMax() const;

private:
    QRectF  m_viewport;
    double  m_gainMin = -48.0;
    double  m_gainMax = +48.0;
    double  m_freqMin = 20.0;
    double  m_freqMax = 22050.0;
};
