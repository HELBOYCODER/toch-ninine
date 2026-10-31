#include <string>

#include <catch2/catch_test_macros.hpp>

#include "core/ShaderPreprocessor.h"
#include "core/ShaderSlot.h"

namespace {

std::size_t countOccurrences(const std::string &haystack, const std::string &needle) {
    std::size_t count = 0;
    for (std::size_t pos = haystack.find(needle); pos != std::string::npos;
         pos = haystack.find(needle, pos + needle.size())) {
        ++count;
    }
    return count;
}

// 1-based line of the first occurrence of marker, or -1 when absent.
int lineOf(const std::string &source, const std::string &marker) {
    const std::size_t hit = source.find(marker);
    if (hit == std::string::npos) {
        return -1;
    }
    int line = 1;
    for (std::size_t i = 0; i < hit; ++i) {
        if (source[i] == '\n') {
            ++line;
        }
    }
    return line;
}

} // namespace

TEST_CASE("inject adds exactly one #version when the author has none", "[preprocessor]") {
    const std::string out = sf::ShaderPreprocessor::inject("void main() {}\n", sf::Platform::Linux);
    REQUIRE(countOccurrences(out, "#version") == 1);
    REQUIRE(out.rfind("#version 150", 0) == 0);
}

TEST_CASE("inject survives a source that already has #version", "[preprocessor]") {
    const std::string src = "#version 330 core\nvoid main() {}\n";
    const std::string out = sf::ShaderPreprocessor::inject(src, sf::Platform::MacOS);
    REQUIRE(countOccurrences(out, "#version") == 1);
    REQUIRE(out.find("#version 330") == std::string::npos);
    REQUIRE(out.find("void main() {}") != std::string::npos);
}

TEST_CASE("preamble declares the fixed uniform contract", "[preprocessor]") {
    const std::string out = sf::ShaderPreprocessor::inject("", sf::Platform::Windows);
    REQUIRE(out.find("in vec2 uv;") != std::string::npos);
    REQUIRE(out.find("out vec4 fragColor;") != std::string::npos);
    REQUIRE(out.find("uniform vec2 uResolution;") != std::string::npos);
    REQUIRE(out.find("uniform float uTime;") != std::string::npos);
    REQUIRE(out.find("uniform float uBeatPhase;") != std::string::npos);
    REQUIRE(out.find("iTrack0") != std::string::npos);
    REQUIRE(out.find("iTrack7") != std::string::npos);
    REQUIRE(out.find("uCustom0") != std::string::npos);
    REQUIRE(out.find("uCustom31") != std::string::npos);
    REQUIRE(out.find("uniform float uWet;") != std::string::npos);
    REQUIRE(out.find("uniform sampler2D uSrc;") != std::string::npos);
    REQUIRE(out.find("#define SF_PLATFORM_WINDOWS 1") != std::string::npos);
}

TEST_CASE("pragma params become macros over uCustom slots", "[preprocessor]") {
    const std::string src = "#pragma param thickness 0 1 0.5\n"
                            "#pragma param rings 1 12 6\n"
                            "void main() { float t = thickness + rings; }\n";
    const auto pre = sf::ShaderPreprocessor::preprocess(src, sf::Platform::Linux);

    REQUIRE(pre.source.find("#define thickness uCustom0") != std::string::npos);
    REQUIRE(pre.source.find("#define rings uCustom1") != std::string::npos);
    REQUIRE(pre.source.find("#pragma param") == std::string::npos);
    REQUIRE(pre.pragmas.params.size() == 2);
}

TEST_CASE("author lines keep their number after the preamble", "[preprocessor]") {
    const std::string src = "#version 150\nfloat a = 1.0;\nMARKER_LINE\n";
    const auto pre = sf::ShaderPreprocessor::preprocess(src, sf::Platform::Linux);
    // The author's third line sits at preambleLines + 3 in the final source.
    REQUIRE(lineOf(pre.source, "MARKER_LINE") == pre.preambleLines + 3);
    REQUIRE(pre.preambleLines > 0);
}

TEST_CASE("compile errors map back to the author's line numbers", "[preprocessor][slot]") {
    // Fake driver: reports an error on the final line that holds "BROKEN".
    sf::CompileFn fakeDriver = [](const std::string &finalSource) {
        sf::CompileOutput out;
        const int line = lineOf(finalSource, "BROKEN");
        if (line < 0) {
            out.ok = true;
            return out;
        }
        out.ok = false;
        out.log = "ERROR: 0:" + std::to_string(line) + ": undeclared identifier\n";
        return out;
    };
    sf::ShaderSlot slot(sf::Platform::Linux, fakeDriver);

    const auto result = slot.submit("void main() {\n  fragColor = vec4(1.0);\n  BROKEN;\n}\n");
    REQUIRE(result.rolledBack);
    REQUIRE(result.diagnostics.size() == 1);
    REQUIRE(result.diagnostics[0].line == 3);
}
