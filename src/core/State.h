#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace sf {

// Versioned session state. Byte layout (little-endian):
//
//   "SFST"            4 bytes  magic
//   u32 version                current = kStateVersion
//   u32 shaderLen     + bytes  shader source (UTF-8)
//   u32 paramCount    + pairs  (u32 paramId, f32 value)
//   u32 fxLen         + bytes  FX chain blob (opaque to this module)
//   u32 uiLen         + bytes  UI blob (opaque to this module)
//
// Forward compatibility: a reader accepts any version >= 1. For a newer version it
// reads the known prefix and ignores the rest, including trailing bytes after uiLen.
constexpr std::uint32_t kStateVersion = 1;

struct SessionState {
    std::string shaderSource;
    std::map<std::uint32_t, float> paramValues;
    std::vector<std::uint8_t> fxChain;
    std::vector<std::uint8_t> uiBlob;
};

struct StateLoadResult {
    bool ok = false;
    std::uint32_t version = 0;
    std::string error;
};

std::vector<std::uint8_t> serializeState(const SessionState &state);

StateLoadResult deserializeState(const std::uint8_t *data, std::size_t size, SessionState &out);

} // namespace sf
