#include "shared/ParamRegistry.h"

#include <algorithm>

namespace sf {

bool ParamRegistry::add(const ParamDesc &desc) {
    if (desc.name.empty() || desc.minValue > desc.maxValue) {
        return false;
    }
    if (desc.defaultValue < desc.minValue || desc.defaultValue > desc.maxValue) {
        return false;
    }
    return params_.emplace(desc.id, desc).second;
}

std::size_t ParamRegistry::removeRange(std::uint32_t first, std::uint32_t last) {
    std::size_t removed = 0;
    auto it = params_.lower_bound(first);
    while (it != params_.end() && it->first <= last) {
        it = params_.erase(it);
        ++removed;
    }
    return removed;
}

const ParamDesc *ParamRegistry::find(std::uint32_t id) const {
    const auto it = params_.find(id);
    return it == params_.end() ? nullptr : &it->second;
}

float ParamRegistry::clamp(std::uint32_t id, float value) const {
    const ParamDesc *desc = find(id);
    if (desc == nullptr) {
        return value;
    }
    return std::clamp(value, desc->minValue, desc->maxValue);
}

} // namespace sf
