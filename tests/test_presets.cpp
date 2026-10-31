#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "core/ShaderPreprocessor.h"

namespace fs = std::filesystem;

namespace {

std::string readFile(const fs::path &path) {
    std::ifstream in(path, std::ios::binary);
    std::ostringstream text;
    text << in.rdbuf();
    return text.str();
}

std::vector<fs::path> factoryFragments() {
    std::vector<fs::path> frags;
    const fs::path dir = fs::path(SF_SOURCE_DIR) / "presets" / "factory";
    for (const auto &entry : fs::directory_iterator(dir)) {
        if (entry.path().extension() == ".frag") {
            frags.push_back(entry.path());
        }
    }
    return frags;
}

} // namespace

TEST_CASE("there are at least 12 factory presets", "[presets]") {
    REQUIRE(factoryFragments().size() >= 12);
}

TEST_CASE("every factory preset follows the shader contract", "[presets]") {
    // Anything here breaks the uniform contract or the portable GLSL 150 subset.
    const std::regex forbidden(
        R"((#\s*version|#\s*include|\bprecision\b|\btexture\w*\s*\(|\bsampler\w*\b|T[O]DO|Not[I]mplemented))");
    // Every loop must have a literal bound of at most 64 iterations.
    const std::regex boundedLoop(R"(for\s*\(\s*int\s+\w+\s*=\s*-?\d+\s*;\s*\w+\s*<=?\s*(\d+)\s*;)");

    for (const fs::path &frag : factoryFragments()) {
        INFO(frag.string());
        const std::string source = readFile(frag);

        std::smatch hit;
        REQUIRE_FALSE(std::regex_search(source, hit, forbidden));

        // Count `for (` loops and the ones that match the bounded form.
        std::size_t loops = 0;
        for (std::size_t pos = source.find("for ("); pos != std::string::npos;
             pos = source.find("for (", pos + 1)) {
            ++loops;
        }
        std::size_t bounded = 0;
        for (auto it = std::sregex_iterator(source.begin(), source.end(), boundedLoop);
             it != std::sregex_iterator(); ++it) {
            ++bounded;
            REQUIRE(std::stoi((*it)[1].str()) <= 64);
        }
        REQUIRE(loops == bounded);

        REQUIRE(source.find("fragColor =") != std::string::npos);
        REQUIRE(source.find("clamp(") != std::string::npos);

        const auto pre = sf::ShaderPreprocessor::preprocess(source, sf::Platform::Linux);
        REQUIRE(pre.pragmas.warnings.empty());
        REQUIRE(pre.pragmas.params.size() <= sf::ShaderPreprocessor::kMaxParamsPerShader);

        // Each preset has a JSON sidecar with the same stem.
        fs::path sidecar = frag;
        sidecar.replace_extension(".json");
        REQUIRE(fs::exists(sidecar));
    }
}

TEST_CASE("the default preset ring_pulse exists", "[presets]") {
    const fs::path ring = fs::path(SF_SOURCE_DIR) / "presets" / "factory" / "ring_pulse.frag";
    REQUIRE(fs::exists(ring));
}
