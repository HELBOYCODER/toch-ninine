#include <atomic>
#include <cstdint>
#include <thread>

#include <catch2/catch_test_macros.hpp>

#include "core/Snapshot.h"

namespace {

struct Pair {
    std::uint32_t a;
    std::uint32_t b; // invariant: b == ~a
};

} // namespace

TEST_CASE("snapshot starts zeroed and returns the last written value", "[snapshot]") {
    sf::Snapshot<Pair> snap;
    const Pair zero = snap.read();
    REQUIRE(zero.a == 0);
    REQUIRE(zero.b == 0);

    snap.write(Pair{7u, ~7u});
    const Pair got = snap.read();
    REQUIRE(got.a == 7u);
    REQUIRE(got.b == ~7u);
}

TEST_CASE("readers never observe a torn snapshot while the writer runs", "[snapshot][stress]") {
    sf::Snapshot<Pair> snap;
    snap.write(Pair{0u, ~0u}); // the zeroed initial value is not a pair; publish a valid one first
    std::atomic<bool> stop{false};
    std::thread writer([&] {
        for (std::uint32_t i = 1; i <= 200000; ++i) {
            snap.write(Pair{i, ~i});
        }
        stop.store(true);
    });

    bool torn = false;
    std::uint64_t reads = 0;
    while (!stop.load()) {
        const Pair p = snap.read();
        if (p.b != static_cast<std::uint32_t>(~p.a)) {
            torn = true;
            break;
        }
        ++reads;
    }
    writer.join();
    REQUIRE_FALSE(torn);
    REQUIRE(reads > 0);
}
