# melee-unlocked-macOS — guide for coding agents and new contributors

melee-unlocked-macOS is a native Apple-silicon Mac app for Super Smash Bros. Melee with Slippi Online. Its engine is
melee-unlocked's (Hero88go/melee-unlocked) "Source Port": the Melee decomp's C compiled natively, fetched and built at
install time, never committed. The app grew out of Dashdance, a client that statically recompiled the game's PowerPC code
to C++; that engine was removed (its history is in git and in `docs/MAC_FIXES.md`). What remains of Dashdance is the
app's renderer and host code in `port/`, which melee-unlocked's engine compiles in. Read this file first; it tells you how
the tree is laid out, how to build, and the rules that are not obvious from the code.

## Repository and workflow (rkoh46/melee-unlocked-macOS, branch `melee-unlocked-macos`)

rkoh's repository: it was the playtest fork rkoh46/dashdance, renamed (the old URL redirects). Commits are authored
as `rkoh` (git config); never as the Mac login name.

- Remote: `origin` is rkoh46/melee-unlocked-macOS. Work on `melee-unlocked-macos` (the default branch; the README's
  install line fetches it). One change per commit; push after each. `dashdance2` is the old Dashdance-only branch:
  don't push new work there. Upstream Dashdance (TheAndersMadsen/dashdance) is never pushed to.
- The installed app is `/Applications/melee-unlocked-macOS.app`; its user data stays in
  `~/Library/Application Support/Dashdance` (launcher.ini, Slippi sign-in, replays; the engine's own folder is
  `melee-unlocked/` there: settings.ini, `Card A`, engine.log, Mods).
- The interface is melee-unlocked's: the launcher (`port/runtime/host/mu_launcher.mm`, rebuilt from MU's Win32
  `port/app/launcher.cpp` paint code) and MU's in-game panels. MU's artwork and icon are converted from the fetched tree at
  packaging time (`tools/mu_launcher_art.py`) and never committed. YouGame is left out.
- The sister project is rkoh46/Melee-NX-NetPlay (the same engine on the Nintendo Switch,
  `experiments/native-engine/`). Code is shared by copying, with notices kept: the runner, the netplay bridge, the
  feature modules, the mod installer (`mu-engine/modinstall`). Its `FEATURES.md` is the full melee-unlocked feature
  list; `docs/FEATURES.md` here holds the Mac status per row.
- `/fix-logs` (`.agents/skills/fix-logs/`) reviews unreviewed engine logs and crash reports and fixes what they show.
- `docs/MAC_FIXES.md` is the issue and fix log. `docs/PR.md` is the running change list: everything added to the
  repo (fix, feature, logging, tool, skill) adds one plain bullet there in the same commit. Keep it short.
- Capture evidence by hand with `tools/mac/report.sh "what happened"`; list unreviewed logs with `tools/mac/unreviewed.sh`.
- The disc path is in `.disc-path` (written by install.sh); nothing reads the disc at build time.

## Layout

| Path | What lives there |
|---|---|
| `mu-engine/` | melee-unlocked's engine on the Mac: `fetch.sh` and `build.sh`, the patches against MU and its decomp, the runner and macOS back ends (`runner/`), the app's engine executable (`app/`: `mu_play_main.cpp`, the frontend layer `fe/`, MU's panel `ui/`), the netplay (`net/`), the mod installer (`modinstall/`), reference digests and test tools. |
| `port/app/main_mac.cpp` | The bundle's main program: shows the launcher, keeps launcher.ini, carries the earlier engine's settings over, then `execv`s `MeleeUnlockedEngine` with the launcher's choices. |
| `port/app/macos/Info.plist` | The bundle's Info.plist template. |
| `port/runtime/host/` | The launcher (`mu_launcher.mm`, `mac_launcher.h`, its Replay Viewer model `dashboard.*`) and the host code the engine compiles in: the SDL3 window, keyboard and gamepads (`window_sdl.cpp`), controller mapping, deadzones and the keyboard layout (`input_config.*`), the GameCube adapter over IOKit with a 1 ms pipe policy (`gc_adapter_iokit.cpp`), controller report rates (`controller_rate.mm`), sound output (`audio_sdl.cpp`), input scripts (`input_script.*`), Slippi's GameFiles diffs (`vcdiff.*`). `host.h`, `game_menu.h`, `overlay.h` and `window.h` declare what the engine's `host_shim.cpp`, `fe/` and `ui/` implement. |
| `port/runtime/gx/` | The GX decoder (`gx_core.cpp`, copied with changes by `mu-engine/app/CMakeLists.txt`), textures, Dolphin-derived shader generation and the Metal renderer (`gx_metal.mm`, `gx_msl.cpp`), sub-frame presentation. |
| `port/runtime/hle/` | Discord Rich Presence over the local IPC socket (`discord_rpc.*`), the Slippi sign-in file and tables (`slippi_login.*`, over `slippi_http_apple.mm`), replay parsing for the Replay Viewer (`slippi_history.*`). |
| `port/runtime/ppc/` | `ppc.h` (the console's RAM range and the CPU state record `host.h` and the renderer's observer name) and `render_observer.h`. |
| `port/slippi_sys/GameFiles` | Slippi's menu files and diffs (from Slippi Ishiiruka), served to the game by `mu-engine/app/slippi_menus.cpp`. |
| `port/third_party/` | ENet and nlohmann/json. SDL 3.4.10 is fetched by `port/CMakeLists.txt`. |
| `native/` | Asset and animation headers the renderer includes. |
| `tools/` | `package_macos_app.sh`, `mu_launcher_art.py`, `release.sh`, `release_notes.py`; `tools/mac/`: `rebuild.sh`, the loopback stand-ins for Slippi's services, replay tools (`slp.py`, `slp_live_writer.py`, `slippi_frames.py`), `report.sh`, `unreviewed.sh`, `gcadapter_probe.cpp`. |
| `docs/` | `TECHNICAL.md` (how it works, building, testing), `FEATURES.md`, `MAC_FIXES.md`, `PR.md`. |

## The melee-unlocked engine (`mu-engine/`)

- `mu-engine/fetch.sh` clones melee-unlocked at its pinned commit into `build/mu-engine/mu` and applies
  `mu-engine/patches/` (`0*.patch` against MU, `decomp-*.patch` against its decomp). Change MU only through patches.
- `mu-engine/build.sh game` builds MU's game image (an AArch64 Linux ELF) with GCC 15.2 in Docker (OrbStack): every
  container gets `--rm`, a `mumac-*` name, `--network none` and a time limit. `build.sh runner` builds the headless
  arm64 runner `build/mu-engine/out/mu-runner`; the top-level CMake builds the app's `MeleeUnlockedEngine`
  (`mu-engine/app`, target `melee_unlocked_mac`) and the launcher (`port/`, target `melee_launcher_mac`).
- macOS cannot map below 4 GB, so the console's memory ranges sit at `0x100000000000` + the console address; the
  patches widen every 32-bit address the game turns into a pointer (`MU_WIDEN`). The game is built `-ffixed-x18`.
- **Exactness gate:** the five reference scripts (parity_vs_onett, vs4_busy, parity_vs_items_off, parity_lcancel,
  native_vs_ko; digests in `mu-engine/reference/`) must print "IDENTICAL for all 9000 rows compared" with `mu-runner`
  before any change lands. Online: `mu-engine/tools/online_loopback_test.sh`, `net_rollback_test.sh` and
  `app_online_test.sh` (net check equal, 0 desyncs). Replays: 0 differing post-frame records on playback.
- Online tests run on loopback only, under a sandbox profile that refuses outside connections, against the
  stand-ins for Slippi 3.7.0's WebSocket matchmaking queue (`tools/mac/mock_matchmaking_ws.py`, `MELEE_MM_LOOPBACK_PORT`) and its
  HTTP services (`tools/mac/mock_slippi_http.py`, `MELEE_SERVICES_LOOPBACK_PORT`). Never contact Slippi's servers from a test;
  never Ranked.
- The engine's netplay is Melee-NX-NetPlay's Slippi 3.7.0 netplay: `mu-engine/net` (a copy of its port/runtime/hle files and
  ENet changes) with its patches and the Mac's (`mu-engine/net/patches`), assembled by `mu-engine/net_sources.sh`. To follow
  the Switch, copy its files and patches again.
- Never run melee-unlocked's release binaries, launcher or updater, and never a mod's own programs: build MU from
  source only. Test windows stay hidden or offscreen (the user plays on this Mac).

## One-command setup

Players use `install.sh` (the one line in the README): it checks for Docker, clones into ~/Dashdance (`DASHDANCE_DIR`), asks
for the disc, runs `setup.sh`, copies `melee-unlocked-macOS.app` to Applications (replacing DashDance2) and opens it.
Developers call `setup.sh` directly:

```bash
./setup.sh [/path/to/melee.iso]   # dist/melee-unlocked-macOS.app; the disc is only opened in the launcher
```

It installs Homebrew packages (cmake ninja python), requires Docker running, runs `mu-engine/fetch.sh` and
`mu-engine/build.sh game`, builds `melee_launcher_mac` and `melee_unlocked_mac` into `build/mac`, packages and opens the app.
A checkout that built the earlier engine has its leftovers (`deps/`, `build/mac-playback`, the old `build/mac`) removed.
There is no store or public-release distribution and there must not be: the built app contains the compiled game.
`tools/release.sh` builds a DMG and publishes drafts from a private repository only.

## Build rules that bite

- **Rebuild with `tools/mac/rebuild.sh`** (game image, engine, launcher, package, install to /Applications);
  `SKIP_MU_GAME=1` skips the Docker game build when only app code changed.
- **Toolchain.** The app builds with the Command Line Tools (`unset DEVELOPER_DIR`).
- **The engine compiles `port/` files directly** (`mu-engine/app/CMakeLists.txt`, `${DD}`), and `mu-engine/build.sh net`
  compiles `port/runtime/hle/discord_rpc.cpp`, `slippi_login.cpp` and `slippi_http_apple.mm` into the netplay object. A
  file there that looks unused may be used by one of those two.
- **Never build while measuring performance**: it distorts the per-frame numbers.

## Running and diagnosing

- `MeleeUnlockedEngine --help` lists the engine's flags (`--iso`, `--profile-dir`, `--hidden`, `--frames`, `--script`,
  `--replay`, `--digest`, `--offline`, `--online`, `--net-delay`). The launcher (`melee-unlocked-macOS --help`) takes
  `--iso` (start the game at once) and `--choose-disc` (show the launcher with that disc).
- The engine logs to `melee-unlocked/engine.log` (the session before: `engine.prev.log`), the launcher to
  `melee-unlocked/launcher.log`.
- Launcher test aids: `MELEE_LAUNCHER_SHOT=<file.png>` (window off every screen, drawn to the file after two seconds, then
  closed), `MELEE_LAUNCHER_PAGE=0..3`, `MELEE_LAUNCHER_PLAY=1` (press PLAY), `MELEE_PLAY_REPLAY=<n>` (press Watch on recent
  game n), `MELEE_LAUNCHER_THEME`, `MELEE_LAUNCHER_LIVE`, `MELEE_SPECTATE_DIR`, `MELEE_DASHBOARD_SAMPLE=1` (sample games),
  `MELEE_MU_ENGINE=<exe>` (another engine executable, e.g. a wrapper that adds `--hidden`), `MELEE_MU_ART=<dir>`,
  `MELEE_UPDATE_CHECK=0` (no GitHub update check; set it in tests). Run launcher tests with `HOME` pointing at a scratch
  folder so the player's own settings are not touched.
- Engine aids: `MELEE_MENU_OPEN=<retrace>` and `MELEE_MENU_PAGE` open the panel, `MELEE_CAPTURE_UI=1` captures it with
  `--capture`, `MELEE_HUD=1`, `MELEE_METAL_LATENCY=1`, `MELEE_METAL_COMPILE_LOG=1`, `MELEE_RENDER_THREAD=0`,
  `MELEE_REALTIME=0`, `MELEE_PAD_FILE=<file>`, `MU_FIXUP=1|2` (runner: accesses through bare console addresses).

## Design rules

- The interface is melee-unlocked's: keep the launcher's layout, palette and words as MU's Win32 launcher has them, and
  MU's panels as the fetched tree draws them. Colours come from the palette at the top of `mu_launcher.mm`.
- Keep the simulation thread free of anything that can block (display, GPU, disk, shader compiles).
- Exactness is sacred: a feature that changes the simulation is off by default and off online unless both peers agree.
- Licensing: anything ported, copied or adapted from another project gets a row in `THIRD_PARTY_NOTICES.md` with its
  licence and the evidence (a quoted licence statement or SPDX header, not a GitHub label: every GPLv2 `LICENSE` file
  contains "any later version" as template text). MIT and zlib notices not already in the tree go in that file. Never
  commit code or headers under proprietary terms.
- Online input delay is a player setting (launcher.ini `online_delay`, 1..9 or auto, default 2), on the launcher's Settings
  page and in the in-game panel; never hardcode it.

## Verification checklist for a change

1. Build (`tools/mac/rebuild.sh`, or the two CMake targets).
2. The exactness gate above, and the online loopback tests if netplay or the game image changed.
3. For launcher changes: screenshots with `MELEE_LAUNCHER_SHOT` (and `MELEE_LAUNCHER_PAGE`) under a scratch `HOME`, and look
   at them.
4. Commit with a message that says what changed for the player and why.
