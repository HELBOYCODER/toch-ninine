#pragma once

#include <array>

#include "shared/Adsr.h"

namespace sf {

// One ADSR per uniform track (iTrack0..iTrack7). Feed it the per-track peak of each
// block; it returns the iTrackN values in the 0..4 range (see kTrackScale).
class TrackEnvelopes {
public:
    static constexpr int kTracks = 8;

    // Out-of-range track indices are ignored.
    void setParams(int track, const AdsrParams &params);

    // peaks and out must each hold kTracks values. peaks are linear 0..1.
    void process(const float *peaks, float dtSeconds, float *out) noexcept;

    float level(int track) const noexcept;

private:
    std::array<Adsr, kTracks> envelopes_{};
};

} // namespace sf
