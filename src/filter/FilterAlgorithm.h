#pragma once
#include "AudioEQTypes.h"
#include <QPair>

class FilterAlgorithm {
public:
    virtual ~FilterAlgorithm() = default;

    virtual double evaluateAt(double freqHz, double sampleRate, const EQBand& band) const = 0;

    virtual FilterAlgorithmType type() const = 0;

    virtual QPair<double,double> qRange(FilterType filterType) const = 0;
};
