# melee-unlocked-macOS technical guide

The [README](../README.md) is the short version for players. This is what is underneath: how the app is put together, how
to build and test it, and the details of the engine on macOS. Contributors and coding agents should also read
[CLAUDE.md](../CLAUDE.md).

## How it fits together

The bundle `melee-unlocked-macOS.app` holds two programs:

- **`melee-unlocked-macOS`** (`port/app/main_mac.cpp`, `port/runtime/host/mu_launcher.mm`): melee-unlocked's launcher,
  rebuilt in AppKit from its Win32 paint code. It keeps `launcher.ini`, reads the Slippi sign-in for the Play page, lists
  the replay folder, runs the mod installer and the crash report, and on PLAY `execv`s the engine with its choices
  (`--iso`, `--profile-dir`, `--volume`, `--scale`, `--fullscreen`, `--no-vsync`, `--vanilla`, `--net-delay`, `--offline`,
  `--user-dir`). Watch starts the engine beside it with `--replay`.
- **`MeleeUnlockedEngine`** (`mu-engine/app`): melee-unlocked's host services run the game image (an AArch64 Linux ELF built
  from the decompilation's C, `mu-engine/runner` loads it) on the main thread, paced to the console's 60 Hz. Through the
  host's frontend hooks (`mu-engine/patches/0011`) the game gets this repository's controllers (keyboard, SDL gamepads, the
  GameCube adapter) and sound output, and its GX stream goes to this repository's decoder and Metal renderer
  (`port/runtime/gx`). melee-unlocked's settings panel (`mu-engine/app/ui`, drawn with Dear ImGui over the picture) and
  Melee-NX-NetPlay's frontend layer (`mu-engine/app/fe`: settings.ini, mods, Training, replays and their viewer, overlays,
  Fight a pro) run in the same process.

User data lives in `~/Library/Application Support/Dashdance`: `launcher.ini` (shared by both programs: each writes its own
keys and keeps the rest), `Slippi/user.json` (the app's own sign-in), `Replays/`, and the engine's folder `melee-unlocked/`
(`settings.ini`, `Card A`, `Mods`, `engine.log`, `launcher.log`).

### Settings from the earlier engine

Until this version the app also carried Dashdance's statically recompiled engine ("Static Recomp (Legacy)"). The launcher
reads what that engine left: `engine=1` becomes the Source Port; `iso`, `scale`, `vsync`, `fullscreen`, `volume`, `online`
and `online_delay` keep their meaning; `controller.*`, `keyboard`, `discord`, `discord_rank`, `anisotropy` and `hud` were
already read by the engine; `widescreen=1` is written once to the engine's `settings.ini` (its `widescreen` line, which the
launcher's Widescreen row now reads and writes); the memory card in `User/GC/CardA` is copied to `melee-unlocked/Card A`
while that folder has no saves (both are folders of Dolphin-layout `.gci` files). `sharpness`, `upscaler` (MetalFX),
`overlay` and `overlay_scale` (touch controls) have no counterpart and are left in the file unused. `setup.sh` deletes the
earlier engine's build inputs and outputs (`deps/`, `build/mac-playback`, an old `build/mac`).

## The engine on macOS (`mu-engine/`)

melee-unlocked 1.0.5 (commit `67868aa`) is fetched at build time (`mu-engine/fetch.sh`) and changed only by the patches in
`mu-engine/patches`; nothing from it or from the game is committed. `mu-engine/build.sh game` builds its game image as an
AArch64 Linux ELF with GCC 15.2 in a container with no network (image `mu-arm-gcc15`, from
`mu-engine/linux-gcc15.Dockerfile`).

**Addresses.** On Linux and the Switch the game sits at its console addresses: main memory at `0x80000000`, the image at
`0x82800000`, the locked cache at `0xE0000000`, because disc data keeps 32-bit pointer slots. macOS will not start an arm64
binary whose `__PAGEZERO` is smaller than 4 GB, so nothing can live below 4 GB. Here the three ranges sit at
`0x100000000000` plus the console address (`MU_HIGH_BASE`, a multiple of 4 GB, so narrowing a pointer to 32 bits still gives
the console address). Patches `0010` and `decomp-0010` make every place that turns a 32-bit console address into a pointer
widen it (`MU_WIDEN`, `MU_WIDEN64` in `mu_disc.h`): the sites were found with a cast audit build and with runtime fault
reports (`MU_FIXUP=1`/`2` in the runner logs any load or store through a bare console address, with the call chain).

**ABI.** `mu-engine/runner/mu_elf.cpp` loads the ELF with macOS back ends in `mu-engine/runner/macos`: `-ffixed-x18` keeps
the image off the platform register; the printf family goes through AAPCS64 variadic entry points (`mu_linux_varargs.S`),
since Apple passes variadic arguments on the stack; the two host functions with 8-bit arguments get adapters
(`abi_macos.cpp`); pages are 16 KiB.

**Netplay.** The engine's netplay object is Melee-NX-NetPlay's port of Slippi 3.7.0 (`mu-engine/net`: the WebSocket
matchmaking queue with STUN, 3.7.0's pads, player-drop reports and input suggestions, Slippi's HTTP services over libcurl),
assembled by `mu-engine/net_sources.sh` with its patches and the Mac's (`mu-engine/net/patches/mac-*`: tests point the queue
and the HTTP services at loopback stand-ins at run time, `MELEE_MM_LOOPBACK_PORT` and `MELEE_SERVICES_LOOPBACK_PORT`;
anything but 127.0.0.1 is ignored). `mu-engine/build.sh net` links it, with the app's Discord presence
(`port/runtime/hle/discord_rpc.cpp`) and sign-in (`slippi_login.cpp`), into one object where only `netplay_bridge::*` is
global.

**Renderer and input.** The GX decoder is a build-time copy of `port/runtime/gx/gx_core.cpp` that reaches 64 MiB of console
addresses and reads the scene from the host. The Metal renderer runs on its own thread (the simulation hands each frame to a
queue and never waits for the GPU or the display), compiles new pipelines in the background and remembers them in the
shader cache for the next start. The GameCube adapter (WUP-028) is opened through IOKit and the USB host is asked for a
1 ms polling interval with `SetPipePolicy`: no driver, no security change (`MELEE_ADAPTER_INTERVAL_MS=2` asks for 500 Hz).
Discord presence talks to the Discord desktop app over its local socket and does nothing when Discord is not running.

## Build from source

`./setup.sh` does all of this. Requirements: an Apple silicon Mac, the Command Line Tools, Homebrew (`cmake ninja python`)
and Docker (OrbStack or Docker Desktop) running.

```bash
git clone -b melee-unlocked-macos https://github.com/rkoh46/melee-unlocked-macOS.git
cd melee-unlocked-macOS
bash mu-engine/fetch.sh             # melee-unlocked at its pinned commit, its patches, the Metal ImGui back end, audio decoders
bash mu-engine/build.sh game        # the game image, in Docker
cmake -S . -B build/mac -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/mac --target melee_launcher_mac melee_unlocked_mac
tools/package_macos_app.sh build/mac dist
open dist/melee-unlocked-macOS.app
```

After an edit, `tools/mac/rebuild.sh` rebuilds, packages and installs to `/Applications` (`SKIP_MU_GAME=1` when the game
image is unchanged).

## Testing

All tests keep their windows hidden and never contact Slippi's servers.

- **Exactness.** `mu-engine/build.sh runner` builds `build/mu-engine/out/mu-runner`; run it with `--iso PATH --script NAME`
  for each of `parity_vs_onett vs4_busy parity_vs_items_off parity_lcancel native_vs_ko`. All five print `digest vs x86-64
  AVX2 reference: IDENTICAL for all 9000 rows compared`. The app's engine with picture and sound gives the same digest:
  `MeleeUnlockedEngine --iso PATH --profile-dir EMPTY_DIR --script build/mu-engine/gen/scripts/parity_vs_onett.txt --digest
  out.csv --frames 9000 --hidden`, then compare with `gunzip -c mu-engine/reference/parity_vs_onett.csv.gz`
  (`mu-engine/tools/compare_digests.py`).
- **Online.** `mu-engine/tools/online_loopback_test.sh ISO` (two runners), `app_online_test.sh ISO` (two engines with
  picture and sound), `net_rollback_test.sh`, `app_online_menus_test.sh`: loopback only, under a sandbox profile that refuses
  every outgoing connection except to 127.0.0.1, against `tools/mac/mock_matchmaking_ws.py` and `tools/mac/mock_slippi_http.py`.
  The bar is `net check: EQUAL` and 0 desync messages.
- **Replays.** Playing a replay back compares every post-frame record with the file; the log's last replay line must say
  `0 differ`.
- **Launcher.** `MELEE_LAUNCHER_SHOT=<file.png>` draws the launcher off every screen into a file; with `MELEE_LAUNCHER_PLAY=1`
  or `MELEE_PLAY_REPLAY=<n>` it presses PLAY or Watch, and `MELEE_MU_ENGINE` points it at a wrapper that records the
  arguments and adds `--hidden`. Run it with `HOME` set to a scratch folder and `MELEE_UPDATE_CHECK=0`.
  `MELEE_SETTINGS_SHOT=<file.png>` with `MELEE_LAUNCHER_SETTINGS=<0..7>` draws the Settings window's category off every
  screen instead (it prints its window number for `screencapture -l`); `MELEE_NO_ALERTS=1` sends alerts to the log.
- **Settings.** `mu-engine/tools/settings_keys_test.sh`: every key the Settings window writes is read by the engine or the
  launcher (no dead rows). `docs/SETTINGS_PARITY.md` lists every melee-unlocked setting and its Mac status.
- **Mods.** `mu-engine/tools/mod_watch_test.sh` (offline): a download-page-only mod found in ~/Downloads or chosen with
  Choose File is installed only after its SHA-256 check.
- Feature tests: `mu-engine/tools/app_*_test.sh`, `mod_install_test.sh`, `music_decode_test.sh`, `crash_report_test.sh`,
  `rvz_check.sh`.

## Releases

There are no public downloads, and there must not be: the built app contains the compiled game. `tools/release.sh` builds
`dist/melee-unlocked-macOS-<version>.dmg`, checksums and release notes (from the commits since the last tag, in player
terms) on your Mac; `--publish` creates a **draft** GitHub release with them and refuses unless the repository is private.

## History

The app started as Dashdance: Melee's PowerPC code statically recompiled to C++ (`port/recomp`, a generated translation
checked against a pinned input manifest, HLE of the GameCube SDK, Slippi 3.6.4 netplay, a second "playback" executable for
replays), with iPhone, iPad and Vision Pro builds. melee-unlocked's engine replaced it on the Mac; the recompiler, the
translated-game runtime, the playback build and the iOS and visionOS targets were removed. Their code is in the git history
before the commit that removed them, and their fixes are in [MAC_FIXES.md](MAC_FIXES.md).
