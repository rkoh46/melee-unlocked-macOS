# Fixing a crash

The engine (`MeleeUnlockedEngine`) logs to `~/Library/Application Support/Dashdance/melee-unlocked/engine.log` (the session
before is `engine.prev.log`; the host's own lines are in `engine.log.host`). A clean end is the line
`melee-unlocked engine: stopped with code N after R retraces`. A crash ends the log early and leaves a macOS report in
`~/Library/Logs/DiagnosticReports/MeleeUnlockedEngine-*.ips`.

## 1. Read the report

- `tools/mac/unreviewed.sh` prints the crashing thread's frames. Frames in `MeleeUnlockedEngine` are the app and
  melee-unlocked's host services; addresses from `0x100080000000` up with no image name are the game image
  (`melee_game.so`, loaded by `mu-engine/runner/mu_elf.cpp` at `MU_HIGH_BASE` + the console address `0x82800000`, so
  `0x100082800000` is the image's start). Subtract the image's load address (the log's `game image: ... r-x` line) and look
  the offset up in `build/mu-engine/game/melee_game.so` with `nm -n` or `addr2line` from the Linux image
  (`docker run --rm --network none ... mu-arm-gcc15`).
- The decomp source the image is built from is `build/mu-engine/mu/sourceport/extern/melee/src`, with melee-unlocked's
  patches and ours (`mu-engine/patches`).

## 2. Reproduce it

- A crash in a match: the game's replay (`Replays/Game_<timestamp>.slp`) holds every frame it finalized. Watch it from the
  launcher, or run `MeleeUnlockedEngine --iso DISC --replay REPLAY --profile-dir SCRATCH --hidden`: a deterministic crash
  repeats there.
- A load or store through a bare console address (a pointer that was not widened to `MU_HIGH_BASE`): run the headless
  runner with `MU_FIXUP=1` (or `2`), which logs each such access with the call chain (docs/TECHNICAL.md, "Addresses").
- Online only: read the log for rollbacks before the crash (`net:` lines); `mu-engine/tools/net_rollback_test.sh` plays
  rollbacks on loopback.
