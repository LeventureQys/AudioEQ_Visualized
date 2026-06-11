#include "ButterworthIIR.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

ButterworthIIR::ButterworthIIR(FilterType type, double freqHz, double gainDb, double q)
    : m_type(type), m_freqHz(freqHz), m_gainDb(gainDb), m_q(q) {}

double ButterworthIIR::evaluateAt(double freqHz, double sampleRate) const {
    if (freqHz <= 0.0 || sampleRate <= 0.0) return 0.0;
    Coeffs c = computeCoeffs(sampleRate);
    return freqResponseDb(c, freqHz, sampleRate);
}

ButterworthIIR::Coeffs ButterworthIIR::computeCoeffs(double sampleRate) const {
    switch (m_type) {
        case FilterType::Peak:      return makePeakFilter(m_freqHz, m_q, m_gainDb, sampleRate);
        case FilterType::LowShelf:  return makeLowShelf(m_freqHz, m_q, m_gainDb, sampleRate);
        case FilterType::HighShelf: return makeHighShelf(m_freqHz, m_q, m_gainDb, sampleRate);
        case FilterType::LowPass:   return makeLowPass(m_freqHz, sampleRate);
        case FilterType::HighPass:  return makeHighPass(m_freqHz, sampleRate);
    }
    return {1, 0, 0, 0, 0};
}

ButterworthIIR::Coeffs ButterworthIIR::makePeakFilter(double freq, double q, double gain, double sr) {
    double A  = std::pow(10.0, gain / 40.0);
    double w0 = 2.0 * M_PI * freq / sr;
    double cosW = std::cos(w0);
    double alpha = std::sin(w0) / (2.0 * q);

    double b0 =  1.0 + alpha * A;
    double b1 = -2.0 * cosW;
    double b2 =  1.0 - alpha * A;
    double a0 =  1.0 + alpha / A;
    double a1 = -2.0 * cosW;
    double a2 =  1.0 - alpha / A;

    return {b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0};
}

ButterworthIIR::Coeffs ButterworthIIR::makeLowShelf(double freq, double q, double gain, double sr) {
    double A  = std::pow(10.0, gain / 40.0);
    double w0 = 2.0 * M_PI * freq / sr;
    double cosW = std::cos(w0);
    double alpha = std::sin(w0) / 2.0 * std::sqrt((A + 1.0 / A) * (1.0 / q - 1.0) + 2.0);

    double b0 =      A * ((A + 1.0) - (A - 1.0) * cosW + alpha);
    double b1 =  2.0 * A * ((A - 1.0) - (A + 1.0) * cosW);
    double b2 =      A * ((A + 1.0) - (A - 1.0) * cosW - alpha);
    double a0 =           (A + 1.0) + (A - 1.0) * cosW + alpha;
    double a1 =     -2.0 * ((A - 1.0) + (A + 1.0) * cosW);
    double a2 =           (A + 1.0) + (A - 1.0) * cosW - alpha;

    return {b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0};
}

ButterworthIIR::Coeffs ButterworthIIR::makeHighShelf(double freq, double q, double gain, double sr) {
    double A  = std::pow(10.0, gain / 40.0);
    double w0 = 2.0 * M_PI * freq / sr;
    double cosW = -std::cos(w0);
    double alpha = std::sin(w0) / 2.0 * std::sqrt((A + 1.0 / A) * (1.0 / q - 1.0) + 2.0);

    double b0 =      A * ((A + 1.0) + (A - 1.0) * cosW + alpha);
    double b1 = -2.0 * A * ((A - 1.0) + (A + 1.0) * cosW);
    double b2 =      A * ((A + 1.0) + (A - 1.0) * cosW - alpha);
    double a0 =           (A + 1.0) - (A - 1.0) * cosW + alpha;
    double a1 =      2.0 * ((A - 1.0) - (A + 1.0) * cosW);
    double a2 =           (A + 1.0) - (A - 1.0) * cosW - alpha;

    return {b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0};
}

ButterworthIIR::Coeffs ButterworthIIR::makeLowPass(double freq, double sr) {
    double w0 = 2.0 * M_PI * freq / sr;
    double cosW = std::cos(w0);
    double alpha = std::sin(w0) / (2.0 * 0.7071);

    double b0 = (1.0 - cosW) / 2.0;
    double b1 =  1.0 - cosW;
    double b2 = (1.0 - cosW) / 2.0;
    double a0 =  1.0 + alpha;
    double a1 = -2.0 * cosW;
    double a2 =  1.0 - alpha;

    return {b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0};
}

ButterworthIIR::Coeffs ButterworthIIR::makeHighPass(double freq, double sr) {
    double w0 = 2.0 * M_PI * freq / sr;
    double cosW = std::cos(w0);
    double alpha = std::sin(w0) / (2.0 * 0.7071);

    double b0 =  (1.0 + cosW) / 2.0;
    double b1 = -(1.0 + cosW);
    double b2 =  (1.0 + cosW) / 2.0;
    double a0 =   1.0 + alpha;
    double a1 =  -2.0 * cosW;
    double a2 =   1.0 - alpha;

    return {b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0};
}

double ButterworthIIR::freqResponseDb(const Coeffs& c, double freqHz, double sampleRate) {
    double w = 2.0 * M_PI * freqHz / sampleRate;
    double phi = 4.0 * std::sin(w / 2.0) * std::sin(w / 2.0);

    double b0 = c.b0, b1 = c.b1, b2 = c.b2;
    double a1 = c.a1, a2 = c.a2;

    double num = (b0 + b1 + b2) * (b0 + b1 + b2) + phi * (b0 * b2 * phi - b1 * (b0 + b2) - 4.0 * b0 * b2);
    double den = (1.0 + a1 + a2) * (1.0 + a1 + a2) + phi * (a2 * phi - a1 * (1.0 + a2) - 4.0 * a2);

    if (den <= 0.0) return -200.0;
    return 10.0 * std::log10(num / den);
}
