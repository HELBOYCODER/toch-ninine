#pragma once

namespace sf {

struct AdsrParams {
    float attack = 0.005f; // seconds, 0 -> instant
    float decay = 0.12f;   // seconds from peak to sustain
    float sustain = 0.65f; // 0..1 level held while the gate is open
    float release = 0.2f;  // seconds from full level to silence
};

// Per-track envelope. Drives the iTrackN uniform as level * kTrackScale.
class Adsr {
public:
    enum class Stage { Idle, Attack, Decay, Sustain, Release };

    void setParams(const AdsrParams &params) noexcept;
    void setGate(bool open) noexcept;

    // Advances the envelope by dtSeconds and returns the level in [0, 1].
    float process(float dtSeconds) noexcept;

    float level() const noexcept { return level_; }
    Stage stage() const noexcept { return stage_; }

private:
    AdsrParams params_{};
    float level_ = 0.0f;
    bool gate_ = false;
    Stage stage_ = Stage::Idle;
};

// iTrackN = level * kTrackScale, so shaders see a 0..4 range.
inline constexpr float kTrackScale = 4.0f;

// Input peak below this value closes the gate.
inline constexpr float kGateThreshold = 0.01f;

} // namespace sf
