#include <cmath>

#include <catch2/catch_test_macros.hpp>

#include "core/TrackEnvelopes.h"

namespace {

constexpr float kDt = 0.001f;

sf::AdsrParams steadyParams() {
    sf::AdsrParams p;
    p.attack = 0.01f;
    p.decay = 0.05f;
    p.sustain = 0.65f;
    p.release = 0.1f;
    return p;
}

} // namespace

TEST_CASE("silent input keeps every iTrack at zero", "[envelopes]") {
    sf::TrackEnvelopes env;
    const float peaks[sf::TrackEnvelopes::kTracks] = {};
    float out[sf::TrackEnvelopes::kTracks] = {};
    for (int i = 0; i < 500; ++i) {
        env.process(peaks, kDt, out);
    }
    for (float v : out) {
        REQUIRE(v == 0.0f);
    }
}

TEST_CASE("only the track that receives signal rises", "[envelopes]") {
    sf::TrackEnvelopes env;
    for (int t = 0; t < sf::TrackEnvelopes::kTracks; ++t) {
        env.setParams(t, steadyParams());
    }
    float peaks[sf::TrackEnvelopes::kTracks] = {};
    peaks[2] = 0.8f;
    float out[sf::TrackEnvelopes::kTracks] = {};
    for (int i = 0; i < 1000; ++i) {
        env.process(peaks, kDt, out);
    }
    REQUIRE(out[2] > 0.0f);
    for (int t = 0; t < sf::TrackEnvelopes::kTracks; ++t) {
        if (t != 2) {
            REQUIRE(out[t] == 0.0f);
        }
    }
}

TEST_CASE("held signal settles at sustain times the track scale", "[envelopes]") {
    sf::TrackEnvelopes env;
    env.setParams(0, steadyParams());
    float peaks[sf::TrackEnvelopes::kTracks] = {};
    peaks[0] = 1.0f;
    float out[sf::TrackEnvelopes::kTracks] = {};
    for (int i = 0; i < 1000; ++i) {
        env.process(peaks, kDt, out);
    }
    REQUIRE(std::fabs(out[0] - 0.65f * sf::kTrackScale) < 1e-4f);
    REQUIRE(out[0] >= 0.0f);
    REQUIRE(out[0] <= sf::kTrackScale);
}

TEST_CASE("out-of-range track indices are ignored", "[envelopes]") {
    sf::TrackEnvelopes env;
    env.setParams(-1, steadyParams());
    env.setParams(sf::TrackEnvelopes::kTracks, steadyParams());
    REQUIRE(env.level(-1) == 0.0f);
    REQUIRE(env.level(sf::TrackEnvelopes::kTracks) == 0.0f);
}
