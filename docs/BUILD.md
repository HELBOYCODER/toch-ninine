# Building

## Portable core (implemented)

Requirements: CMake 3.22 or newer and a C++17 compiler. Catch2 3 is found with `find_package`;
if it is missing, CMake fetches v3.7.1.

```bash
# Linux / macOS
scripts/build_macos.sh            # configure, build, fail on warnings
ctest --test-dir build --output-on-failure

# Windows (PowerShell)
scripts/build_windows.ps1
ctest --test-dir build -C Release --output-on-failure
```

Presets: after building `shaderforge_inject`, run

```bash
python3 scripts/validate_presets.py --inject build/shaderforge_inject --glslang glslangValidator
```

`glslangValidator` comes from the `glslang-tools` package on Debian/Ubuntu, or from the LunarG SDK
on Windows and macOS.

## Test list

| Test | Covers |
|---|---|
| test_ringbuffer | wrap, drop-oldest overrun, two-thread stress with ordering and accounting checks |
| test_diag | Mesa, NVIDIA and Apple log formats, CRLF, line shifting |
| test_pragma | valid and malformed `#pragma param`, reserved names, the 8-parameter cap |
| test_preprocessor | single `#version`, preamble contract, param macros, line mapping |
| test_state | round trip, forward-compatible prefix read, truncation at every cut, bad headers |
| test_bus | name form, magic validation, owner/client lifecycle, sample transfer |
| test_adsr | monotonic attack and release, sustain, retrigger |
| test_slot | rollback keeps the last good shader, debounce timing |
| test_params | ID ranges, duplicate and invalid registration, clamping |
| test_snapshot | seqlock invariants under a writer thread |
| test_presets | count, contract tokens, bounded loops, JSON sidecar, pragma validity |

## Manual matrix (not run; requires a DAW and a GPU)

The matrix from §10 of the build order needs hardware and a host application. None of it has been
run for this revision: 8 senders at 60 fps, shader swap during playback, rollback under playback,
30-minute 4K60 capture with A/V drift below 20 ms, HAP in Resolume, Spout in OBS, monitor unplug
recovery, 50 open/close cycles without a GL context leak, the 14-day offline license grace, and
60 fps on RTX-class hardware.

## Not yet present

The JUCE plugin targets, `pluginval` and `auval` steps, FFmpeg fetch and encoders, installers,
and signing. See `DECISIONS.md` item 18.
