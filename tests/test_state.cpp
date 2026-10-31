#include <cstdint>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "core/State.h"

namespace {

sf::SessionState sampleState() {
    sf::SessionState state;
    state.shaderSource = "void main() { fragColor = vec4(uv, 0.0, 1.0); }";
    state.paramValues[100] = 0.75f;
    state.paramValues[700] = -0.25f;
    state.paramValues[600] = 1.0f;
    state.fxChain = {1, 2, 3, 4, 5};
    state.uiBlob = {0xAA, 0xBB};
    return state;
}

} // namespace

TEST_CASE("state roundtrips shader, params, and blobs", "[state]") {
    const sf::SessionState in = sampleState();
    const auto bytes = sf::serializeState(in);

    sf::SessionState out;
    const auto result = sf::deserializeState(bytes.data(), bytes.size(), out);
    REQUIRE(result.ok);
    REQUIRE(result.version == sf::kStateVersion);
    REQUIRE(out.shaderSource == in.shaderSource);
    REQUIRE(out.paramValues == in.paramValues);
    REQUIRE(out.fxChain == in.fxChain);
    REQUIRE(out.uiBlob == in.uiBlob);
}

TEST_CASE("state starts with the SFST magic and the version", "[state]") {
    const auto bytes = sf::serializeState(sampleState());
    REQUIRE(bytes.size() > 8);
    REQUIRE(bytes[0] == 'S');
    REQUIRE(bytes[1] == 'F');
    REQUIRE(bytes[2] == 'S');
    REQUIRE(bytes[3] == 'T');
    REQUIRE(bytes[4] == sf::kStateVersion);
    REQUIRE(bytes[5] == 0);
}

TEST_CASE("newer versions load their known prefix and ignore trailing bytes", "[state]") {
    auto bytes = sf::serializeState(sampleState());
    bytes[4] = 2; // pretend this was written by version 2
    for (int i = 0; i < 64; ++i) {
        bytes.push_back(static_cast<std::uint8_t>(0xF0 + i)); // unknown appended data
    }

    sf::SessionState out;
    const auto result = sf::deserializeState(bytes.data(), bytes.size(), out);
    REQUIRE(result.ok);
    REQUIRE(result.version == 2);
    REQUIRE(out.paramValues.at(100) == 0.75f);
    REQUIRE(out.shaderSource == sampleState().shaderSource);
}

TEST_CASE("truncated input fails at every cut point", "[state]") {
    const auto bytes = sf::serializeState(sampleState());
    for (std::size_t cut = 0; cut < bytes.size(); ++cut) {
        INFO("cut at " << cut);
        sf::SessionState out;
        const auto result = sf::deserializeState(bytes.data(), cut, out);
        REQUIRE_FALSE(result.ok);
    }
}

TEST_CASE("bad magic and version zero are rejected", "[state]") {
    auto bytes = sf::serializeState(sampleState());
    bytes[0] = 'X';
    sf::SessionState out;
    REQUIRE_FALSE(sf::deserializeState(bytes.data(), bytes.size(), out).ok);

    bytes = sf::serializeState(sampleState());
    bytes[4] = 0;
    REQUIRE_FALSE(sf::deserializeState(bytes.data(), bytes.size(), out).ok);
}

TEST_CASE("oversized length fields are rejected", "[state]") {
    auto bytes = sf::serializeState(sampleState());
    // shaderLen lives at offset 8; make it enormous.
    bytes[8] = 0xFF;
    bytes[9] = 0xFF;
    bytes[10] = 0xFF;
    bytes[11] = 0x7F;
    sf::SessionState out;
    REQUIRE_FALSE(sf::deserializeState(bytes.data(), bytes.size(), out).ok);
}
