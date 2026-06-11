#pragma once

#include "FilterAlgorithm.h"
#include "../AudioEQTypes.h"

class AUDIOEQ_EXPORT ButterworthIIR final : public FilterAlgorithm {
public:
    ButterworthIIR(FilterType type, double freqHz, double gainDb, double q);

    double evaluateAt(double freqHz, double sampleRate) const override;

private:
    FilterType m_type;
    double     m_freqHz;
    double     m_gainDb;
    double     m_q;

    struct Coeffs { double b0, b1, b2, a1, a2; };

    Coeffs computeCoeffs(double sampleRate) const;

    static Coeffs makePeakFilter(double freq, double q, double gain, double sr);
    static Coeffs makeLowShelf(double freq, double q, double gain, double sr);
    static Coeffs makeHighShelf(double freq, double q, double gain, double sr);
    static Coeffs makeLowPass(double freq, double sr);
    static Coeffs makeHighPass(double freq, double sr);

    static double freqResponseDb(const Coeffs& c, double freqHz, double sampleRate);
};
