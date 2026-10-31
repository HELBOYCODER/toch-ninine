#pragma once

#include <vector>

#include "core/ShaderPreprocessor.h"
#include "shared/ParamRegistry.h"

namespace sf {

// Replaces the custom-parameter range of the registry with the shader's
// #pragma params. Param i gets id param::kCustom + i, the same channel as uCustomN.
// Returns false (and leaves the range empty) when a param index is out of range.
bool registerShaderParams(ParamRegistry &registry, const std::vector<PragmaParam> &params);

} // namespace sf
