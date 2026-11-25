# melee-unlocked-macOS — guide for coding agents and new contributors

melee-unlocked-macOS is a native Apple-silicon Mac app for Super Smash Bros. Melee with Slippi Online. Its engine is
melee-unlocked's (Hero88go/melee-unlocked) "Source Port": the Melee decomp's C compiled natively, fetched and built at
install time, never committed. The app grew out of Dashdance, a client that statically recompiles the game's PowerPC
code to C++. That engine is still in the tree as "Static Recomp (Legacy)" until the melee-unlocked engine does
everything it did; most of the layout and build rules below are Dashdance's and still apply to it. Read this file
first; it tells you how the tree is laid out, how to build, and the rules that are not obvious from the code.

## Repository and workflow (rkoh46/melee-unlocked-macOS, branch `melee-unlocked-macos`)

rkoh's repository: it was the playtest fork rkoh46/dashdance, renamed (the old URL redirects). Commits are authored
as `rkoh` (git config); never as the Mac login name.

- Remote: `origin` is rkoh46/melee-unlocked-macOS. Work on `melee-unlocked-macos` (the default branch; the README's
  install line fetches it). One change per commit; push after each. `dashdance2` is the old Dashdance-only branch:
  don't push new work there. Upstream Dashdance (TheAndersMadsen/dashdance) is never pushed to.
- The installed app is `/Applications/melee-unlocked-macOS.app`; its user data stays in
  `~/Library/Application Support/Dashdance` (launcher.ini, Slippi sign-in, replays, cards; the melee-unlocked engine's
  card is `melee-unlocked/Card A` there).
- The interface is melee-unlocked's, not Dashdance's: the launcher (`port/runtime/host/mu_launcher.mm`, rebuilt from
  MU's Win32 `port/app/launcher.cpp` paint code) and MU's in-game panels. MU's artwork and icon are converted from the
  fetched tree at packaging time (`tools/mu_launcher_art.py`) and never committed. YouGame is left out.
- The sister project is rkoh46/Melee-NX-NetPlay (the same engine on the Nintendo Switch,
  `experiments/native-engine/`). Code is shared by copying, with notices kept: the runner, the netplay bridge, the
  feature modules, the mod installer (`mu-engine/modinstall`). Its `FEATURES.md` is the full melee-unlocked feature
  list; `docs/FEATURES.md` here holds the Mac status per row.
- `/fix-logs` (`.agents/skills/fix-logs/`) reviews unreviewed session logs and crash reports and fixes what they show.
- `docs/MAC_FIXES.md` is the issue and fix log. `docs/PR.md` is the running change list: everything added to the
  repo (fix, feature, logging, tool, skill) adds one plain bullet there in the same commit. Keep it short.
- Capture evidence by hand with `tools/mac/report.sh "what happened"`; list unreviewed logs with `tools/mac/unreviewed.sh`.
- The disc path is in `.disc-path`; `deps/melee` must stay at the pinned decomp commit `05a1394f` (legacy engine).

## The melee-unlocked engine (`mu-engine/`)

- `mu-engine/fetch.sh` clones melee-unlocked at its pinned commit into `build/mu-engine/mu` and applies
  `mu-engine/patches/` (`0*.patch` against MU, `decomp-*.patch` against its decomp). Change MU only through patches.
- `mu-engine/build.sh game` builds MU's game image (an AArch64 Linux ELF) with GCC 15.2 in Docker (OrbStack): every
  container gets `--rm`, a `mumac-*` name, `--network none` and a time limit. `build.sh runner` builds the headless
  arm64 runner `build/mu-engine/out/mu-runner`; `tools/mac/rebuild.sh` / `setup.sh` build the app's
  `MeleeUnlockedEngine` (`mu-engine/app`) next to the launcher.
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
  the Switch, copy its files and patches again; port/runtime/hle (Slippi 3.6.4) stays the legacy engine's.
- Never run melee-unlocked's release binaries, launcher or updater, and never a mod's own programs: build MU from
  source only. Test windows stay hidden or offscreen (the user plays on this Mac).

## Legacy engine notes

- Replays (legacy): `DashdancePlayback` (`build/mac-playback`) played games back with Slippi's playback codes; the
  melee-unlocked engine now does playback itself.
- Rebuild with `tools/mac/rebuild.sh`, never a bare cmake build (the input manifest must be regenerated after an edit).
- Desyncs (legacy): `tools/mac/desync.py <replay.slp>` finds the first frame where the recording differs from Slippi
  Dolphin re-simulating it (`reports/desync/<replay>/report.md`; method in `docs/MAC_FIXES.md`).

## One-command setup

Players use `install.sh` (the one line in the README): it clones into ~/Dashdance (`DASHDANCE_DIR`), asks for the disc, runs
`setup.sh` (which builds the melee-unlocked engine when Docker is running), copies `melee-unlocked-macOS.app` to Applications
and opens it. Developers call `setup.sh` directly:

```bash
./setup.sh /path/to/melee.iso            # macOS app -> dist/melee-unlocked-macOS.app
./setup.sh /path/to/melee.iso --ios      # iPad/iPhone Simulator app (needs Xcode)
./setup.sh /path/to/melee.iso --visionos # Vision Pro Simulator app (needs Xcode)
./setup.sh /path/to/melee.iso --device   # dist/Dashdance.ipa for AltStore/SideStore/Sideloadly; --team auto signs and installs over USB
```

There is no store or public-release distribution and there must not be: the built app contains the
translated game. `tools/release.sh` and `.github/workflows/release.yml` build DMG/IPA release assets for a
private fork only (both refuse public repositories).

The script installs Homebrew packages (cmake ninja python), clones doldecomp/melee into
`deps/melee`, extracts `main.dol` from the disc (`tools/extract_dol.py`), fetches the pinned Aurora
dependency, generates the port, builds, packages and opens the app. Nothing from the game is
committed; the disc stays where it is.

## Layout

| Path | What lives there |
|---|---|
| `port/app/main_mac.cpp` | The app entry point for all Apple platforms: options, launcher/dashboard, game loop start. |
| `port/app/ios/`, `port/app/macos/` | Info.plist templates. |
| `port/app/icons/AppIcon.icon` | Icon Composer bundle (Dashdance mark as a glass layer). `tools/package_macos_app.sh` and CMake compile it with `actool`. |
| `port/runtime/host/` | Host services: window + input (`window_sdl.cpp`), dashboards (`mac_launcher.mm`, `ios_launcher.mm`), controller mapping, deadzones, trigger point, rumble and the keyboard layout (`input_config.*`), the Connect a Controller pairing steps (`controller_pairing.h`), the live GameCube controller in the controller editors (`gc_diagram.*`, drawing the GPL-3.0 ControllerOverlays artwork layers in `port/app/art/controller`; overlay coordinates are in the artwork's 3828 x 2689 units), dashboard model (`dashboard.*`), retrace/timing (`host.cpp`), touch overlay (`overlay.*`), GameCube adapter (`gc_adapter_iokit.cpp` on macOS: IOKit with a 1 ms pipe policy; `gc_adapter_libusb.cpp` elsewhere), controller report-rate measurement (`controller_rate.mm`), in-game menu and HUD (`game_menu.*`, drawn through the Metal overlay's glyph atlas), local notifications (`notify_apple.mm`). |
| `port/runtime/gx/` | The GX (GameCube GPU) translation and the Metal renderer (`gx_metal.mm`, `gx_msl.cpp`). |
| `port/runtime/hle/` | High-level emulation of the GameCube SDK the game calls (disc, pads, audio, memory cards) and Slippi's EXI device, login (`slippi_login.*`), replay parsing (`slippi_history.*`), Discord Rich Presence over the local IPC socket (`discord_rpc.*`). |
| `port/runtime/ppc/` | Guest CPU context, memory, interpreter fallback. |
| `port/recomp/` | The recompiler configuration; `hle_list.txt` names the guest functions replaced by host code. |
| `port/slippi_sys/` | Vendored Slippi game files (Sys folder). |
| `tools/` | Build helpers: `bootstrap_port.py` (generate/configure/build), `extract_dol.py`, `package_macos_app.sh`, `make_icons.py`. |
| `docs/` | Longer notes and README images. |

## Build rules that bite

- **Provenance gate.** Any edit under `port/` invalidates the generated manifest. Before building run
  `python3 tools/bootstrap_port.py --decomp-root <decomp> --dol <main.dol> --build-dir <build-dir> --gct-base 0x8065CC80 --macos-arch arm64 --stage generate`
  (or `./setup.sh`, which does it). Otherwise `port_verify_generated` fails with a one-line error.
- **Toolchains.** macOS builds run with the Command Line Tools (`unset DEVELOPER_DIR`). iOS and
  visionOS builds need `export DEVELOPER_DIR=/Applications/Xcode.app/Contents/Developer`. Mixing
  them produces precompiled-header mismatches.
- **Simulator runtimes.** Keep `xcode-select` on the Command Line Tools and prefix Xcode-only commands instead, e.g.
  `DEVELOPER_DIR=/Applications/Xcode.app/Contents/Developer xcodebuild -downloadPlatform visionOS`; without the prefix it fails
  with "requires Xcode". With the visionOS 26 runtime, create the simulator from the `Apple-Vision-Pro-4K` device type.
- **HLE hooks are matched by symbol name.** A name missing from `port/recomp/hle_list.txt` is silently
  recompiled instead of hooked (this is how a `longjmp` bug once corrupted guest memory).
- **Never build while measuring performance**, and shut down Simulators first: both distort the
  per-frame numbers.

## Running and diagnosing

- `build/<dir>/port/melee_port_mac --help` lists every flag. Useful: `--iso`, `--fullscreen`,
  `--no-vsync`, `--window WxH`, `--offline`, `--profile-dir`, `--cache-dir`, `--log-file`.
- Every 60 frames the log has `[frame N] ... sim: X ms/frame (worst Y) | <cost slots>`; frames over
  16.7 ms log `sim frame N took ...` with the cost slots that explain it.
- `MELEE_METAL_LATENCY=1` logs XFB-copy-to-panel latency from Metal's presented timestamps and GPU
  time. `MELEE_METAL_COMPILE_LOG=1` logs every shader compile. `MELEE_RENDER_THREAD=0`,
  `MELEE_REALTIME=0`, `MELEE_METAL_SYNC_COMPILE=1` turn the render thread, real-time scheduling and
  background shader compilation off for A/B tests.
- `MELEE_DASHBOARD_SAMPLE=1` fills the dashboard with sample data; `MELEE_LAUNCHER_SCROLL=<pt>` starts
  it scrolled; `MELEE_MARK=<glyph.png>` shows the Dashdance mark when running the bare binary.
- `MELEE_OPEN_EDITOR=keyboard` (or `pad:<guid>:<name>`) opens the dashboard's controller editor after launch (iOS takes `pad:` only); `MELEE_CONTROLLER_ART=<dir>` loads the controller artwork from another folder; `MELEE_OPEN_PAIRING=1` opens Connect a Controller; `MELEE_PLAY_REPLAY=<n>` presses Watch on recent game n (0 is the newest; `reveal:<n>` presses Show in Finder); `MELEE_TEXT_AUDIT=1` logs every label, button or picker whose text is cut off, needs more lines than its box, leaves the window or runs into the rounded shape around it (`text-audit [...] done: N problems`; run it on each device and orientation after UI changes, and look at the screenshots too); `MELEE_ORIENTATION=portrait|landscape` rotates the iPhone/iPad dashboard (the game follows the device: rotate the Simulator with Command-arrow), `MELEE_WINDOW_SIZE=WxH` sizes the Vision Pro window and `MELEE_LAUNCHER_SIZE=WxH` the Mac dashboard window;
  `MELEE_MENU_PAGE=controls` or `remap` opens that page of the in-game menu together with `MELEE_MENU_OPEN` (screenshots).
- `MELEE_MENU_OPEN=<retrace>` opens the in-game menu at that frame and `MELEE_HUD=1` turns the HUD on (screenshots);
  `MELEE_DUMP_GLYPHS=<file.pgm>` writes the text atlas; `MELEE_DISCORD_APP_ID=<id>` points Discord presence at another application (the built-in id,
  `slippi::discord::kApplicationId`, is Dashdance's own from slippi-rust-extensions PR 36, with its character, stage and rank artwork).
- `MELEE_PAD_FILE=<file>` drives the game from a text file (one line per pad: `p=1 A sx=127`), which
  is how the scripted match tests work without a window in focus.
- On the Simulator, prefix environment variables with `SIMCTL_CHILD_` and pass them to
  `xcrun simctl launch`.

## Design rules

- Apple platform conventions first: Liquid Glass on macOS/iOS 26 (`NSGlassEffectView`, `UIGlassEffect`)
  with material fallbacks, glass buttons, SF Symbols, native controls with visible labels and values,
  haptics only for user actions, a real menu bar on the Mac. See README "Design".
- Melee's own grammar stays: angled yellow section headers, italic display type, dark blue grid.
- Colours and glass tints come from the small theme helpers at the top of each launcher file; do not
  scatter literal colours.
- Keep the simulation thread free of anything that can block (display, GPU, disk, shader compiles).
- Licensing: anything ported, copied or adapted from another project gets a row in `THIRD_PARTY_NOTICES.md` with its
  licence and the evidence (a quoted licence statement or SPDX header, not a GitHub label: every GPLv2 `LICENSE` file
  contains "any later version" as template text). MIT and zlib notices not already in the tree go in that file. Never
  commit code or headers under proprietary terms.
- Online input delay is a player setting (`online_delay`, 1..9, default 2), exposed on both dashboards and the in-game menu; never hardcode it. On iPhone/iPad `power_play_begin` holds the display at full refresh and requests a 5 ms audio buffer (`apple_power.mm`); `latency_warning()` feeds the HUD. `competitive_readiness()` (apple_power.mm) feeds the Ready to compete card on both dashboards and the `readiness:` log lines; keep its wording player-facing. Rollback save states are timed as the `savestate` cost slot. The phase lock stays off: measured as noise.
- iPhone Duo (see docs/TECHNICAL.md): one game rectangle (`host::window_game_rect`) for renderer, touch layout and letterbox artwork; upright, the game never passes the middle of the display (the fold); controls and overlay text are sized in points; dashboard columns switch on size class and centre on the display; no global main screen; `UIRequiresFullScreen` stays off. The README is written for players; put technical material in docs/TECHNICAL.md.
- Every screen adapts: two columns of cards when the window is wide (Mac from 1100 pt, iPad and Vision Pro from 960 pt), a side-by-side controller editor in landscape, and touch controls, HUD and menu inside the safe area. Check portrait and landscape on iPhone and iPad and a narrow and wide window on Mac and Vision Pro with the aids above.

## Verification checklist for a change

1. Build the target you touched (macOS at least; iOS if you touched shared UI or Metal).
2. Run a scripted match and confirm `late` frames stay at zero (see the scripts described in
   `docs/PERFORMANCE.md`).
3. For UI: capture the dashboard (`MELEE_DASHBOARD_SAMPLE=1`) and look at it.
4. Commit with a message that says what changed for the player and why.
