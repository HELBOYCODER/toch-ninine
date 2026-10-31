#pragma once

#include <string>
#include <vector>

namespace sf {

enum class DiagSeverity { Error, Warning };

// One compiler message, with a 1-based line in the source that was compiled.
struct Diagnostic {
    int line = 0;
    int column = 0; // 0 when the driver does not report one
    DiagSeverity severity = DiagSeverity::Error;
    std::string message;
};

// Parses a GLSL info log produced by any of the supported drivers:
//   Mesa / AMD (Linux):  0:12(5): error: syntax error
//   NVIDIA:              0(12) : error C0000: syntax error
//   Apple / ANGLE:       ERROR: 0:12: 'x' : undeclared identifier
// Lines that match none of the formats are ignored.
std::vector<Diagnostic> parseCompileLog(const std::string &log);

// Moves every diagnostic by delta lines (used to undo the injected preamble).
// Diagnostics that end up before line 1 are clamped to line 1.
void shiftDiagnosticLines(std::vector<Diagnostic> &diagnostics, int delta);

} // namespace sf
