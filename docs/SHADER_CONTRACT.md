# Shader contract

This is what an author may write, what the host guarantees, and what the factory presets are
checked against. The same rules appear in the AI system prompt once that layer exists.

## What the author writes

- A `main()` that sets `fragColor` (vec4) and clamps the output to 0..1.
- Coordinates: `uv` is in 0..1. Use `uv * 2.0 - 1.0` for centred coordinates.
- Loops: literal bounds of at most 64, written as `for (int i = <int>; i < <literal>; ++i)`.
- Tunables: `#pragma param <name> <min> <max> <default>`, at most 8 per shader. The name is then
  a float you can use directly.
- Prefer `sin`, `cos`, `smoothstep`, `fract`, `mix`, `dot`.

## What the author must not write

- `#version`, `#include`, `precision` lines. The host injects them.
- Textures or any `texture*` call. Generator shaders have no inputs.
- To-do markers or not-implemented placeholders of any kind.
- A `#pragma param` whose name is reserved (see below) or duplicated.

## What the host injects (in this order)

```glsl
#version 150
#define SF_PLATFORM_<WINDOWS|MACOS|LINUX> 1
in vec2 uv;
out vec4 fragColor;
uniform vec2 uResolution;
uniform float uTime;
uniform float uBeatPhase;
uniform float iTrack0, ..., iTrack7;      // smoothed envelopes, 0..4
uniform float uCustom0, ..., uCustom31;   // parameter channels
uniform float uWet;
uniform sampler2D uSrc;                   // FX passes only
#define <param0> uCustom0                 // one define per #pragma param, in order
#define <param1> uCustom1
...
<author code, with #version and #pragma param lines blanked>
```

## Uniform meanings

| Name | Meaning |
|---|---|
| `uResolution` | output size in pixels |
| `uTime` | seconds since the shader was activated |
| `uBeatPhase` | 0..1 position within the host beat (0 when no tempo is available) |
| `iTrack0..7` | ADSR level of each Sender track times 4, so 0..4 |
| `uCustom0..31` | values of the `#pragma param` declarations, plus later custom parameters |
| `uWet` | FX wet amount for the current pass |
| `uSrc` | input image for FX passes |

## Reserved names

Pragma names may not be `uv`, `fragColor`, `uResolution`, `uTime`, `uBeatPhase`, `uWet`, `uSrc`,
`main`, GLSL keywords (`float`, `vec4`, `return`, ...), anything beginning with `SF_`, or
`iTrack<N>` / `uCustom<N>` for any N.

## Errors and rollback

- A pragma that fails to parse is skipped and reported as a warning. The shader still compiles.
- A compile error keeps the last good shader active. The error is reported at the author's line
  number.

## Factory presets

Each preset in `presets/factory/` has a `.frag` file and a `.json` sidecar with the same stem.
`scripts/validate_presets.py` checks every rule above and compiles each preset with
`glslangValidator -S frag` after injection. The render-time budget (under 2 ms at 1080p on an
RTX-class GPU) has not been measured in this revision.
