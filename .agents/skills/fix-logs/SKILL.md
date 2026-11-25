---
name: fix-logs
description: Review melee-unlocked-macOS engine logs and crash reports that nobody has looked at yet, turn what they show into issues, fix them one commit at a time, rebuild, and keep docs/MAC_FIXES.md and docs/PR.md current. Desyncs are diagnosed against real Melee with the replay playback check and Slippi Dolphin (desync.md). Use when Christopher says "/fix-logs", "check the logs", "look at my crashes", "I had a desync/disconnect/crash", "it desynced again", or "fix what broke".
---

# fix-logs

Christopher plays melee-unlocked-macOS and hits crashes, desyncs and disconnects. This skill is the loop that turns
those sessions into fixes in his repository.

## 1. Find what is unreviewed

```bash
tools/mac/unreviewed.sh
```

It lists the engine's logs (`~/Library/Application Support/Dashdance/melee-unlocked/engine.log` and `engine.prev.log`,
known by name and time) and macOS crash reports (`~/Library/Logs/DiagnosticReports/MeleeUnlockedEngine*.ips`,
`melee-unlocked-macOS*.ips`) not yet in `.reviewed-logs`, with the lines that matter: desyncs, disconnects, a start that
failed, a non-zero stop code, adapter read failures. It skips `engine.log` while the engine is running. The earlier
engine's `Logs/session-*.log` and `Dashdance*.ips` are listed too, for history only: that engine is gone, so mark them
reviewed. If it prints "Nothing unreviewed.", say so and stop.

## 2. Triage

For each file, open the full log around the flagged lines, not just the grep. Match a crash report to its
session log by time (the `.ips` timestamp falls inside the session). Classify:

- **Crash**: the log ends without `melee-unlocked engine: stopped with code`, and an `.ips` report matches its time.
  Follow [crash.md](crash.md).
- **Desync**: the game's own checksum comparison reported one (the `net check` line at the end counts them). Note the
  frame and which player, then follow [desync.md](desync.md): the replay's playback check and Slippi Dolphin's
  re-simulation find the first divergent frame. Never guess a desync cause from the log alone.
- **Disconnect**: who dropped. `got disconnect from peer` or `force-disconnecting player N after 7 s stall`
  means the other side went away; `online connection failed` or our own stall means it may be us.
  Christopher suspects some disconnects were his side, so say which it was and why.
- **Noise**: a network blip or a clean exit with nothing actionable.

Check `docs/MAC_FIXES.md` Open issues first. A repeat of a known issue gets its evidence added to that
row, not a new row. Group files that show the same failure.

## 3. Fix, one issue at a time

Work the issue with the most evidence first. Read the code before changing it. For a desync, the fix is verified only
when the replay plays back with 0 differing post-frame records and Slippi Dolphin's re-simulation agrees with it
(desync.md). Make the smallest change, following the repo's CLAUDE.md rules (melee-unlocked changed only through
`mu-engine/patches`, the exactness gate, no game data in the tree). If the root cause is not provable from the evidence, don't guess a fix: add the
logging or tooling that would prove it next time, commit that, and say so. New diagnostic tools go in
`tools/mac/` and get written up in desync.md or crash.md (or a new sibling note linked here), so the next run
can use them.

For each fix:

1. Edit.
2. `tools/mac/rebuild.sh` (`SKIP_MU_GAME=1` when the game image is unchanged). If the app is running it only builds,
   and `tools/mac/rebuild.sh --install-only` installs later. Run the exactness gate (CLAUDE.md) before committing.
3. Commit that fix alone. The message says what broke, the evidence, and why the change fixes it.
4. Move or add the row in `docs/MAC_FIXES.md` (Fixed, with the short commit hash), and add one bullet to
   `docs/PR.md` under the matching section (Fixes, Features, Logging, Debugging tools). Anything else
   you add to the repo, such as a tool or logging, gets a plain bullet too. Commit those docs changes.
5. `git push origin melee-unlocked-macos`.

A fix is "confirmed" only after Christopher has played through the case again. Until then say
"fixed, not yet confirmed in play".

## 4. Mark reviewed

```bash
tools/mac/unreviewed.sh --mark <every file you triaged>
```

Mark a file even when it was noise, so it never comes back. Don't mark a file whose issue you ran out of
time on; leave it for the next run and say which.

## 5. Report

Open with one line: how many sessions and crash reports were reviewed, and how many problems they held.

Then one short block per problem, in plain words (no function addresses unless they are the only name it has):

```
**<what he saw, e.g. "Crash in the middle of a match">** (<when>, <how many times>)
- What went wrong: <the cause, in one or two sentences a player can follow>
- Whose side: <for desyncs and disconnects: us, them, or unknown, and the log line that says so>
- Fix: <what changed and why that stops it>, or "Not fixed yet: <why>, and what now gets logged to catch it"
- Status: fixed and confirmed / fixed, not yet confirmed in play / logged only / noise
```

Files that were clean or noise get one line total, not a block each. End with what, if anything, he should
try in play to confirm a fix.
