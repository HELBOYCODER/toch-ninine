#include <catch2/catch_test_macros.hpp>

#include "shared/Adsr.h"

namespace {

sf::AdsrParams testParams() {
    sf::AdsrParams p;
    p.attack = 0.05f;
    p.decay = 0.1f;
    p.sustain = 0.5f;
    p.release = 0.2f;
    return p;
}

constexpr float kDt = 0.001f;

} // namespace

TEST_CASE("attack rises monotonically to full level", "[adsr]") {
    sf::Adsr env;
    env.setParams(testParams());
    env.setGate(true);

    float previous = -1.0f;
    for (int i = 0; i < 200 && env.stage() == sf::Adsr::Stage::Attack; ++i) {
        const float level = env.process(kDt);
        REQUIRE(level >= previous);
        previous = level;
    }
    REQUIRE(env.stage() != sf::Adsr::Stage::Attack);
}

TEST_CASE("decay settles at the sustain level while the gate is held", "[adsr]") {
    sf::Adsr env;
    env.setParams(testParams());
    env.setGate(true);
    for (int i = 0; i < 1000; ++i) {
        env.process(kDt);
    }
    REQUIRE(env.stage() == sf::Adsr::Stage::Sustain);
    REQUIRE(env.level() == 0.5f);
}

TEST_CASE("release falls monotonically to silence", "[adsr]") {
    sf::Adsr env;
    env.setParams(testParams());
    env.setGate(true);
    for (int i = 0; i < 1000; ++i) {
        env.process(kDt);
    }
    env.setGate(false);

    float previous = 2.0f;
    for (int i = 0; i < 1000 && env.stage() != sf::Adsr::Stage::Idle; ++i) {
        const float level = env.process(kDt);
        REQUIRE(level <= previous);
        previous = level;
    }
    REQUIRE(env.stage() == sf::Adsr::Stage::Idle);
    REQUIRE(env.level() == 0.0f);
}

TEST_CASE("retrigger during release starts a new attack from the current level", "[adsr]") {
    sf::Adsr env;
    env.setParams(testParams());
    env.setGate(true);
    for (int i = 0; i < 1000; ++i) {
        env.process(kDt);
    }
    env.setGate(false);
    for (int i = 0; i < 20; ++i) {
        env.process(kDt);
    }
    const float before = env.level();
    env.setGate(true);
    const float after = env.process(kDt);
    REQUIRE(env.stage() == sf::Adsr::Stage::Attack);
    REQUIRE(after >= before);
}
