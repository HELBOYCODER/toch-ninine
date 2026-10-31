#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>

namespace sf {

// Stable parameter IDs. These numbers are written into saved state and into
// host automation, so they must never be renumbered.
namespace param {
inline constexpr std::uint32_t kMasterGain = 100;
inline constexpr std::uint32_t kFps = 101;
inline constexpr std::uint32_t kVsync = 102;
inline constexpr std::uint32_t kResMode = 103;
inline constexpr std::uint32_t kRecFps = 104;
inline constexpr std::uint32_t kRecCodec = 105;
inline constexpr std::uint32_t kRecQuality = 106;
inline constexpr std::uint32_t kPreviewQuality = 107;

inline constexpr std::uint32_t kTrackAttack = 200;  // + track 0..7
inline constexpr std::uint32_t kTrackDecay = 300;   // + track 0..7
inline constexpr std::uint32_t kTrackSustain = 400; // + track 0..7
inline constexpr std::uint32_t kTrackRelease = 500; // + track 0..7
inline constexpr std::uint32_t kFxWet = 600;        // + pass 0..15
inline constexpr std::uint32_t kCustom = 700;       // + uCustom 0..31

inline constexpr int kTrackCount = 8;
inline constexpr int kFxPassCount = 16;
inline constexpr int kCustomCount = 32;
} // namespace param

struct ParamDesc {
    std::uint32_t id = 0;
    std::string name;
    float minValue = 0.0f;
    float maxValue = 1.0f;
    float defaultValue = 0.0f;
};

// Registry of host-visible parameters. Rejects duplicate IDs and bad ranges so
// that a typo in a table cannot silently shadow another parameter.
class ParamRegistry {
public:
    bool add(const ParamDesc &desc);

    // Removes every parameter whose id lies in [first, last]. Returns how many were removed.
    std::size_t removeRange(std::uint32_t first, std::uint32_t last);
    const ParamDesc *find(std::uint32_t id) const;
    std::size_t size() const noexcept { return params_.size(); }

    // Clamps value into the descriptor's range; unknown IDs pass through unchanged.
    float clamp(std::uint32_t id, float value) const;

private:
    std::map<std::uint32_t, ParamDesc> params_;
};

} // namespace sf
