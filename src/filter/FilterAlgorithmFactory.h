#pragma once

#include <memory>
#include "../AudioEQTypes.h"
#include "FilterAlgorithm.h"

// ══════════════════════════════════════════════
// FilterAlgorithmFactory
// ══════════════════════════════════════════════
// Creates FilterAlgorithm instances based on type/algorithm/parameters.
// Stage2: returns nullptr for all combinations (concrete implementations
// will be registered in Stage3).

class AUDIOEQ_EXPORT FilterAlgorithmFactory {
public:
    // Create a filter algorithm instance.
    // type:   FilterType (Peak, LowShelf, HighShelf, LowPass, HighPass)
    // algo:   algorithm selection (currently only ButterworthIIR)
    // freqHz: center/corner frequency
    // gainDb: gain in dB
    // q:      Q factor
    // Returns nullptr if the algo/type combination is not supported.
    static std::unique_ptr<FilterAlgorithm> create(
        FilterType type,
        FilterAlgorithmType algo,
        double freqHz,
        double gainDb,
        double q
    );
};
