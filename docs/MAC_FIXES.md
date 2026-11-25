# Mac fixes log

Fixes found by playing Dashdance on a MacBook Pro (M4 Pro, macOS 26) with a Mayflash adapter in Wii U mode.
Each fix is its own commit on `christopher/mac-fixes`, so any subset can go upstream as a PR.

How the loop works: play, then run `/fix-logs`. It reads the session logs and crash reports nobody has
reviewed, logs what broke here, fixes it one commit at a time, rebuilds, and adds a bullet to `docs/PR.md`.
A fix counts as confirmed once it has been played through again.

## Open issues

| # | Found | What happens | Evidence | Notes |
|---|---|---|---|---|
| 10 | 2026-11-24 | melee-unlocked engine: a vanilla recording made from the menus and its playback differ in a few post-frame records (vs4_busy recorded by `--play-script`: 16 of 29296). | `replay: post-frame record of frame -118 port 0 differs from the file at byte 16 (27, file 28)`; frames 1706.. port 2 byte 43 (C3, file 8C) | Frame -118 (cause found, not fixed): the y of the first EntryStart frame. EntryStart_Phys computes the same y (41C9E127) in both runs; the air collision after it (`ft_80083E64` -> `mpColl_8004730C`) nudges it by one ulp only in the recording, because the fighter's CollData differs before that call: cur_pos.y 15.2 / last_pos.y 35.2 in the recording against 25.2 / 25.2 in playback, floor_skip 6 against 0xB. Those come from `Fighter_Spawn`'s spawn ground check (`ft_80082A68`: cur -10, last +10, `mpColl_800471F8` searches the stage's floor lines): in the menus-started match it finds no floor and leaves the offsets, in the playback's direct boot (debug VS, scene 0x0E) it finds one. So the stage's collision lines are not in the same state when the fighters spawn (Onett's awnings move): the two boots set up the stage differently before the first spawn. Clearing the fighter block in playback too (as recording does) changes nothing. Byte 43 is misc AS (hitstun float) of a fighter whose position and state agree; not traced yet. Diagnosed with temporary prints in the fetched tree (removed). |

## Fixed

| # | What was wrong | Fix | Commit |
|---|---|---|---|
| 9 | Direct games dropped 30.7 to 32.0 s after connecting on a healthy link (six times on 2026-09-19 against The__Phenom2009: `session-20260919-114240.log`, `-122015`, `-122449`, `-122828`; report `winner -3, end 7`). Both players dial each other. When a player's real port differs from the one matchmaking advertised (their router remaps it), the attempt to the advertised port never connects, sits alone under its own address key and expires at ENet's 30 s cap; `all peers gone` was judged per address, so the expiry marked the player inactive and the netplay loop closed the healthy connection. Reproduced with two local instances, one dialling a dead port for the other: the victim's log matches the real sessions line for line. | A player is gone only when no connected peer of theirs is left under any address. Same test now plays through the 30 s mark; the normal two-connection case still closes its duplicate and stays in sync (2100 checksums, 0 mismatched). In the real sessions this Mac was the side that got closed, so the opponent's client needs the same fix: the logic is inherited from Slippi Dolphin. `slippi: peer ...` and `peer health` lines, `MELEE_NET_DROP_CONNECT=1` and `--local-peer` with a wrong port are the tools. | see git log |
| 1 | GameCube adapter never read on this Mac: `ReadPipeTO` on the interrupt pipe returns `kIOReturnBadArgument`, so the reader thread gave up after 20 failures. | Fall back to blocking `ReadPipe` when timed reads are rejected; abort the pipe on close so the blocking read wakes. | see git log |
| 2 | No way to quit from full screen except the Dock. | Cmd+Q requests exit from the SDL event loop. | see git log |
| 3 | HUD showed any adapter rate below 900 Hz as "125 Hz" (the Mayflash overclocks to ~540 Hz). | Show the measured rate. | see git log |
| 6 | No way to see ping during a match (it was only written to the log every 10 s). | HUD shows "ping N ms" while connected to an online opponent. Not yet seen in a live match. | see git log |
| 5 | Launcher crashed on open (`+[NSTextField wrappingLabelWithString:]` assertion in `refreshGames`, 2026-09-16 10:46). `ns()` used `stringWithUTF8String`, which returns nil for invalid UTF-8, and replay names are Shift-JIS on console. | `ns()` falls back to Shift-JIS, then Latin-1. Launcher stayed up after the fix; the original crash was not reproduced on demand, so the cause is inferred from the stack. | see git log |
| 7 | Desync against Dolphin players whenever the UCF 0.84 shield drop fired (seen twice on 2026-09-16: `Game_20260916T121422` frame 3791, Marth spot-dodged instead of dropping through Stadium's platform; `Game_20260916T131538` frame 1058, a shield out of landing lag came a frame late, then the drop was missed). The UCF code returns to its caller's return address + 8 (`lwz r7,28(r1); addi r7,r7,8; mtlr r7; blr`) to skip the caller's `li r3,1`, so the spot-dodge check reports "no input" and the next check (shield or drop) runs. The recompiler turns every `blr` into a C++ `return`, so the caller always resumed right after the call: the check reported "handled" with no state change, and the fighter lost that frame's input. | `analyze.py` recognises returns to the return address + N and the emitter resumes the caller there (`if (c.lr == ret+N) goto`). Headless re-simulations of both replays now match Slippi Dolphin on every player-frame (7840 and 2372), and a Slippi Dolphin replay from 2025-10-12 still matches its recording (16916). The full decision is now unit-tested against the translated code: `tools/mac/shield_drop_test.sh` drives `ftCo_8009980C`/`ftCo_80099894`/`ftCo_8009A080` with the Game_20260916T121422 stick dive (-0.39, -0.60, -0.74, -0.96) and the disc thresholds (spot dodge y <= -0.7 within 4 frames, UCF skip y in (-0.8, -0.7] with the |x|,|y| >= ~0.63 diagonal, platform pass y <= -0.66 within 6 frames) and expects the resim outcomes (25 checks). Online desyncs now also name their first domino: a remote-input gap (newest remote input older than the simulated frame) is logged when it opens and again on the next checksum mismatch. Not yet confirmed in a live match. | see git log |
| 8 | Crash mid-match, `guest call depth exceeded` in `HSD_JObjSetupMatrixSub` (2026-09-16 09:36, 10:44, 14:23, 15:37; `session-20260916-140128.log`, `session-20260916-152215.log` and its `.ram`). The 15:37 RAM showed the JObj has no parent and the guest stack was 16 frames deep, so the game was not recursing: the host counter in `ppc::call` had drifted to 20000. It counted with ++/-- around the call, and guest `longjmp` (a C++ exception) skipped the decrement for every frame it unwound. Stage animation lookups (`grAnime_801C8318`: setjmp, `HSD_ForeachAnim`, a callback that longjmps) do that every frame a background animates; a Battlefield replay leaked 1 to 801 in 11019 frames, and the count carries across games. | `CallDepthScope` guard decrements on unwind. The same replay stays at depth 1 with an identical re-simulated recording; the dispatch test runs 25000 longjmps (old code faults). The frame log line shows `call depth N`. Not yet confirmed in play. | 2626ae4 |

| 4 | No session logs at all: the first `host::log` ran before `main_mac` set the log path, so the file opened relative to cwd (`/` from Finder), failed, and was never retried. Crashes left no trace. | Buffer lines until the path is set; one timestamped log per launch in `~/Library/Application Support/Dashdance/Logs/`, newest 50 kept, `latest.log` symlink. | see git log |

## Investigating a desync

`tools/mac/desync.py <replay.slp>` does the whole loop and writes `reports/desync/<replay>/report.md`:

1. Slippi Dolphin re-simulates the replay from its recorded inputs (`slippi_frames.py`, a patched copy of the
   Launcher's playback Dolphin, resync off). Every pre-frame and post-frame field is diffed against Dashdance's
   recording: first divergent frame, fields, and the inputs leading up to it. The online frame is replay frame + 124.
2. Dashdance re-simulates the same replay headless (`dashdance_resim.py`, the playback code set translated into
   `build/mac-playback*`). Reproducing its own recording means a deterministic bug in the translated game or HLE;
   matching Slippi means an online-only path (or a bug already fixed).
3. Guest RAM entering the divergent frame, from both (`slippi_ram.py` attaches lldb to Dolphin;
   `MELEE_RAM_DUMP_FRAMES` in the playback build), compared by `ramdiff.py`: fighters by decomp field name, bone
   animation, player blocks, pointers normalised. Identical state means the bug is in code run during that frame.
4. Slippi's frames around the divergence as PNGs.

For fix 7 the state entering the frame was identical and the inputs identical, which pointed at control flow in
that frame; reading the translated `ftCo_80099794` next to the UCF cave found the return-address trick.

## Setup notes that are not code

- Fixed: `install.sh` used to fail on a fresh Mac. Its `--depth 1` clone lacked the upstream commit `bootstrap_port.py` checks, and `setup.sh` cloned doldecomp/melee at HEAD instead of the pinned revision. `install.sh` now clones full history (and unshallows an older download), and `setup.sh` fetches the commit from `tools/port_source_pins.json` directly, repairing a checkout left at HEAD.
- 1000 Hz polling without a driver (the README claim) did not work here: `SetPipePolicy` is rejected, and with no driver Apple's HID driver owns the adapter exclusively. The legacy GCAdapterDriver.kext (Permissive Security, SIP off) gives ~540 Hz, which looks like the Mayflash hardware cap.
