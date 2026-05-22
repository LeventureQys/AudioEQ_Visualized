#include "filter/ButterworthIIR.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

double ButterworthIIR::evaluateAt(double freqHz, double sampleRate, const EQBand& band) const
{
	if (band.bypass)
		return 0.0;

	if (freqHz <= 0.0 || sampleRate <= 0.0)
		return 0.0;

	BiquadCoeff coeff;
	switch (band.type) {
	case FilterType::Peak:
		coeff = makePeakFilter(band.frequency, band.q, band.gain, sampleRate);
		break;
	case FilterType::LowShelf:
		coeff = makeLowShelf(band.frequency, band.q, band.gain, sampleRate);
		break;
	case FilterType::HighShelf:
		coeff = makeHighShelf(band.frequency, band.q, band.gain, sampleRate);
		break;
	case FilterType::LowPass:
		coeff = makeLowPass(band.frequency, sampleRate);
		break;
	case FilterType::HighPass:
		coeff = makeHighPass(band.frequency, sampleRate);
		break;
	default:
		return 0.0;
	}

	return freqResponseGain(coeff, freqHz, sampleRate);
}

QPair<double, double> ButterworthIIR::qRange(FilterType filterType) const
{
	switch (filterType) {
	case FilterType::Peak:       return {0.4, 128.0};
	case FilterType::LowShelf:
	case FilterType::HighShelf:  return {0.4, 1.6};
	case FilterType::LowPass:
	case FilterType::HighPass:
	case FilterType::BandPass:   return {0.4, 128.0};
	}
	return {0.4, 128.0};
}

ButterworthIIR::BiquadCoeff ButterworthIIR::makePeakFilter(double freq, double q, double gain, double sampleRate) const
{
	double A = std::pow(10.0, gain / 40.0);
	double omega = 2.0 * M_PI * freq / sampleRate;
	double cos_omega = std::cos(omega);
	double sin_omega = std::sin(omega);
	double alpha = sin_omega / (2.0 * q);

	double b0 = 1.0 + alpha * A;
	double b1 = -2.0 * cos_omega;
	double b2 = 1.0 - alpha * A;
	double a0 = 1.0 + alpha / A;
	double a1 = -2.0 * cos_omega;
	double a2 = 1.0 - alpha / A;

	b0 /= a0;
	b1 /= a0;
	b2 /= a0;
	a1 /= a0;
	a2 /= a0;

	return {b0, b1, b2, a1, a2};
}

ButterworthIIR::BiquadCoeff ButterworthIIR::makeLowShelf(double freq, double q, double gain, double sampleRate) const
{
	double A = std::pow(10.0, gain / 40.0);
	double aminus1 = A - 1.0;
	double aplus1 = A + 1.0;
	double omega = 2.0 * M_PI * freq / sampleRate;
	double coso = std::cos(omega);
	double beta = std::sin(omega) * std::sqrt(A) / q;

	double aminus1TimesCoso = aminus1 * coso;
	double aplus1TimesCoso = aplus1 * coso;
	double a0 = aplus1 + aminus1TimesCoso + beta;

	double b0 = (A * (aplus1 - aminus1TimesCoso + beta)) / a0;
	double b1 = (2.0 * A * (aminus1 - aplus1TimesCoso)) / a0;
	double b2 = (A * (aplus1 - aminus1TimesCoso - beta)) / a0;
	double a1 = (-2.0 * (aminus1 + aplus1TimesCoso)) / a0;
	double a2 = (aplus1 + aminus1TimesCoso - beta) / a0;

	return {b0, b1, b2, a1, a2};
}

ButterworthIIR::BiquadCoeff ButterworthIIR::makeHighShelf(double freq, double q, double gain, double sampleRate) const
{
	double A = std::pow(10.0, gain / 40.0);
	double aminus1 = A - 1.0;
	double aplus1 = A + 1.0;
	double omega = 2.0 * M_PI * freq / sampleRate;
	double coso = std::cos(omega);
	double beta = std::sin(omega) * std::sqrt(A) / q;

	double aminus1TimesCoso = aminus1 * coso;
	double aplus1TimesCoso = aplus1 * coso;
	double a0 = aplus1 - aminus1TimesCoso + beta;

	double b0 = (A * (aplus1 + aminus1TimesCoso + beta)) / a0;
	double b1 = (-2.0 * A * (aminus1 + aplus1TimesCoso)) / a0;
	double b2 = (A * (aplus1 + aminus1TimesCoso - beta)) / a0;
	double a1 = (2.0 * (aminus1 - aplus1TimesCoso)) / a0;
	double a2 = (aplus1 - aminus1TimesCoso - beta) / a0;

	return {b0, b1, b2, a1, a2};
}

ButterworthIIR::BiquadCoeff ButterworthIIR::makeLowPass(double freq, double sampleRate) const
{
	double omega = 2.0 * M_PI * freq / sampleRate;
	double cos_omega = std::cos(omega);
	double sin_omega = std::sin(omega);
	double alpha = sin_omega / std::sqrt(2.0);

	double b0 = (1.0 - cos_omega) / 2.0;
	double b1 = 1.0 - cos_omega;
	double b2 = (1.0 - cos_omega) / 2.0;
	double a0 = 1.0 + alpha;
	double a1 = -2.0 * cos_omega;
	double a2 = 1.0 - alpha;

	b0 /= a0;
	b1 /= a0;
	b2 /= a0;
	a1 /= a0;
	a2 /= a0;

	return {b0, b1, b2, a1, a2};
}

ButterworthIIR::BiquadCoeff ButterworthIIR::makeHighPass(double freq, double sampleRate) const
{
	double omega = 2.0 * M_PI * freq / sampleRate;
	double cos_omega = std::cos(omega);
	double sin_omega = std::sin(omega);
	double alpha = sin_omega / std::sqrt(2.0);

	double b0 = (1.0 + cos_omega) / 2.0;
	double b1 = -(1.0 + cos_omega);
	double b2 = (1.0 + cos_omega) / 2.0;
	double a0 = 1.0 + alpha;
	double a1 = -2.0 * cos_omega;
	double a2 = 1.0 - alpha;

	b0 /= a0;
	b1 /= a0;
	b2 /= a0;
	a1 /= a0;
	a2 /= a0;

	return {b0, b1, b2, a1, a2};
}

double ButterworthIIR::freqResponseGain(const BiquadCoeff& coeff, double freq, double sampleRate) const
{
	double omega = 2.0 * M_PI * freq / sampleRate;
	double cos_omega = std::cos(omega);
	double cos_2omega = std::cos(2.0 * omega);

	double numerator = coeff.b0 * coeff.b0 + coeff.b1 * coeff.b1 + coeff.b2 * coeff.b2
		+ 2.0 * (coeff.b0 * coeff.b1 + coeff.b1 * coeff.b2) * cos_omega
		+ 2.0 * coeff.b0 * coeff.b2 * cos_2omega;

	double denominator = 1.0 + coeff.a1 * coeff.a1 + coeff.a2 * coeff.a2
		+ 2.0 * (coeff.a1 + coeff.a1 * coeff.a2) * cos_omega
		+ 2.0 * coeff.a2 * cos_2omega;

	return 10.0 * std::log10(numerator / denominator);
}
