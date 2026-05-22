#include "filter/FilterAlgorithmFactory.h"
#include <unordered_map>

namespace {

std::unordered_map<FilterAlgorithmType, std::function<std::unique_ptr<FilterAlgorithm>()>>& registry()
{
	static std::unordered_map<FilterAlgorithmType, std::function<std::unique_ptr<FilterAlgorithm>()>> map;
	return map;
}

} // namespace

std::unique_ptr<FilterAlgorithm> FilterAlgorithmFactory::create(FilterAlgorithmType algoType)
{
	auto& map = registry();
	auto it = map.find(algoType);
	if (it != map.end()) {
		return it->second();
	}
	return nullptr;
}

void FilterAlgorithmFactory::registerAlgorithm(FilterAlgorithmType algoType,
                                               std::function<std::unique_ptr<FilterAlgorithm>()> creator)
{
	registry()[algoType] = std::move(creator);
}
