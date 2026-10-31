# Architecture

## Status

Only the portable core exists in this revision. Everything marked *planned* in the diagram below
is not implemented yet and has no code in the repository.

## Signal and data flow

```mermaid
flowchart LR
    subgraph Sender["ForgeSender (planned: JUCE plugin)"]
        SIN[Audio in] --> SP[Peak per block]
        SP --> SW[SlotWriter]
    end

    subgraph Bus["Shared memory bus (implemented)"]
        SW -->|writePos release, writeStart announce| RING[(Slot ring x16)]
        RING -->|readPos release| SR[SlotReader]
    end

    subgraph Forge["ShaderForge (planned: JUCE plugin)"]
        SR --> ENV[Adsr x8 (implemented)]
        ENV -->|iTrackN = level x 4| UNI[Uniforms]
        UNI --> GL[GlScene (planned)]
        GL --> FX[FxChain, 16 passes (planned)]
        FX --> OUT[Preview / Recorder / Spout / Syphon (planned)]
        CODE[GLSL text] --> PRE[ShaderPreprocessor (implemented)]
        PRE --> SLOT[ShaderSlot with rollback (implemented)]
        SLOT --> GL
    end

    STATE[(SFST state chunk, implemented)] <--> Forge
```

## Modules implemented

| Module | Files | Role |
|---|---|---|
| shared/bus | `Bus.h`, `Bus.cpp`, `BusPosix.cpp` | Fixed-layout shared memory, magic check, sender count |
| shared/ring | `RingBuffer.h` | Lock-free SPSC ring, drop-oldest, overwrite-safe reads |
| shared/envelope | `Adsr.h`, `Adsr.cpp` | Per-track ADSR, output scaled to the 0..4 uniform range |
| shared/diagnostics | `Diag.h`, `Diag.cpp` | Parses Mesa, NVIDIA and Apple driver logs |
| shared/params | `ParamRegistry.h`, `.cpp` | Stable IDs, ranges, duplicate rejection |
| shared/log | `Log.h`, `Log.cpp` | Pluggable sink |
| core/preprocess | `ShaderPreprocessor.h`, `.cpp` | Preamble injection, `#pragma param` parser |
| core/slot | `ShaderSlot.h`, `.cpp` | Compile-with-rollback and debounce |
| core/state | `State.h`, `State.cpp` | Versioned `SFST` chunk |
| core/snapshot | `Snapshot.h` | Seqlock for UI-to-render handoff |
| tools | `shaderforge_inject.cpp` | Prints the injected source for a shader file |

## Threading rules

- Audio thread: the ring writer and the envelope step only. No allocation, no locks, no GL.
- Reader thread (render): pops from the ring, steps the envelopes, publishes uniforms through a
  `Snapshot`.
- Compile happens on the GL thread. `ShaderSlot::submit` is called from there only.
