#include "shared/Adsr.h"

#include <algorithm>

namespace sf {

namespace {
constexpr float kMinTime = 1e-4f;
}

void Adsr::setParams(const AdsrParams &params) noexcept {
    params_.attack = std::max(params.attack, 0.0f);
    params_.decay = std::max(params.decay, 0.0f);
    params_.sustain = std::clamp(params.sustain, 0.0f, 1.0f);
    params_.release = std::max(params.release, 0.0f);
}

void Adsr::setGate(bool open) noexcept {
    gate_ = open;
}

float Adsr::process(float dtSeconds) noexcept {
    const float dt = std::max(dtSeconds, 0.0f);

    // Gate edges switch stages; retriggering starts the attack from the current level.
    if (gate_) {
        if (stage_ == Stage::Idle || stage_ == Stage::Release) {
            stage_ = Stage::Attack;
        }
    } else if (stage_ == Stage::Attack || stage_ == Stage::Decay || stage_ == Stage::Sustain) {
        stage_ = Stage::Release;
    }

    switch (stage_) {
    case Stage::Idle:
        level_ = 0.0f;
        break;
    case Stage::Attack:
        level_ += dt / std::max(params_.attack, kMinTime);
        if (level_ >= 1.0f) {
            level_ = 1.0f;
            stage_ = Stage::Decay;
        }
        break;
    case Stage::Decay: {
        const float span = 1.0f - params_.sustain;
        level_ -= dt * span / std::max(params_.decay, kMinTime);
        if (level_ <= params_.sustain) {
            level_ = params_.sustain;
            stage_ = Stage::Sustain;
        }
        break;
    }
    case Stage::Sustain:
        level_ = params_.sustain;
        break;
    case Stage::Release:
        level_ -= dt / std::max(params_.release, kMinTime);
        if (level_ <= 0.0f) {
            level_ = 0.0f;
            stage_ = Stage::Idle;
        }
        break;
    }
    return level_;
}

} // namespace sf
