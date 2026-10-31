// Platform-independent bus helpers, plus the Windows mapping implementation.
// The POSIX implementation lives in BusPosix.cpp.

#include "shared/Bus.h"

#include <utility>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace sf {

std::string busName(const std::string &uid) {
#ifdef _WIN32
    return "Local\\ShaderForge_Bus_" + uid;
#else
    return "/ShaderForge_Bus_" + uid;
#endif
}

bool isValidBus(const Bus *bus) noexcept {
    return bus != nullptr && bus->magic.load(std::memory_order_acquire) == kMagic;
}

void initBus(void *memory) noexcept {
    Bus *bus = new (memory) Bus();
    bus->magic.store(kMagic, std::memory_order_release);
}

#ifdef _WIN32

std::unique_ptr<SharedBus> SharedBus::create(const std::string &name) {
    const std::size_t size = sizeof(Bus);
    HANDLE handle = CreateFileMappingA(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE,
                                       static_cast<DWORD>(size >> 32),
                                       static_cast<DWORD>(size & 0xffffffffu), name.c_str());
    if (handle == nullptr || GetLastError() == ERROR_ALREADY_EXISTS) {
        if (handle != nullptr) {
            CloseHandle(handle);
        }
        return nullptr;
    }
    void *view = MapViewOfFile(handle, FILE_MAP_ALL_ACCESS, 0, 0, size);
    if (view == nullptr) {
        CloseHandle(handle);
        return nullptr;
    }
    initBus(view);
    return std::unique_ptr<SharedBus>(
        new SharedBus(name, static_cast<Bus *>(view), size, true, handle));
}

std::unique_ptr<SharedBus> SharedBus::attach(const std::string &name) {
    HANDLE handle = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, name.c_str());
    if (handle == nullptr) {
        return nullptr;
    }
    const std::size_t size = sizeof(Bus);
    void *view = MapViewOfFile(handle, FILE_MAP_ALL_ACCESS, 0, 0, size);
    if (view == nullptr) {
        CloseHandle(handle);
        return nullptr;
    }
    Bus *bus = static_cast<Bus *>(view);
    if (!isValidBus(bus)) {
        UnmapViewOfFile(view);
        CloseHandle(handle);
        return nullptr;
    }
    bus->liveSenders.fetch_add(1, std::memory_order_acq_rel);
    return std::unique_ptr<SharedBus>(new SharedBus(name, bus, size, false, handle));
}

SharedBus::~SharedBus() {
    if (bus_ == nullptr) {
        return;
    }
    if (!owner_) {
        bus_->liveSenders.fetch_sub(1, std::memory_order_acq_rel);
    }
    UnmapViewOfFile(bus_);
    CloseHandle(handle_);
}

#endif // _WIN32

SharedBus::SharedBus(std::string name, Bus *bus, std::size_t size, bool owner, NativeHandle handle)
    : name_(std::move(name)), bus_(bus), size_(size), owner_(owner), handle_(handle) {}

} // namespace sf
