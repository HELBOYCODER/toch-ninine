// POSIX implementation of SharedBus (Linux and macOS). Windows builds skip this file.

#include "shared/Bus.h"

#ifndef _WIN32

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>

namespace sf {

std::unique_ptr<SharedBus> SharedBus::create(const std::string &name) {
    const std::size_t size = sizeof(Bus);
    // A crashed owner can leave the object behind; replace it so the new owner starts clean.
    shm_unlink(name.c_str());
    const int fd = shm_open(name.c_str(), O_CREAT | O_EXCL | O_RDWR, 0600);
    if (fd < 0) {
        return nullptr;
    }
    if (ftruncate(fd, static_cast<off_t>(size)) != 0) {
        close(fd);
        shm_unlink(name.c_str());
        return nullptr;
    }
    void *view = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (view == MAP_FAILED) {
        close(fd);
        shm_unlink(name.c_str());
        return nullptr;
    }
    initBus(view);
    return std::unique_ptr<SharedBus>(
        new SharedBus(name, static_cast<Bus *>(view), size, true, fd));
}

std::unique_ptr<SharedBus> SharedBus::attach(const std::string &name) {
    const int fd = shm_open(name.c_str(), O_RDWR, 0600);
    if (fd < 0) {
        return nullptr;
    }
    const std::size_t size = sizeof(Bus);
    struct stat info {};
    if (fstat(fd, &info) != 0 || static_cast<std::size_t>(info.st_size) < size) {
        close(fd);
        return nullptr;
    }
    void *view = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (view == MAP_FAILED) {
        close(fd);
        return nullptr;
    }
    Bus *bus = static_cast<Bus *>(view);
    if (!isValidBus(bus)) {
        munmap(view, size);
        close(fd);
        return nullptr;
    }
    bus->liveSenders.fetch_add(1, std::memory_order_acq_rel);
    return std::unique_ptr<SharedBus>(new SharedBus(name, bus, size, false, fd));
}

SharedBus::~SharedBus() {
    if (bus_ == nullptr) {
        return;
    }
    if (!owner_) {
        bus_->liveSenders.fetch_sub(1, std::memory_order_acq_rel);
    }
    munmap(bus_, size_);
    close(handle_);
    if (owner_) {
        shm_unlink(name_.c_str());
    }
}

} // namespace sf

#endif // !_WIN32
