#pragma once
#include "FilterAlgorithm.h"

class ButterworthIIR : public FilterAlgorithm {
public:
    FilterAlgorithmType type() const override { return FilterAlgorithmType::ButterworthIIR; }
    double evaluateAt(double freqHz, double sampleRate, const EQBand& band) const override;
    QPair<double,double> qRange(FilterType filterType) const override;

private:
    struct BiquadCoeff { double b0, b1, b2, a1, a2; };

    BiquadCoeff makePeakFilter(double freq, double q, double gain, double sampleRate) const;
    BiquadCoeff makeLowShelf(double freq, double q, double gain, double sampleRate) const;
    BiquadCoeff makeHighShelf(double freq, double q, double gain, double sampleRate) const;
    BiquadCoeff makeLowPass(double freq, double sampleRate) const;
    BiquadCoeff makeHighPass(double freq, double sampleRate) const;
    double freqResponseGain(const BiquadCoeff& coeff, double freq, double sampleRate) const;
};
