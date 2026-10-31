#pragma once

#include <atomic>
#include <cstdint>
#include <cstring>
#include <type_traits>

namespace sf {

// Single-writer, multi-reader snapshot of a trivially copyable value (a seqlock).
// The payload is stored as relaxed atomic words, so concurrent reads are race-free.
// The writer never blocks. Readers retry while a write is in progress, which is
// fine for UI-to-render handoff but must not be used on the audio thread.
template <typename T> class Snapshot {
    static_assert(std::is_trivially_copyable<T>::value,
                  "Snapshot payload must be trivially copyable");
    static_assert(sizeof(T) % sizeof(std::uint32_t) == 0,
                  "Snapshot payload size must be a multiple of 4");

    static constexpr std::size_t kWords = sizeof(T) / sizeof(std::uint32_t);

public:
    Snapshot() {
        const T zero{};
        store(zero);
    }

    void write(const T &value) noexcept {
        const std::uint32_t s = seq_.load(std::memory_order_relaxed);
        seq_.store(s + 1, std::memory_order_relaxed); // odd: write in progress
        std::atomic_thread_fence(std::memory_order_release);
        store(value);
        seq_.store(s + 2, std::memory_order_release); // even: published
    }

    T read() const noexcept {
        for (;;) {
            const std::uint32_t s1 = seq_.load(std::memory_order_acquire);
            if ((s1 & 1u) != 0) {
                continue;
            }
            std::uint32_t words[kWords];
            for (std::size_t i = 0; i < kWords; ++i) {
                words[i] = words_[i].load(std::memory_order_relaxed);
            }
            std::atomic_thread_fence(std::memory_order_acquire);
            if (seq_.load(std::memory_order_relaxed) == s1) {
                T out;
                std::memcpy(&out, words, sizeof(T));
                return out;
            }
        }
    }

private:
    void store(const T &value) noexcept {
        std::uint32_t words[kWords];
        std::memcpy(words, &value, sizeof(T));
        for (std::size_t i = 0; i < kWords; ++i) {
            words_[i].store(words[i], std::memory_order_relaxed);
        }
    }

    std::atomic<std::uint32_t> seq_{0};
    std::atomic<std::uint32_t> words_[kWords]{};
};

} // namespace sf
