// Prints the final GLSL that ShaderForge would compile for an author's shader file.
// Used by scripts/validate_presets.py to run glslangValidator on the real injected source.
//
// Usage: shaderforge_inject <shader.frag> [--platform windows|macos|linux]
// Exit status: 0 on success, 1 on usage or read errors, 2 when pragma warnings exist.

#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "core/ShaderPreprocessor.h"

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "usage: shaderforge_inject <shader.frag> [--platform windows|macos|linux]\n";
        return 1;
    }

    sf::Platform platform = sf::Platform::Linux;
    for (int i = 2; i + 1 < argc; i += 2) {
        const std::string flag = argv[i];
        const std::string value = argv[i + 1];
        if (flag != "--platform") {
            std::cerr << "unknown option: " << flag << "\n";
            return 1;
        }
        if (value == "windows") {
            platform = sf::Platform::Windows;
        } else if (value == "macos") {
            platform = sf::Platform::MacOS;
        } else if (value == "linux") {
            platform = sf::Platform::Linux;
        } else {
            std::cerr << "unknown platform: " << value << "\n";
            return 1;
        }
    }

    std::ifstream in(argv[1], std::ios::binary);
    if (!in) {
        std::cerr << "cannot read " << argv[1] << "\n";
        return 1;
    }
    std::ostringstream text;
    text << in.rdbuf();

    const sf::PreprocessResult result = sf::ShaderPreprocessor::preprocess(text.str(), platform);
    for (const std::string &warning : result.pragmas.warnings) {
        std::cerr << "warning: " << warning << "\n";
    }
    std::cout << result.source;
    return result.pragmas.warnings.empty() ? 0 : 2;
}
