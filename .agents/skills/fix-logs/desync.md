# Fixing a desync

The game's own checksum comparison with the other side found a difference: the engine and the opponent's Slippi Dolphin
simulated the same inputs differently. Real Melee is the reference.

## 0. Find the replay

The engine records every match to `~/Library/Application Support/Dashdance/Replays/Game_<timestamp>.slp`; the timestamp is
when the game started. The log's `net check` lines list frame hashes, and the end of the log counts the desync messages.

## 1. Does our simulation repeat itself?

Play the replay back: the launcher's Watch, or
`MeleeUnlockedEngine --iso DISC --replay REPLAY --profile-dir SCRATCH --hidden`. At the end the log says
`replay: playback finished at frame F: N post-frame records compared with the file, D differ` (the first differences are
logged with the frame, port and byte).

- **D = 0**: our game repeats its own recording, so either Slippi Dolphin simulates the same inputs differently (go to 2)
  or the difference came from live play (remote inputs, rollback, timing: read the `net:` lines around the frame and run
  `mu-engine/tools/net_rollback_test.sh`).
- **D > 0**: the playback differs from the live game. Look at what only the live game does first (rollbacks, input
  suggestions).

## 2. What does real Melee do?

```bash
tools/mac/slippi_frames.py REPLAY START END   # Slippi Dolphin's frames as PNGs, and resim.slp, its re-simulation
```

Compare `resim.slp` with our recording using `tools/mac/slp.py` (`diff_records`, `state_name`): the first frame where the
post-frame records differ, and the player. Then read the decomp for the action states involved
(`build/mu-engine/mu/sourceport/extern/melee/src/melee/ft/`) and melee-unlocked's changes to them.

## Things that bite

- **Slippi playback's "resync" hides desyncs**: it snaps positions back to the recording. `slippi_frames.py` turns it off;
  the engine's playback only corrects the first frame's spawn when the recording says the fighter is elsewhere
  (`mu-engine/patches/0014`).
- **Frame dumping is disabled in the Mac playback Dolphin** (built without libav). `slippi_frames.py` patches a copy in
  `~/Library/Caches/Dashdance/slippi-frames/` (OpenGL backend, an existing `Dump/Frames` folder); the Launcher's own Dolphin
  and user folder are never touched.
- **"Waiting for game" is what Slippi playback shows after a replay ends**, not a hang.
- **Every change to the game image must keep the exactness gate** (CLAUDE.md): the five reference scripts identical, the
  online loopback tests equal.
