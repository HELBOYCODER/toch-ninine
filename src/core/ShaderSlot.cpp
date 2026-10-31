#include "core/ShaderSlot.h"

namespace sf {

ShaderSlot::ShaderSlot(Platform platform, CompileFn compile)
    : platform_(platform), compile_(std::move(compile)) {}

SubmitResult ShaderSlot::submit(const std::string &userSource) {
    const PreprocessResult pre = ShaderPreprocessor::preprocess(userSource, platform_);

    SubmitResult result;
    result.params = pre.pragmas.params;
    result.warnings = pre.pragmas.warnings;

    const CompileOutput output = compile_(pre.source);
    if (output.ok) {
        activeSource_ = userSource;
        activeParams_ = pre.pragmas.params;
        hasActive_ = true;
        result.accepted = true;
        return result;
    }

    result.rolledBack = true;
    result.diagnostics = parseCompileLog(output.log);
    shiftDiagnosticLines(result.diagnostics, -pre.preambleLines);
    if (result.diagnostics.empty()) {
        // The driver failed without a parseable message. Report the failure anyway.
        Diagnostic generic;
        generic.line = 1;
        generic.message = output.log.empty() ? "shader failed to compile" : output.log;
        result.diagnostics.push_back(generic);
    }
    return result;
}

void Debouncer::touch(double nowMs) noexcept {
    lastTouchMs_ = nowMs;
    pending_ = true;
}

bool Debouncer::poll(double nowMs) noexcept {
    if (!pending_ || nowMs - lastTouchMs_ < delayMs_) {
        return false;
    }
    pending_ = false;
    return true;
}

} // namespace sf
