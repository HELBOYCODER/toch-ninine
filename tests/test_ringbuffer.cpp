#include <algorithm>
#include <atomic>
#include <cstddef>
#include <thread>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "shared/RingBuffer.h"

namespace {

constexpr std::uint32_t kSmall = 64;

struct SmallRing {
    std::atomic<std::uint32_t> writePos{0};
    std::atomic<std::uint32_t> readPos{0};
    std::atomic<std::uint32_t> writeStart{0};
    float buf[kSmall]{};
};

} // namespace

TEST_CASE("ring wraps around and preserves sample order", "[ringbuffer]") {
    SmallRing ring;
    sf::RingWriter<kSmall> writer(ring.writePos, ring.writeStart, ring.buf);
    sf::RingReader<kSmall> reader(ring.writePos, ring.readPos, ring.writeStart, ring.buf);

    std::vector<float> out(kSmall);
    float next = 0.0f;
    for (int round = 0; round < 50; ++round) {
        std::vector<float> in(40);
        for (float &v : in) {
            v = next++;
        }
        writer.push(in.data(), in.size());
        const std::size_t got = reader.pop(out.data(), out.size());
        REQUIRE(got == in.size());
        for (std::size_t i = 0; i < in.size(); ++i) {
            REQUIRE(out[i] == in[i]);
        }
    }
    REQUIRE(reader.dropped() == 0);
}

TEST_CASE("overrun drops the oldest samples", "[ringbuffer]") {
    SmallRing ring;
    sf::RingWriter<kSmall> writer(ring.writePos, ring.writeStart, ring.buf);
    sf::RingReader<kSmall> reader(ring.writePos, ring.readPos, ring.writeStart, ring.buf);

    const std::size_t total = 3 * kSmall + 5;
    std::vector<float> in(total);
    for (std::size_t i = 0; i < total; ++i) {
        in[i] = static_cast<float>(i);
    }
    writer.push(in.data(), in.size());

    std::vector<float> out(2 * kSmall);
    const std::size_t got = reader.pop(out.data(), out.size());
    REQUIRE(got == kSmall);
    REQUIRE(out[0] == static_cast<float>(total - kSmall));
    REQUIRE(out[got - 1] == static_cast<float>(total - 1));
    REQUIRE(reader.dropped() == total - kSmall);
}

TEST_CASE("available() reports unread samples capped at the ring size", "[ringbuffer]") {
    SmallRing ring;
    sf::RingWriter<kSmall> writer(ring.writePos, ring.writeStart, ring.buf);
    sf::RingReader<kSmall> reader(ring.writePos, ring.readPos, ring.writeStart, ring.buf);

    std::vector<float> in(10, 1.0f);
    writer.push(in.data(), in.size());
    REQUIRE(reader.available() == 10);

    std::vector<float> big(4 * kSmall, 2.0f);
    writer.push(big.data(), big.size());
    REQUIRE(reader.available() == kSmall);
}

TEST_CASE("SPSC stress with two threads keeps order and accounts for every sample",
          "[ringbuffer][stress]") {
    constexpr std::uint32_t kRing = 1024;
    constexpr int kTotal = 2000000;

    std::atomic<std::uint32_t> writePos{0};
    std::atomic<std::uint32_t> readPos{0};
    std::atomic<std::uint32_t> writeStart{0};
    std::vector<float> buf(kRing, 0.0f);
    std::atomic<bool> producerDone{false};

    std::thread producer([&] {
        sf::RingWriter<kRing> writer(writePos, writeStart, buf.data());
        std::vector<float> chunk(37);
        int sent = 0;
        while (sent < kTotal) {
            const int n = std::min<int>(static_cast<int>(chunk.size()), kTotal - sent);
            for (int i = 0; i < n; ++i) {
                chunk[static_cast<std::size_t>(i)] = static_cast<float>(sent + i);
            }
            writer.push(chunk.data(), static_cast<std::size_t>(n));
            sent += n;
        }
        producerDone.store(true, std::memory_order_release);
    });

    sf::RingReader<kRing> reader(writePos, readPos, writeStart, buf.data());
    std::vector<float> out(64);
    double last = -1.0;
    bool ordered = true;
    std::size_t received = 0;
    for (;;) {
        const std::size_t n = reader.pop(out.data(), out.size());
        for (std::size_t i = 0; i < n; ++i) {
            if (out[i] <= last) {
                ordered = false;
            }
            last = out[i];
        }
        received += n;
        if (n == 0) {
            if (producerDone.load(std::memory_order_acquire) && reader.available() == 0) {
                break;
            }
            std::this_thread::yield();
        }
    }
    producer.join();

    REQUIRE(ordered);
    REQUIRE(received + reader.dropped() == static_cast<std::uint64_t>(kTotal));
    REQUIRE(last == static_cast<double>(kTotal - 1));
}
