#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "shared/Diag.h"

TEST_CASE("parses Mesa / AMD logs", "[diag]") {
    const std::string log = "Fragment shader failed to compile with the following errors:\n"
                            "0:12(5): error: syntax error, unexpected '}'\n"
                            "0:20(1): warning: extension foo not supported\n";
    const auto diags = sf::parseCompileLog(log);

    REQUIRE(diags.size() == 2);
    REQUIRE(diags[0].line == 12);
    REQUIRE(diags[0].column == 5);
    REQUIRE(diags[0].severity == sf::DiagSeverity::Error);
    REQUIRE(diags[0].message == "syntax error, unexpected '}'");
    REQUIRE(diags[1].line == 20);
    REQUIRE(diags[1].severity == sf::DiagSeverity::Warning);
}

TEST_CASE("parses NVIDIA logs", "[diag]") {
    const std::string log =
        "0(12) : error C0000: syntax error, unexpected '}', expecting ',' or ';'\n"
        "0(18) : warning C7555: extension is not supported\n";
    const auto diags = sf::parseCompileLog(log);

    REQUIRE(diags.size() == 2);
    REQUIRE(diags[0].line == 12);
    REQUIRE(diags[0].column == 0);
    REQUIRE(diags[0].severity == sf::DiagSeverity::Error);
    REQUIRE(diags[0].message == "syntax error, unexpected '}', expecting ',' or ';'");
    REQUIRE(diags[1].line == 18);
    REQUIRE(diags[1].severity == sf::DiagSeverity::Warning);
}

TEST_CASE("parses Apple / ANGLE logs", "[diag]") {
    const std::string log = "ERROR: 0:14: 'foo' : undeclared identifier\n"
                            "WARNING: 0:20: 'x' : possible use of uninitialized variable\n";
    const auto diags = sf::parseCompileLog(log);

    REQUIRE(diags.size() == 2);
    REQUIRE(diags[0].line == 14);
    REQUIRE(diags[0].message == "'foo' : undeclared identifier");
    REQUIRE(diags[0].severity == sf::DiagSeverity::Error);
    REQUIRE(diags[1].line == 20);
    REQUIRE(diags[1].severity == sf::DiagSeverity::Warning);
}

TEST_CASE("ignores lines that match no known format", "[diag]") {
    const auto diags = sf::parseCompileLog("garbage\n\nlink failed\n");
    REQUIRE(diags.empty());
}

TEST_CASE("CRLF logs parse the same as LF logs", "[diag]") {
    const auto diags = sf::parseCompileLog("ERROR: 0:3: boom\r\n");
    REQUIRE(diags.size() == 1);
    REQUIRE(diags[0].line == 3);
    REQUIRE(diags[0].message == "boom");
}

TEST_CASE("shiftDiagnosticLines removes the preamble offset and clamps at line 1", "[diag]") {
    std::vector<sf::Diagnostic> diags = sf::parseCompileLog("ERROR: 0:30: a\nERROR: 0:2: b\n");
    sf::shiftDiagnosticLines(diags, -29);
    REQUIRE(diags[0].line == 1);
    REQUIRE(diags[1].line == 1);

    diags = sf::parseCompileLog("ERROR: 0:40: c\n");
    sf::shiftDiagnosticLines(diags, -29);
    REQUIRE(diags[0].line == 11);
}
