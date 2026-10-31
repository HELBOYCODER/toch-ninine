#include "core/ParamBinding.h"

namespace sf {

bool registerShaderParams(ParamRegistry &registry, const std::vector<PragmaParam> &params) {
    registry.removeRange(param::kCustom, param::kCustom + param::kCustomCount - 1);
    bool ok = true;
    for (const PragmaParam &p : params) {
        if (p.index < 0 || p.index >= param::kCustomCount) {
            ok = false;
            continue;
        }
        ParamDesc desc;
        desc.id = param::kCustom + static_cast<std::uint32_t>(p.index);
        desc.name = p.name;
        desc.minValue = p.minValue;
        desc.maxValue = p.maxValue;
        desc.defaultValue = p.defaultValue;
        ok = registry.add(desc) && ok;
    }
    return ok;
}

} // namespace sf
