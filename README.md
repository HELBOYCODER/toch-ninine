# ShaderForge

Realtime GLSL fragment shaders driven by per-track audio envelopes. The product is a VST3 (and AU
on macOS) pair: ShaderForge, the renderer, and ForgeSender, a per-track signal sender.

> **Status: work in progress.** This revision contains the portable core, its tests, the factory
> presets, and CI for both. The plugins themselves, the GL renderer, the encoders, the UI, the
> AI/MCP layer, licensing, installers and the v2.0.0 release are **not implemented yet**. See
> [DECISIONS.md](DECISIONS.md) for the exact scope.

## What works today

| Area | State |
|---|---|
| Shared-memory audio bus (8-16 slots, lock-free, drop-oldest) | implemented, tested |
| Per-track ADSR driving `iTrackN` (0..4) | implemented, tested |
| `#pragma param` parsing and preamble injection (`#version 150`, uniform contract) | implemented, tested |
| GLSL driver-log parsing (Mesa, NVIDIA, Apple) with line mapping | implemented, tested |
| Compile-with-rollback and 350 ms debounce logic | implemented, tested (no GL yet) |
| Versioned state chunk (`SFST`) with forward-compatible reads | implemented, tested |
| 12 factory presets, validated with glslangValidator | implemented, CI on Linux |
| Windows (MSVC) and macOS (clang) core build and tests | CI configured, first run pending |
| JUCE plugins, GL rendering, texture sharing, external display | not implemented |
| FX chain, recorder/encoders, HAP/H.264/H.265/NVENC | not implemented |
| Vibe Code tab, MCP server, auto-repair | not implemented |
| License with 14-day grace | not implemented |
| Installers, signing, notarization, release v2.0.0 | not implemented |

## Build and test

```bash
scripts/build_macos.sh                 # Linux or macOS
ctest --test-dir build --output-on-failure
```

On Windows use `scripts/build_windows.ps1`. Full instructions, including the preset validator and
the manual test matrix, are in [docs/BUILD.md](docs/BUILD.md).

## Documentation

- [Architecture](docs/ARCHITECTURE.md): signal flow and module map
- [Shader contract](docs/SHADER_CONTRACT.md): what authors write and what the host injects
- [Build](docs/BUILD.md): building, testing and the manual matrix
- [MCP](docs/MCP.md): planned interface (not implemented)
- [Signing](docs/SIGNING.md): planned signing design (not implemented)
- [Decisions](DECISIONS.md): every choice not fixed by the spec, and every deviation

## Repository

Development happens on the branch `claude/shaderforge-vst3-build-gak8ug` of
`HELBOYCODER/toch-ninine`. A standalone `shaderforge` repository has not been created.

## License

MIT. See [LICENSE](LICENSE).
