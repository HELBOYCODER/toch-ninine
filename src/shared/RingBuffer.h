#pragma once

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace sf {

// Single-producer / single-consumer float ring used by every bus slot.
//
// Ownership:
//   writer -> writePos (release store after the samples are stored)
//   reader -> readPos  (release store after the samples are consumed)
//
// Overrun policy: the writer never blocks and never touches readPos. When the
// reader has fallen more than N samples behind it jumps to the oldest sample
// still in the ring (drop-oldest) and counts what it skipped.
//
// Overwrite detection: before the writer touches any slot it first publishes the
// end index of the push in writeStart (release fence, then data). A reader that
// has seen any new sample data is therefore guaranteed to see the matching
// writeStart value after its own acquire fence. Samples whose slot may have been
// reused are discarded, so a reader never returns a torn or stale value.

template <std::uint32_t N> class RingWriter {
    static_assert(N != 0 && (N & (N - 1)) == 0, "ring size must be a power of two");

public:
    static constexpr std::uint32_t kMask = N - 1;

    RingWriter(std::atomic<std::uint32_t> &writePos, std::atomic<std::uint32_t> &writeStart,
               float *buf) noexcept
        : writePos_(writePos), writeStart_(writeStart), buf_(buf) {}

    void push(const float *src, std::size_t count) noexcept {
        if (count == 0) {
            return;
        }
        const std::uint32_t base = writePos_.load(std::memory_order_relaxed);
        const std::uint32_t end = base + static_cast<std::uint32_t>(count);

        // Announce first: any reader that observes the data below must also observe `end`.
        writeStart_.store(end, std::memory_order_relaxed);
        std::atomic_thread_fence(std::memory_order_release);

        // Only the newest N samples can survive; earlier ones are skipped.
        const std::size_t keep = count > N ? N : count;
        const std::size_t skip = count - keep;
        for (std::size_t i = 0; i < keep; ++i) {
            const std::uint32_t index = base + static_cast<std::uint32_t>(skip + i);
            buf_[index & kMask] = src[skip + i];
        }
        writePos_.store(end, std::memory_order_release);
    }

private:
    std::atomic<std::uint32_t> &writePos_;
    std::atomic<std::uint32_t> &writeStart_;
    float *buf_;
};

template <std::uint32_t N> class RingReader {
    static_assert(N != 0 && (N & (N - 1)) == 0, "ring size must be a power of two");

public:
    static constexpr std::uint32_t kMask = N - 1;

    RingReader(std::atomic<std::uint32_t> &writePos, std::atomic<std::uint32_t> &readPos,
               const std::atomic<std::uint32_t> &writeStart, const float *buf) noexcept
        : writePos_(writePos), readPos_(readPos), writeStart_(writeStart), buf_(buf) {}

    // Number of samples currently readable (capped at the ring size).
    std::size_t available() const noexcept {
        const std::uint32_t w = writePos_.load(std::memory_order_acquire);
        const std::uint32_t r = readPos_.load(std::memory_order_relaxed);
        const std::uint32_t lag = w - r;
        return lag > N ? N : lag;
    }

    // Copies up to maxCount samples into dst and returns how many were valid.
    std::size_t pop(float *dst, std::size_t maxCount) noexcept {
        const std::uint32_t w = writePos_.load(std::memory_order_acquire);
        std::uint32_t r = readPos_.load(std::memory_order_relaxed);

        std::uint32_t lag = w - r;
        if (lag > N) {
            const std::uint32_t skipped = lag - N;
            dropped_ += skipped;
            r += skipped;
            lag = N;
        }

        const std::size_t n = std::min<std::size_t>(lag, maxCount);
        if (n == 0) {
            return 0;
        }

        for (std::size_t i = 0; i < n; ++i) {
            dst[i] = buf_[(r + static_cast<std::uint32_t>(i)) & kMask];
        }

        // Indices below (announced end - N) may have had their slots reused during the copy.
        std::atomic_thread_fence(std::memory_order_acquire);
        const std::uint32_t announced = writeStart_.load(std::memory_order_relaxed);
        const std::uint32_t lag2 = announced - r;
        std::size_t torn = 0;
        if (lag2 > N) {
            torn = std::min<std::size_t>(lag2 - N, n);
        }
        if (torn > 0) {
            dropped_ += torn;
            std::memmove(dst, dst + torn, (n - torn) * sizeof(float));
        }

        readPos_.store(r + static_cast<std::uint32_t>(n), std::memory_order_release);
        return n - torn;
    }

    // Total samples skipped because the reader fell behind.
    std::uint64_t dropped() const noexcept { return dropped_; }

private:
    std::atomic<std::uint32_t> &writePos_;
    std::atomic<std::uint32_t> &readPos_;
    const std::atomic<std::uint32_t> &writeStart_;
    const float *buf_;
    std::uint64_t dropped_ = 0;
};

} // namespace sf
