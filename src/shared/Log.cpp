#include "shared/Log.h"

#include <mutex>

namespace sf::log {

namespace {
std::mutex &sinkMutex() {
    static std::mutex mutex;
    return mutex;
}

Sink &activeSink() {
    static Sink sink;
    return sink;
}
} // namespace

void setSink(Sink sink) {
    std::lock_guard<std::mutex> lock(sinkMutex());
    activeSink() = std::move(sink);
}

void write(Level level, const std::string &message) {
    std::lock_guard<std::mutex> lock(sinkMutex());
    if (activeSink()) {
        activeSink()(level, message);
    }
}

const char *levelName(Level level) noexcept {
    switch (level) {
    case Level::Debug:
        return "DEBUG";
    case Level::Info:
        return "INFO";
    case Level::Warn:
        return "WARN";
    case Level::Error:
        return "ERROR";
    }
    return "UNKNOWN";
}

} // namespace sf::log
