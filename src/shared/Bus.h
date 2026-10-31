#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <string>

#include "shared/RingBuffer.h"

namespace sf {

// Shared-memory layout shared by ForgeSender (writer) and ShaderForge (reader).
// Changing any of these values changes the on-the-wire format; bump kMagic if so.
constexpr std::uint32_t kMagic = 0x53464742; // "SFGB"
constexpr int kMaxTracks = 16;
constexpr std::uint32_t kRingSize = 16384; // samples per slot, power of two
static_assert((kRingSize & (kRingSize - 1)) == 0, "kRingSize must be a power of two");

struct Slot {
    std::atomic<std::uint32_t> writePos{0};
    std::atomic<std::uint32_t> readPos{0};
    std::atomic<float> peak{0.0f};
    std::atomic<std::uint32_t> sampleRate{48000};
    float buf[kRingSize]{};
    char name[64]{};
    // Appended after the contract fields so their offsets stay the same. Announces the
    // end index of the writer's current push; see RingBuffer.h for the protocol.
    std::atomic<std::uint32_t> writeStart{0};
};

struct Bus {
    std::atomic<std::uint32_t> magic{0};
    std::atomic<std::uint32_t> liveSenders{0};
    std::atomic<std::uint32_t> daemonPid{0};
    Slot slots[kMaxTracks];
};

static_assert(std::atomic<std::uint32_t>::is_always_lock_free,
              "bus counters must be lock-free to be shared across processes");
static_assert(std::atomic<float>::is_always_lock_free, "bus peaks must be lock-free");

using SlotWriter = RingWriter<kRingSize>;
using SlotReader = RingReader<kRingSize>;

// Name of the bus object for a user id. Windows: "Local\ShaderForge_Bus_<uid>".
// POSIX: "/ShaderForge_Bus_<uid>". macOS limits POSIX shm names to 31 characters,
// so callers must keep uid short (8 characters is the recommended length).
std::string busName(const std::string &uid);

// True when the memory block carries the expected magic value.
bool isValidBus(const Bus *bus) noexcept;

// Constructs a Bus in freshly zeroed memory and publishes it by storing the magic last.
void initBus(void *memory) noexcept;

#ifdef _WIN32
using NativeHandle = void *;
#else
using NativeHandle = int;
#endif

// Owns one mapping of the bus. The creator owns the object name; clients
// (ForgeSender instances) register themselves in liveSenders while attached.
class SharedBus {
public:
    // Creates a new bus. Fails if an object with this name already exists (Windows)
    // or if it cannot be created. POSIX replaces a stale object left by a crash.
    static std::unique_ptr<SharedBus> create(const std::string &name);

    // Attaches to an existing bus. Returns nullptr if the object is missing,
    // too small, or does not carry kMagic.
    static std::unique_ptr<SharedBus> attach(const std::string &name);

    ~SharedBus();

    SharedBus(const SharedBus &) = delete;
    SharedBus &operator=(const SharedBus &) = delete;

    Bus &bus() noexcept { return *bus_; }
    const Bus &bus() const noexcept { return *bus_; }
    Slot &slot(int index) noexcept { return bus_->slots[index]; }
    bool isOwner() const noexcept { return owner_; }
    const std::string &name() const noexcept { return name_; }

private:
    SharedBus(std::string name, Bus *bus, std::size_t size, bool owner, NativeHandle handle);

    std::string name_;
    Bus *bus_ = nullptr;
    std::size_t size_ = 0;
    bool owner_ = false;
    NativeHandle handle_{};
};

} // namespace sf
