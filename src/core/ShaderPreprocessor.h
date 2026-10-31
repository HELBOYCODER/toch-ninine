#pragma once

#include <string>
#include <vector>

namespace sf {

enum class Platform { Windows, MacOS, Linux };

const char *platformMacro(Platform platform) noexcept;

// A tunable declared with `#pragma param <name> <min> <max> <default>`.
// The preprocessor maps the i-th parameter to uCustom<i>, so the shader can use
// `name` directly as a float.
struct PragmaParam {
    std::string name;
    float minValue = 0.0f;
    float maxValue = 1.0f;
    float defaultValue = 0.0f;
    int index = 0;
};

struct PragmaParseResult {
    std::vector<PragmaParam> params;
    std::vector<std::string> warnings;
};

struct PreprocessResult {
    std::string source;    // final GLSL handed to the compiler
    int preambleLines = 0; // lines injected before the author's first line
    PragmaParseResult pragmas;
};

class ShaderPreprocessor {
public:
    static constexpr int kMaxParamsPerShader = 8;

    // Returns the final source: the injected preamble followed by the author's
    // code. Any `#version` line in the author's code is blanked, so there is never
    // a duplicate. Blanked lines keep their place so diagnostics stay aligned.
    static PreprocessResult preprocess(const std::string &userSource, Platform platform);
    static std::string inject(const std::string &userSource, Platform platform);

    static PragmaParseResult parsePragmas(const std::string &userSource);
    static bool isReservedName(const std::string &name) noexcept;
};

} // namespace sf
