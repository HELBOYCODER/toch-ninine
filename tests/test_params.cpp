#include <catch2/catch_test_macros.hpp>

#include "shared/ParamRegistry.h"

TEST_CASE("param id ranges do not overlap", "[params]") {
    using namespace sf::param;
    REQUIRE(kFps == kMasterGain + 1);
    REQUIRE(kPreviewQuality == kMasterGain + 7);
    REQUIRE(kPreviewQuality < kTrackAttack);
    REQUIRE(kTrackAttack + kTrackCount <= kTrackDecay);
    REQUIRE(kTrackDecay + kTrackCount <= kTrackSustain);
    REQUIRE(kTrackSustain + kTrackCount <= kTrackRelease);
    REQUIRE(kTrackRelease + kTrackCount <= kFxWet);
    REQUIRE(kFxWet + kFxPassCount <= kCustom);
    REQUIRE(kCustom + kCustomCount == 732);
}

TEST_CASE("registry rejects duplicate ids and invalid ranges", "[params]") {
    sf::ParamRegistry registry;
    sf::ParamDesc gain{sf::param::kMasterGain, "Master Gain", 0.0f, 2.0f, 1.0f};
    REQUIRE(registry.add(gain));
    REQUIRE_FALSE(registry.add(gain));

    sf::ParamDesc inverted{200, "Inverted", 1.0f, 0.0f, 0.5f};
    REQUIRE_FALSE(registry.add(inverted));

    sf::ParamDesc outOfRange{201, "Default outside", 0.0f, 1.0f, 3.0f};
    REQUIRE_FALSE(registry.add(outOfRange));

    sf::ParamDesc unnamed{202, "", 0.0f, 1.0f, 0.0f};
    REQUIRE_FALSE(registry.add(unnamed));
    REQUIRE(registry.size() == 1);
}

TEST_CASE("registry clamps values into range and passes unknown ids through", "[params]") {
    sf::ParamRegistry registry;
    REQUIRE(registry.add({sf::param::kMasterGain, "Master Gain", 0.0f, 2.0f, 1.0f}));
    REQUIRE(registry.clamp(sf::param::kMasterGain, 5.0f) == 2.0f);
    REQUIRE(registry.clamp(sf::param::kMasterGain, -1.0f) == 0.0f);
    REQUIRE(registry.clamp(9999, 7.5f) == 7.5f);
}
