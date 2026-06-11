#pragma once

#include "../AudioEQTypes.h"

// ══════════════════════════════════════════════
// FilterAlgorithm — Abstract base class
// ══════════════════════════════════════════════
// Pure virtual interface for all EQ filter algorithms.
// All implementations MUST make evaluateAt() a pure function
// (const, no mutable state) for thread safety.

class FilterAlgorithm {
public:
    virtual ~FilterAlgorithm() = default;

    // Compute the gain (dB) contributed by this filter at a given frequency.
    // freqHz:     frequency in Hz (> 0)
    // sampleRate: sampling rate in Hz (e.g. 44100)
    // Returns gain in dB (positive = boost, negative = cut)
    virtual double evaluateAt(double freqHz, double sampleRate) const = 0;
};
