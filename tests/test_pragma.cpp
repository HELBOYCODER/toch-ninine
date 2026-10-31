#include <string>

#include <catch2/catch_test_macros.hpp>

#include "core/ShaderPreprocessor.h"

TEST_CASE("valid pragma params are parsed in declaration order", "[pragma]") {
    const std::string src = "#pragma param thickness 0.005 0.2 0.05\n"
                            "#pragma param rings 1 12 6\n";
    const auto result = sf::ShaderPreprocessor::parsePragmas(src);

    REQUIRE(result.warnings.empty());
    REQUIRE(result.params.size() == 2);
    REQUIRE(result.params[0].name == "thickness");
    REQUIRE(result.params[0].minValue == 0.005f);
    REQUIRE(result.params[0].maxValue == 0.2f);
    REQUIRE(result.params[0].defaultValue == 0.05f);
    REQUIRE(result.params[0].index == 0);
    REQUIRE(result.params[1].name == "rings");
    REQUIRE(result.params[1].index == 1);
}

TEST_CASE("tabs and extra spacing are accepted", "[pragma]") {
    const auto result =
        sf::ShaderPreprocessor::parsePragmas("\t#  pragma\tparam   speed\t0\t2\t0.4\n");
    REQUIRE(result.warnings.empty());
    REQUIRE(result.params.size() == 1);
    REQUIRE(result.params[0].name == "speed");
}

TEST_CASE("unknown pragmas are ignored silently", "[pragma]") {
    const auto result = sf::ShaderPreprocessor::parsePragmas("#pragma optimize(off)\n");
    REQUIRE(result.params.empty());
    REQUIRE(result.warnings.empty());
}

TEST_CASE("malformed pragma params are rejected with a warning", "[pragma]") {
    struct Case {
        const char *line;
    };
    const Case cases[] = {
        {"#pragma param onlyname 0 1\n"},      // too few tokens
        {"#pragma param a 0 1 0.5 extra\n"},   // too many tokens
        {"#pragma param 1abc 0 1 0.5\n"},      // identifier starts with a digit
        {"#pragma param a-b 0 1 0.5\n"},       // illegal character
        {"#pragma param uTime 0 1 0.5\n"},     // reserved name
        {"#pragma param iTrack3 0 1 0.5\n"},   // reserved channel name
        {"#pragma param a zero 1 0.5\n"},      // non-numeric bound
        {"#pragma param a 1 0 0.5\n"},         // min greater than max
        {"#pragma param a 0 1 1.5\n"},         // default outside range
        {"#pragma param a 0 1 0.5trailing\n"}, // trailing junk after number
    };
    for (const Case &c : cases) {
        INFO(c.line);
        const auto result = sf::ShaderPreprocessor::parsePragmas(c.line);
        REQUIRE(result.params.empty());
        REQUIRE(result.warnings.size() == 1);
    }
}

TEST_CASE("duplicate names keep the first declaration", "[pragma]") {
    const auto result =
        sf::ShaderPreprocessor::parsePragmas("#pragma param a 0 1 0.1\n#pragma param a 0 1 0.9\n");
    REQUIRE(result.params.size() == 1);
    REQUIRE(result.params[0].defaultValue == 0.1f);
    REQUIRE(result.warnings.size() == 1);
}

TEST_CASE("at most eight parameters per shader", "[pragma]") {
    std::string src;
    for (int i = 0; i < 10; ++i) {
        src += "#pragma param p" + std::to_string(i) + " 0 1 0\n";
    }
    const auto result = sf::ShaderPreprocessor::parsePragmas(src);
    REQUIRE(result.params.size() == sf::ShaderPreprocessor::kMaxParamsPerShader);
    REQUIRE(result.params.back().name == "p7");
    REQUIRE(result.warnings.size() == 2);
}
