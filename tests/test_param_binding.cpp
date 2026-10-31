#include <catch2/catch_test_macros.hpp>

#include "core/ParamBinding.h"
#include "core/ShaderPreprocessor.h"

namespace {

std::vector<sf::PragmaParam> paramsFrom(const char *source) {
    return sf::ShaderPreprocessor::parsePragmas(source).params;
}

} // namespace

TEST_CASE("shader pragma params become host params at the uCustom ids", "[params][binding]") {
    sf::ParamRegistry registry;
    const auto params = paramsFrom("#pragma param thickness 0 1 0.05\n"
                                   "#pragma param rings 1 12 6\n");
    REQUIRE(sf::registerShaderParams(registry, params));

    const sf::ParamDesc *first = registry.find(sf::param::kCustom);
    REQUIRE(first != nullptr);
    REQUIRE(first->name == "thickness");
    REQUIRE(first->maxValue == 1.0f);
    REQUIRE(first->defaultValue == 0.05f);

    const sf::ParamDesc *second = registry.find(sf::param::kCustom + 1);
    REQUIRE(second != nullptr);
    REQUIRE(second->name == "rings");
}

TEST_CASE("registering a new shader replaces the previous custom params", "[params][binding]") {
    sf::ParamRegistry registry;
    REQUIRE(sf::registerShaderParams(registry, paramsFrom("#pragma param a 0 1 0\n"
                                                          "#pragma param b 0 1 0\n")));
    REQUIRE(sf::registerShaderParams(registry, paramsFrom("#pragma param only 0 1 0\n")));

    REQUIRE(registry.find(sf::param::kCustom)->name == "only");
    REQUIRE(registry.find(sf::param::kCustom + 1) == nullptr);
}

TEST_CASE("an empty shader clears the custom range and keeps fixed params", "[params][binding]") {
    sf::ParamRegistry registry;
    REQUIRE(registry.add({sf::param::kMasterGain, "Master Gain", 0.0f, 2.0f, 1.0f}));
    REQUIRE(sf::registerShaderParams(registry, paramsFrom("#pragma param a 0 1 0\n")));
    REQUIRE(sf::registerShaderParams(registry, {}));

    REQUIRE(registry.find(sf::param::kCustom) == nullptr);
    REQUIRE(registry.find(sf::param::kMasterGain) != nullptr);
}

TEST_CASE("removeRange only touches ids inside the range", "[params]") {
    sf::ParamRegistry registry;
    REQUIRE(registry.add({100, "a", 0.0f, 1.0f, 0.0f}));
    REQUIRE(registry.add({200, "b", 0.0f, 1.0f, 0.0f}));
    REQUIRE(registry.add({300, "c", 0.0f, 1.0f, 0.0f}));
    REQUIRE(registry.removeRange(150, 250) == 1);
    REQUIRE(registry.find(100) != nullptr);
    REQUIRE(registry.find(200) == nullptr);
    REQUIRE(registry.find(300) != nullptr);
}
