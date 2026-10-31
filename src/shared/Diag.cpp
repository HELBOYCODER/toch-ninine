#include "shared/Diag.h"

#include <algorithm>
#include <regex>
#include <sstream>

namespace sf {

namespace {

DiagSeverity severityFromWord(const std::string &word) {
    return word == "warning" || word == "WARNING" ? DiagSeverity::Warning : DiagSeverity::Error;
}

// Mesa / AMD: "0:12(5): error: message"
bool parseMesa(const std::string &line, Diagnostic &out) {
    static const std::regex kPattern(R"(^\s*\d+:(\d+)\((\d+)\):\s*(error|warning):\s*(.*)$)");
    std::smatch m;
    if (!std::regex_match(line, m, kPattern)) {
        return false;
    }
    out.line = std::stoi(m[1].str());
    out.column = std::stoi(m[2].str());
    out.severity = severityFromWord(m[3].str());
    out.message = m[4].str();
    return true;
}

// NVIDIA: "0(12) : error C0000: message"  (the Cxxxx code is optional)
bool parseNvidia(const std::string &line, Diagnostic &out) {
    static const std::regex kPattern(
        R"(^\s*\d+\((\d+)\)\s*:\s*(error|warning)\s*(?:C\d+)?:?\s*(.*)$)");
    std::smatch m;
    if (!std::regex_match(line, m, kPattern)) {
        return false;
    }
    out.line = std::stoi(m[1].str());
    out.column = 0;
    out.severity = severityFromWord(m[2].str());
    out.message = m[3].str();
    return true;
}

// Apple / ANGLE: "ERROR: 0:12: 'x' : undeclared identifier"
bool parseApple(const std::string &line, Diagnostic &out) {
    static const std::regex kPattern(R"(^\s*(ERROR|WARNING):\s*\d+:(\d+):\s*(.*)$)");
    std::smatch m;
    if (!std::regex_match(line, m, kPattern)) {
        return false;
    }
    out.line = std::stoi(m[2].str());
    out.column = 0;
    out.severity = severityFromWord(m[1].str());
    out.message = m[3].str();
    return true;
}

} // namespace

std::vector<Diagnostic> parseCompileLog(const std::string &log) {
    std::vector<Diagnostic> result;
    std::istringstream stream(log);
    std::string line;
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        Diagnostic diag;
        if (parseMesa(line, diag) || parseNvidia(line, diag) || parseApple(line, diag)) {
            result.push_back(diag);
        }
    }
    return result;
}

void shiftDiagnosticLines(std::vector<Diagnostic> &diagnostics, int delta) {
    for (Diagnostic &diag : diagnostics) {
        diag.line = std::max(1, diag.line + delta);
    }
}

} // namespace sf
