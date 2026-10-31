#include <chrono>
#include <cstdint>
#include <memory>
#include <string>

#include <catch2/catch_test_macros.hpp>

#include "shared/Bus.h"

namespace {

// Short unique uid per run so parallel test processes do not collide.
std::string uniqueUid(const char *tag) {
    static int counter = 0;
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    return std::string(tag) + std::to_string(stamp % 100000000) + std::to_string(++counter);
}

} // namespace

TEST_CASE("bus name has the platform-specific form", "[bus]") {
    const std::string name = sf::busName("abc");
#ifdef _WIN32
    REQUIRE(name == "Local\\ShaderForge_Bus_abc");
#else
    REQUIRE(name == "/ShaderForge_Bus_abc");
#endif
}

TEST_CASE("magic validation rejects null and zeroed memory", "[bus]") {
    REQUIRE_FALSE(sf::isValidBus(nullptr));

    auto zeroed = std::make_unique<sf::Bus>();
    REQUIRE_FALSE(sf::isValidBus(zeroed.get()));

    sf::initBus(zeroed.get());
    REQUIRE(sf::isValidBus(zeroed.get()));
    REQUIRE(zeroed->magic.load() == sf::kMagic);
}

TEST_CASE("owner creates, client attaches, and liveSenders tracks detach", "[bus]") {
    const std::string name = sf::busName(uniqueUid("u"));
    auto owner = sf::SharedBus::create(name);
    REQUIRE(owner != nullptr);
    REQUIRE(owner->isOwner());
    REQUIRE(sf::isValidBus(&owner->bus()));

    {
        auto sender = sf::SharedBus::attach(name);
        REQUIRE(sender != nullptr);
        REQUIRE_FALSE(sender->isOwner());
        REQUIRE(owner->bus().liveSenders.load() == 1);
    }
    REQUIRE(owner->bus().liveSenders.load() == 0);
}

TEST_CASE("samples written by a sender are read by the owner", "[bus]") {
    const std::string name = sf::busName(uniqueUid("s"));
    auto owner = sf::SharedBus::create(name);
    REQUIRE(owner != nullptr);
    auto sender = sf::SharedBus::attach(name);
    REQUIRE(sender != nullptr);

    sf::Slot &txSlot = sender->slot(3);
    sf::SlotWriter writer(txSlot.writePos, txSlot.writeStart, txSlot.buf);
    const float samples[4] = {0.1f, 0.2f, 0.3f, 0.4f};
    writer.push(samples, 4);
    owner->slot(3).peak.store(0.4f);

    sf::Slot &rxSlot = owner->slot(3);
    sf::SlotReader reader(rxSlot.writePos, rxSlot.readPos, rxSlot.writeStart, rxSlot.buf);
    float out[4] = {};
    REQUIRE(reader.pop(out, 4) == 4);
    for (int i = 0; i < 4; ++i) {
        REQUIRE(out[i] == samples[i]);
    }
    REQUIRE(owner->slot(3).peak.load() == 0.4f);
}

TEST_CASE("attaching to a missing bus fails", "[bus]") {
    auto missing = sf::SharedBus::attach(sf::busName(uniqueUid("missing")));
    REQUIRE(missing == nullptr);
}

TEST_CASE("the bus object goes away when the owner is destroyed", "[bus]") {
    const std::string name = sf::busName(uniqueUid("d"));
    {
        auto owner = sf::SharedBus::create(name);
        REQUIRE(owner != nullptr);
    }
    REQUIRE(sf::SharedBus::attach(name) == nullptr);
}
