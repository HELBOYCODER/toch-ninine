#pragma once

#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "core/ShaderPreprocessor.h"
#include "shared/Diag.h"

namespace sf {

struct CompileOutput {
    bool ok = false;
    std::string log; // raw driver info log
};

// Compiles one final source (preamble included). On the GL thread in production.
using CompileFn = std::function<CompileOutput(const std::string &finalSource)>;

struct SubmitResult {
    bool accepted = false;               // the new source compiled and is now active
    bool rolledBack = false;             // the new source failed; the last good one stays active
    std::vector<Diagnostic> diagnostics; // line numbers refer to the author's source
    std::vector<PragmaParam> params;
    std::vector<std::string> warnings;
};

// Holds the active shader. Submitting a source that fails to compile never
// changes the active source (rollback).
class ShaderSlot {
public:
    ShaderSlot(Platform platform, CompileFn compile);

    SubmitResult submit(const std::string &userSource);

    bool hasActive() const noexcept { return hasActive_; }
    const std::string &activeSource() const noexcept { return activeSource_; }
    const std::vector<PragmaParam> &activeParams() const noexcept { return activeParams_; }

private:
    Platform platform_;
    CompileFn compile_;
    bool hasActive_ = false;
    std::string activeSource_;
    std::vector<PragmaParam> activeParams_;
};

// Coalesces bursts of edits. Fires once, delayMs after the last touch.
class Debouncer {
public:
    explicit Debouncer(double delayMs) noexcept : delayMs_(delayMs) {}

    void touch(double nowMs) noexcept;
    // Returns true once when the quiet period has elapsed since the last touch.
    bool poll(double nowMs) noexcept;

private:
    double delayMs_;
    double lastTouchMs_ = 0.0;
    bool pending_ = false;
};

// Quiet period after the last edit before the shader is compiled (§4.4 F4).
inline constexpr double kShaderDebounceMs = 350.0;

// Live-edit path: every edit is debounced, and only the newest text in a burst is
// compiled through the slot, so a failed compile keeps the last good shader.
class ShaderEditSession {
public:
    explicit ShaderEditSession(ShaderSlot &slot, double debounceMs = kShaderDebounceMs);

    void edit(const std::string &text, double nowMs);

    // Returns the result once the quiet period has elapsed; nullopt otherwise.
    std::optional<SubmitResult> poll(double nowMs);

    bool pending() const noexcept { return hasPending_; }

private:
    ShaderSlot &slot_;
    Debouncer debouncer_;
    std::string pendingText_;
    bool hasPending_ = false;
};

} // namespace sf
