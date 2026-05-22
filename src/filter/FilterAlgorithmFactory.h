#pragma once
#include "FilterAlgorithm.h"
#include <functional>
#include <memory>

class FilterAlgorithmFactory {
public:
    static std::unique_ptr<FilterAlgorithm> create(FilterAlgorithmType algoType);
    static void registerAlgorithm(FilterAlgorithmType algoType,
                                  std::function<std::unique_ptr<FilterAlgorithm>()> creator);
};
