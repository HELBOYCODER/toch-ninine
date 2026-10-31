#include "core/ShaderPreprocessor.h"

#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <set>
#include <sstream>

namespace sf {

namespace {

constexpr int kTrackCount = 8;
constexpr int kCustomCount = 32;

bool isIdentStart(char c) {
    return std::isalpha(static_cast<unsigned char>(c)) != 0 || c == '_';
}

bool isIdentChar(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_';
}

bool isValidIdentifier(const std::string &name) {
    if (name.empty() || !isIdentStart(name[0])) {
        return false;
    }
    for (char c : name) {
        if (!isIdentChar(c)) {
            return false;
        }
    }
    return true;
}

// Splits on '\n' and strips a trailing '\r'. A final line without a newline is kept.
std::vector<std::string> splitLines(const std::string &text) {
    std::vector<std::string> lines;
    std::string current;
    for (char c : text) {
        if (c == '\n') {
            if (!current.empty() && current.back() == '\r') {
                current.pop_back();
            }
            lines.push_back(current);
            current.clear();
        } else {
            current.push_back(c);
        }
    }
    if (!current.empty()) {
        if (current.back() == '\r') {
            current.pop_back();
        }
        lines.push_back(current);
    }
    return lines;
}

std::vector<std::string> tokenize(const std::string &line) {
    std::istringstream stream(line);
    std::vector<std::string> tokens;
    std::string token;
    while (stream >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

bool parseFloatStrict(const std::string &text, float &out) {
    if (text.empty()) {
        return false;
    }
    errno = 0;
    char *end = nullptr;
    const double value = std::strtod(text.c_str(), &end);
    if (errno != 0 || end == nullptr || *end != '\0') {
        return false;
    }
    out = static_cast<float>(value);
    return true;
}

// Returns true for lines whose first directive is `#version`.
bool isVersionLine(const std::string &line) {
    std::size_t pos = line.find_first_not_of(" \t");
    if (pos == std::string::npos || line[pos] != '#') {
        return false;
    }
    pos = line.find_first_not_of(" \t", pos + 1);
    return pos != std::string::npos && line.compare(pos, 7, "version") == 0 &&
           (pos + 7 == line.size() || !isIdentChar(line[pos + 7]));
}

// Returns true for `#pragma param ...` lines (malformed ones included).
bool isParamPragma(const std::string &line, std::vector<std::string> *tokensOut) {
    std::size_t pos = line.find_first_not_of(" \t");
    if (pos == std::string::npos || line[pos] != '#') {
        return false;
    }
    const std::vector<std::string> tokens = tokenize(line.substr(pos + 1));
    if (tokens.size() < 2 || tokens[0] != "pragma" || tokens[1] != "param") {
        return false;
    }
    if (tokensOut != nullptr) {
        tokensOut->assign(tokens.begin() + 2, tokens.end());
    }
    return true;
}

std::string buildPreamble(const std::vector<PragmaParam> &params, Platform platform) {
    std::ostringstream out;
    out << "#version 150\n";
    out << "#define " << platformMacro(platform) << " 1\n";
    out << "in vec2 uv;\n";
    out << "out vec4 fragColor;\n";
    out << "uniform vec2 uResolution;\n";
    out << "uniform float uTime;\n";
    out << "uniform float uBeatPhase;\n";
    out << "uniform float ";
    for (int i = 0; i < kTrackCount; ++i) {
        out << "iTrack" << i << (i + 1 < kTrackCount ? ", " : ";\n");
    }
    out << "uniform float ";
    for (int i = 0; i < kCustomCount; ++i) {
        out << "uCustom" << i << (i + 1 < kCustomCount ? ", " : ";\n");
    }
    out << "uniform float uWet;\n";
    out << "uniform sampler2D uSrc;\n";
    for (const PragmaParam &param : params) {
        out << "#define " << param.name << " uCustom" << param.index << "\n";
    }
    return out.str();
}

int countNewlines(const std::string &text) {
    int count = 0;
    for (char c : text) {
        if (c == '\n') {
            ++count;
        }
    }
    return count;
}

} // namespace

const char *platformMacro(Platform platform) noexcept {
    switch (platform) {
    case Platform::Windows:
        return "SF_PLATFORM_WINDOWS";
    case Platform::MacOS:
        return "SF_PLATFORM_MACOS";
    case Platform::Linux:
        return "SF_PLATFORM_LINUX";
    }
    return "SF_PLATFORM_UNKNOWN";
}

bool ShaderPreprocessor::isReservedName(const std::string &name) noexcept {
    static const std::set<std::string> kReserved = {
        "uv",   "fragColor", "uResolution", "uTime", "uBeatPhase", "uWet",  "uSrc",    "main",
        "void", "float",     "int",         "vec2",  "vec3",       "vec4",  "uniform", "in",
        "out",  "return",    "if",          "else",  "for",        "while", "const",   "sampler2D",
    };
    if (kReserved.count(name) != 0 || name.rfind("SF_", 0) == 0) {
        return true;
    }
    // iTrackN and uCustomN are the fixed uniform channels.
    for (const char *prefix : {"iTrack", "uCustom"}) {
        const std::string p(prefix);
        if (name.size() > p.size() && name.compare(0, p.size(), p) == 0) {
            const std::string digits = name.substr(p.size());
            if (digits.find_first_not_of("0123456789") == std::string::npos) {
                return true;
            }
        }
    }
    return false;
}

PragmaParseResult ShaderPreprocessor::parsePragmas(const std::string &userSource) {
    PragmaParseResult result;
    int lineNumber = 0;
    for (const std::string &line : splitLines(userSource)) {
        ++lineNumber;
        std::vector<std::string> args;
        if (!isParamPragma(line, &args)) {
            continue; // unknown pragmas are ignored, as the spec requires
        }
        const std::string where = "line " + std::to_string(lineNumber) + ": ";
        if (args.size() != 4) {
            result.warnings.push_back(where +
                                      "expected '#pragma param <name> <min> <max> <default>'");
            continue;
        }
        PragmaParam param;
        param.name = args[0];
        if (!isValidIdentifier(param.name)) {
            result.warnings.push_back(where + "invalid parameter name '" + param.name + "'");
            continue;
        }
        if (isReservedName(param.name)) {
            result.warnings.push_back(where + "'" + param.name + "' is a reserved name");
            continue;
        }
        bool duplicate = false;
        for (const PragmaParam &existing : result.params) {
            duplicate = duplicate || existing.name == param.name;
        }
        if (duplicate) {
            result.warnings.push_back(where + "duplicate parameter '" + param.name + "'");
            continue;
        }
        if (!parseFloatStrict(args[1], param.minValue) ||
            !parseFloatStrict(args[2], param.maxValue) ||
            !parseFloatStrict(args[3], param.defaultValue)) {
            result.warnings.push_back(where + "min, max and default must be numbers");
            continue;
        }
        if (param.minValue > param.maxValue) {
            result.warnings.push_back(where + "min is greater than max");
            continue;
        }
        if (param.defaultValue < param.minValue || param.defaultValue > param.maxValue) {
            result.warnings.push_back(where + "default is outside [min, max]");
            continue;
        }
        if (static_cast<int>(result.params.size()) >= kMaxParamsPerShader) {
            result.warnings.push_back(where + "more than 8 parameters, ignored");
            continue;
        }
        param.index = static_cast<int>(result.params.size());
        result.params.push_back(param);
    }
    return result;
}

PreprocessResult ShaderPreprocessor::preprocess(const std::string &userSource, Platform platform) {
    PreprocessResult result;
    result.pragmas = parsePragmas(userSource);

    const std::string preamble = buildPreamble(result.pragmas.params, platform);
    result.preambleLines = countNewlines(preamble);

    std::ostringstream body;
    for (const std::string &line : splitLines(userSource)) {
        if (isVersionLine(line) || isParamPragma(line, nullptr)) {
            body << '\n'; // blank the line but keep the line count
        } else {
            body << line << '\n';
        }
    }

    result.source = preamble + body.str();
    return result;
}

std::string ShaderPreprocessor::inject(const std::string &userSource, Platform platform) {
    return preprocess(userSource, platform).source;
}

} // namespace sf
