# Decisions

Every choice that §2 of the build order does not cover, and every deviation from it.
Format: date, decision, reason.

## 2026-10-31

1. **Target branch.** Work is pushed to `claude/shaderforge-vst3-build-gak8ug` in
   `HELBOYCODER/toch-ninine`, not to a new `shaderforge` repository on `main`. Reason: the
   session is scoped to this repository and its designated branch, and the branch must not be
   changed without permission.

2. **Scope of this revision.** This revision delivers the portable core (shared memory bus,
   ring buffer, ADSR, GLSL driver-log parsing, `#pragma param` parser, preamble injection,
   state chunk, rollback slot, seqlock snapshot), its Catch2 tests, the 12 factory presets with
   GLSL 150 validation, CI for the core and the presets, and the docs. The JUCE plugin shells,
   GL rendering, texture sharing, external display, encoders, UI, AI/MCP, licensing, installers,
   signing and the release workflow are **not implemented**. No stubs were written for them.
   Reason: the sandbox cannot build or run Windows/macOS JUCE targets, and writing those parts
   unverified would break §0 rule 3 (no stubs) and §12 (every commit must build).

3. **Commit trailer.** Commits end with `Co-Authored-By: Claude <noreply@anthropic.com>` and the
   `Claude-Session` link. The model name is left out because the system instructions forbid
   model identifiers in commit messages.

4. **Slot layout.** `Slot` gains a trailing `std::atomic<uint32_t> writeStart`. Every field the
   contract lists keeps its offset and type. Reason: with only `writePos`, a reader can copy a
   sample that the writer is overwriting before the writer publishes, so a drop-oldest reader
   could return out-of-order samples. The stress test failed without it. `writeStart` is
   announced before any slot data is written, and the reader validates against it.

5. **Ring buffer element type.** Sample data stays `float` as the contract requires. Accesses are
   ordered by the fences in RingWriter/RingReader. Formally this is a benign race on non-atomic
   floats. If a strict model is needed later, switch the buffer to `std::atomic<float>` with
   relaxed access; the layout is unchanged.

6. **Platform shared memory names.** `busName(uid)` returns `Local\ShaderForge_Bus_<uid>` on
   Windows and `/ShaderForge_Bus_<uid>` on POSIX. macOS limits POSIX shm names to 31
   characters, so uids should be 8 characters or fewer. `create()` fails on Windows if the object
   already exists. On POSIX it replaces a stale object left by a crashed owner.

7. **Parameter IDs.** The contract fixes `P_MasterGain=100` and the order of the scalar
   parameters. Their IDs are assigned consecutively: Fps=101, Vsync=102, ResMode=103, RecFps=104,
   RecCodec=105, RecQuality=106, PreviewQuality=107. Tracks, FX and custom IDs follow the table
   exactly.

8. **Pragma params.** The i-th `#pragma param` maps to `uCustom<i>`, and `#define <name>
   uCustom<i>` makes the name usable directly as a float. The author never declares them. Limit:
   8 per shader, which keeps the total well under the 32 `uCustom` channels. Reserved names are
   the contract uniforms, `uv`, `fragColor`, GLSL keywords, `SF_*`, and `iTrackN`/`uCustomN`
   names. Malformed or reserved pragmas are skipped with a warning and never abort the shader.

9. **Preamble.** `#version 150`, one `SF_PLATFORM_*` macro, the contract uniforms, and
   `uniform sampler2D uSrc`. The sampler is declared for FX passes; generator shaders ignore it.
   Any `#version` or `#pragma param` line in the author's code is blanked, not deleted, so line
   numbers stay aligned. Diagnostics are shifted back by the preamble line count.

10. **Driver logs.** Three formats are parsed: Mesa/AMD, NVIDIA, Apple/ANGLE. Unknown lines are
    ignored. If a failing compile yields no parseable line, the whole log is reported as a
    line-1 error so a failure is never silent.

11. **Track envelope scale.** `iTrackN = level * 4`, so the output is 0..4 as the contract
    requires. The gate threshold is 0.01 peak. Retriggering during release restarts the attack
    from the current level.

12. **Preset JSON.** `adsr` may hold one entry (applied to all eight tracks) or eight entries.
    The contract example shows one entry, so both are accepted. Preset JSON is validated by
    `scripts/validate_presets.py`. A C++ JSON loader is deferred to the JUCE layer, which has a
    JSON parser; adding a hand-written one now would be extra untested code.

13. **Preset validation.** Each preset is checked for the contract tokens, bounded loops, pragma
    validity, JSON sidecar consistency, and GLSL 150 compilation with `glslangValidator`. The
    compile check runs on Linux in CI. Windows and macOS drivers are not exercised here. The
    "under 2 ms at 1080p" requirement cannot be measured without a GPU; it is unverified.

14. **Tooling.** `shaderforge_inject` (under `src/tools/`, not in the §3 layout) prints the final
    injected source for a shader. The validator uses it, so CI checks the same code as the app.

15. **Warnings.** GCC/Clang: `-Wall -Wextra -Werror`. MSVC: `/W4 /permissive- /WX`. Applied to
    our targets through the `sf_warnings` interface target, not globally, so fetched Catch2 is
    not affected. MSVC has not been run locally; CI is the first check.

16. **Catch2.** `find_package(Catch2 3)` is preferred. Otherwise v3.7.1 is fetched. Both paths
    were built and tested in this sandbox (system Catch2 3.4.0 and fetched 3.7.1).

17. **Snapshot.** A seqlock whose payload is stored as relaxed atomic words, so concurrent reads
    are race-free. It is for UI-to-render handoff and must never be used on the audio thread.

18. **Not done in this revision (tracked for later sessions):** `Json.h`, `FboPair`, `GlScene`,
    `FxChain`, `TextureShare*`, `ExtDisplay*`, `Paths*`, `Encoders`, `Recorder`, `ui/*`, `ai/*`,
    `license/*`, `mcp_launcher`, `selftest` (GL), `fetch_ffmpeg.*`, `package_*`, Inno Setup and
    pkgbuild scripts, `release.yml`, `cmake/*.cmake` for JUCE/FFmpeg/Spout/Syphon/signing,
    `pluginval`/`auval` steps, and the release tag `v2.0.0`.
