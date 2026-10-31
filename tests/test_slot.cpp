#include <string>

#include <catch2/catch_test_macros.hpp>

#include "core/ShaderSlot.h"

namespace {

// Driver stub that rejects any source containing the word "BROKEN".
sf::CompileFn strictDriver(int &calls) {
    return [&calls](const std::string &finalSource) {
        ++calls;
        sf::CompileOutput out;
        const std::size_t pos = finalSource.find("BROKEN");
        if (pos == std::string::npos) {
            out.ok = true;
            return out;
        }
        int line = 1;
        for (std::size_t i = 0; i < pos; ++i) {
            if (finalSource[i] == '\n') {
                ++line;
            }
        }
        out.ok = false;
        out.log = "0(" + std::to_string(line) + ") : error C0000: bad token\n";
        return out;
    };
}

} // namespace

TEST_CASE("a good source becomes active and params are reported", "[slot]") {
    int calls = 0;
    sf::ShaderSlot slot(sf::Platform::Linux, strictDriver(calls));
    const auto result = slot.submit("#pragma param speed 0 2 0.5\nvoid main() {}\n");

    REQUIRE(result.accepted);
    REQUIRE_FALSE(result.rolledBack);
    REQUIRE(slot.hasActive());
    REQUIRE(result.params.size() == 1);
    REQUIRE(slot.activeParams().size() == 1);
    REQUIRE(calls == 1);
}

TEST_CASE("a failing source rolls back to the last good one", "[slot]") {
    int calls = 0;
    sf::ShaderSlot slot(sf::Platform::Linux, strictDriver(calls));
    REQUIRE(slot.submit("void main() { fragColor = vec4(0.0); }\n").accepted);

    const auto bad = slot.submit("void main() {\nBROKEN\n}\n");
    REQUIRE(bad.rolledBack);
    REQUIRE_FALSE(bad.accepted);
    REQUIRE(bad.diagnostics.size() == 1);
    REQUIRE(bad.diagnostics[0].line == 2);
    REQUIRE(slot.activeSource() == "void main() { fragColor = vec4(0.0); }\n");
}

TEST_CASE("a driver failure with no parseable message still reports an error", "[slot]") {
    sf::ShaderSlot slot(sf::Platform::Linux, [](const std::string &) {
        sf::CompileOutput out;
        out.ok = false;
        out.log = "something unexpected";
        return out;
    });
    const auto result = slot.submit("void main() {}\n");
    REQUIRE(result.rolledBack);
    REQUIRE(result.diagnostics.size() == 1);
    REQUIRE(result.diagnostics[0].message == "something unexpected");
}

TEST_CASE("debouncer fires once after the quiet period", "[slot]") {
    sf::Debouncer debounce(350.0);
    REQUIRE_FALSE(debounce.poll(0.0));

    debounce.touch(0.0);
    REQUIRE_FALSE(debounce.poll(100.0));
    debounce.touch(200.0);
    REQUIRE_FALSE(debounce.poll(400.0)); // only 200 ms since the last touch
    REQUIRE(debounce.poll(600.0));
    REQUIRE_FALSE(debounce.poll(900.0)); // fires once per burst
}

TEST_CASE("live edits compile once after the debounce and keep the last good shader",
          "[slot][edit]") {
    REQUIRE(sf::kShaderDebounceMs == 350.0);
    int calls = 0;
    sf::ShaderSlot slot(sf::Platform::Linux, strictDriver(calls));
    sf::ShaderEditSession session(slot);

    session.edit("void main() { fragColor = vec4(0.0); }\n", 0.0);
    session.edit("void main() { fragColor = vec4(0.5); }\n", 100.0);
    REQUIRE_FALSE(session.poll(300.0).has_value()); // still inside the 350 ms window
    REQUIRE(session.pending());

    const auto accepted = session.poll(500.0);
    REQUIRE(accepted.has_value());
    REQUIRE(accepted->accepted);
    REQUIRE(calls == 1); // only the newest text was compiled
    REQUIRE(slot.activeSource() == "void main() { fragColor = vec4(0.5); }\n");

    session.edit("BROKEN\n", 600.0);
    const auto rejected = session.poll(1000.0);
    REQUIRE(rejected.has_value());
    REQUIRE(rejected->rolledBack);
    REQUIRE(slot.activeSource() == "void main() { fragColor = vec4(0.5); }\n");
}
