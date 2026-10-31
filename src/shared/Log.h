#pragma once

#include <functional>
#include <string>

namespace sf::log {

enum class Level { Debug, Info, Warn, Error };

using Sink = std::function<void(Level, const std::string &)>;

// Replaces the active sink. Passing an empty function silences logging.
void setSink(Sink sink);

// Thread-safe. Messages are dropped when no sink is installed.
void write(Level level, const std::string &message);

const char *levelName(Level level) noexcept;

} // namespace sf::log
