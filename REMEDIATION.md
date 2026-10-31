# Remediation record

Source: `AUDIT.md` at commit `00b1db2`. Only S0, S1 and S2 findings are in scope (§8).
Status values: **fixed** (verified), **partial** (code added, the finding still fails the audit),
**open** (not changed, with the reason).

| Finding | Sev | Status | Commit | Check output |
|---|---|---|---|---|
| F-001 | S1 | open: needs a spec decision | — | see below |
| F-002 | S2 | open: no GL renderer | — | the GL pipeline needs GL 3.2 core, which cannot run in this sandbox |
| F-003 | S2 | open: no ForgeSender | — | depends on the JUCE target (F-019) |
| F-004 | S2 | partial | 7b4a200 | `ctest -R test_envelopes` → 1/1 passed |
| F-005 | S2 | partial | 3897e89 | `ctest -R slot` → 1/1 passed; debounce constant 350 asserted |
| F-006 | S2 | open: no AI layer | — | not started |
| F-007 | S2 | open: no repair loop | — | not started |
| F-008 | S2 | open: no FX chain | — | needs GL |
| F-009 | S2 | partial | 7b05514 | `ctest -R param` → 2/2 + binding tests pass |
| F-010 | S2 | open: no encoders | — | needs FFmpeg (F-023) and the recorder |
| F-011 | S2 | open: no WAV sidecar | — | not started |
| F-012 | S2 | open: no Spout/Syphon | — | needs the platform layer (F-020) |
| F-013 | S2 | open: no external display | — | needs the platform layer (F-020) |
| F-014 | S2 | open: no preset manager | — | needs UI and the JSON loader |
| F-015 | S2 | open: no automation/MIDI/LFO | — | needs the plugin host (F-019) |
| F-016 | S2 | open: state not wired | — | needs the plugin host (F-019) |
| F-017 | S2 | open: no license | — | needs an RSA implementation and a decision on library |
| F-018 | S2 | open: no MCP server | — | not started |
| F-019 | S2 | open: no JUCE target | — | JUCE 8.0.4 fetch and plugin targets are not added |
| F-020 | S2 | open: no platform layer | — | not started |
| F-021 | S2 | fixed (verified) | 7182a95 | CI run 41203785514 on a97096a: `core (macos-14)` Configure, Build, Test all success |
| F-022 | S2 | open | — | no hardened runtime, no notarization path |
| F-023 | S2 | open | — | FFmpeg pin needs a verified tarball hash |
| F-024 | S2 | open | — | pluginval needs a plugin |
| F-025 | S2 | open | — | auval needs a plugin |
| F-026 | S2 | open | — | GL selftest needs GL |
| F-027 | S2 | open | — | tag `v2.0.0` only after release workflow exists |
| F-028 | S2 | open | — | no installers |
| F-029 | S3 | out of scope | — | S3 is not in §8 |
| F-030 | S3 | out of scope | — | S3 is not in §8 |
| F-031 | S3 | out of scope | — | S3 is not in §8 |

## F-001 (S1): why it is not changed

`Slot` has an extra member, `writeStart`, after `name`. Without it, the drop-oldest reader can
return a sample that the writer is overwriting. The two-thread stress test showed that failure
before the member was added.

Removing the member within the §4.1 layout would mean dropping the overwrite check, which
weakens the overrun guarantee in §4.1. Keeping the check requires amending §4.1. That is a spec
decision, so it is not made here. Options for the decision:

1. Amend §4.1 to add `writeStart` (keeps every existing offset).
2. Keep §4.1 unchanged and accept a reader that can briefly return stale samples under overrun.

## Not yet verified

- F-021 is verified by CI run 41203785514 (macOS universal build passed).
- The final §8.5 step (re-run all of §4.1) and the `chore: audit remediation complete` commit are
  **not** done, because S1 and most S2 findings are still open.
