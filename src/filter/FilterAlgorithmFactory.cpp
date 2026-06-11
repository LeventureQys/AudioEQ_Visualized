#include "FilterAlgorithmFactory.h"
#include "ButterworthIIR.h"

std::unique_ptr<FilterAlgorithm> FilterAlgorithmFactory::create(
    FilterType type,
    FilterAlgorithmType algo,
    double freqHz,
    double gainDb,
    double q
) {
    if (algo == FilterAlgorithmType::ButterworthIIR) {
        return std::make_unique<ButterworthIIR>(type, freqHz, gainDb, q);
    }
    return nullptr;
}
