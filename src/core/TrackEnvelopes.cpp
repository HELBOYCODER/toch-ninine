#include "core/TrackEnvelopes.h"

namespace sf {

void TrackEnvelopes::setParams(int track, const AdsrParams &params) {
    if (track < 0 || track >= kTracks) {
        return;
    }
    envelopes_[static_cast<std::size_t>(track)].setParams(params);
}

void TrackEnvelopes::process(const float *peaks, float dtSeconds, float *out) noexcept {
    for (std::size_t i = 0; i < envelopes_.size(); ++i) {
        envelopes_[i].setGate(peaks[i] > kGateThreshold);
        out[i] = envelopes_[i].process(dtSeconds) * kTrackScale;
    }
}

float TrackEnvelopes::level(int track) const noexcept {
    if (track < 0 || track >= kTracks) {
        return 0.0f;
    }
    return envelopes_[static_cast<std::size_t>(track)].level();
}

} // namespace sf
