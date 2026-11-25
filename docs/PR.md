# Mac fixes from playtesting

Found by playing Dashdance on an M4 Pro MacBook Pro (macOS 26) with a Mayflash adapter in Wii U mode.
Details and evidence for each are in `docs/MAC_FIXES.md`.

## Fixes

- Read the GameCube adapter when macOS rejects timed reads on its interrupt pipe (it was never read at all on this Mac).
- Quit with Cmd+Q from full screen.
- Show the adapter's measured polling rate in the HUD instead of rounding anything under 900 Hz to "125 Hz".
- Keep the launcher from crashing when a replay's player name isn't valid UTF-8.
- Fix desyncs against Dolphin players when UCF's shield drop fires. The Gecko code returns past its call site
  (return address + 8), and the recompiler now resumes the caller there instead of right after the call. A decision
  test now replays the Stadium desync's stick dive through the translated checks (25 cases), and online logs name a
  remote-input gap - the usual first domino of a desync - when it opens and on the next checksum mismatch.
- Fix the mid-match crash `guest call depth exceeded`. Guest `longjmp` unwinds as a C++ exception and skipped the
  call counter's decrement, so stage animations leaked about one level a frame until a long session hit the limit.

- The headless executable links again (it called Discord presence, which only the app has).

- A pipeline waiting in the boot-time precompile queue is compiled at once when a draw needs it. With a cold shader cache (after every app update) characters stayed invisible for up to a minute and a half while 900 pipelines compiled one at a time.

- Online games no longer end 30 s after connecting when the opponent's real port differs from the advertised one: an expired connection attempt under another address no longer marks the player as gone (docs/MAC_FIXES.md fix 9).

## Features

- Show live ping in the performance HUD during online matches.

- Recent games: every game gets two buttons, Watch (plays the replay back in a window) and Show in Finder (selects the .slp so it can be shared). Playback is the app's second executable, `DashdancePlayback`: the game translated with Slippi's playback codes and the replay code list, bundled by `tools/package_macos_app.sh` and built by `tools/mac/rebuild.sh`. Re-recordings of four online games matched the originals on every post-frame record (up to 21,860 per game).
- `--replay FILE.slp` and `--replay-codes FILE` on the Mac frontend; a replay whose code list needs another translation plays with Slippi's resync on and says so in the log.

## Logging

- Write a log file for every Mac session (no Mac session ever wrote one, so crashes left no trace).
- On a guest crash, log the guest call stack and all registers, and save guest RAM next to the session log.
  `MELEE_TEST_FATAL_RETRACE=N` triggers a fake crash to test this.
- Log every rollback (frame rolled back to, and from).
- Log the guest call depth every 60 frames, so a leak shows up long before it crashes.

- Netplay logs one `slippi: peer ...` line for every ENet connect, late connect, duplicate close and drop (slot, state, peer ids, RTT, loss, last receive, reason, whether it was the connection in use), to settle the 30-second direct-game drops (docs/MAC_FIXES.md issue 9).

## Debugging tools we built

All in `tools/mac/`, written up for agents in the `fix-logs` skill (`desync.md`, `crash.md`).

- `rebuild.sh`, `report.sh`, `unreviewed.sh`, `gcadapter_probe.cpp`: rebuild and install, capture a bug
  report, list logs nobody has read, probe the adapter.
- `desync.py`: one command to find a desync. It finds the first frame where a replay stops matching real Melee,
  checks whether Dashdance repeats it offline, compares memory at that frame, and renders what Melee shows.
- `slippi_frames.py`: renders any frame range of a replay with Slippi's own Dolphin, as ground truth.
- `dashdance_resim.py`: plays a replay back in headless Dashdance on the Mac. This builds on the existing replay
  playback, which was Windows-only: `bootstrap_port.py --playback`, headless `--replay`, and
  `MELEE_RAM_DUMP_FRAMES` to save memory at chosen frames.
- `slippi_ram.py`: saves Slippi Dolphin's game memory at chosen frames, through the debugger.
- `ramdiff.py`: compares Dashdance's and Dolphin's memory field by field (fighters, animation).
- `crashram.py`: shows what a crash's broken object is and what points at it.
- `slp.py`: small replay reader the others share.

- Simulation thread real-time budget raised from 5 to 8 ms (an M2 Pro needs 6 to 10 ms per online frame); `MELEE_RT_COMPUTE_MS` overrides it.
- Ready to compete warns when AirDrop/AirPlay (AWDL) is active on Wi-Fi, a known source of ping spikes.
- `MELEE_FORCE_CPU=<level>` and `port/scripts/cpu_match.txt`: a scripted level 9 CPU fight for measuring frame times in a real match.
- `MELEE_PHASE_SWEEP_MS`: sweeps the game/display phase to show whether latency depends on it (on an M2 Pro it does not).
- `APP_NAME=DashDance2 tools/package_macos_app.sh` packages a side-by-side copy with its own name, bundle identifier and an orange icon.
- docs/PERFORMANCE.md: M2 Pro in-match measurements; MetalFX at a fixed scale costs 3 ms of latency.

- `MELEE_PLAY_REPLAY=<n>` / `reveal:<n>`: presses a recent game's Watch or Show in Finder button (UI test aid).

- `slippi: peer health` every 5 s (unacknowledged reliable commands per ENet peer) and `MELEE_NET_DROP_CONNECT=1` (ignore incoming ENet connects, a one-way NAT) for netplay connection tests with `--local-peer`.
- `slippi: disconnect cause`: ENet records why a connection ended (the opponent's disconnect command, a reliable-data timeout, or this side) with the RTT and time since their last packet before `enet_peer_reset` wipes the peer, so a dropped game shows which side ended it.

- Installer works on a fresh Mac: `setup.sh` fetches doldecomp/melee at the commit pinned in `tools/port_source_pins.json` (it cloned HEAD, which `bootstrap_port.py` rejects), and `install.sh` clones full history, which the Slippi file check needs.
- `setup.sh` also builds the replay playback executable, so Watch works in an app built by the installer.
- A side-by-side copy gets its orange icon without Xcode too: `tools/package_macos_app.sh` falls back to `port/app/icons/AppIcon-orange-1024.png` when `actool` is missing.

## Before opening

- Drop the "Fork workflow" section from `CLAUDE.md`, `.agents/skills/fix-logs`, `.claude/skills/fix-logs` and this file.
- Set the default app name in `setup.sh` and `install.sh` (`DASHDANCE_APP_NAME`) back to `Dashdance`; the fork installs as DashDance2.
- melee-unlocked engine on Apple silicon (`mu-engine/`): MU v1.0.5 fetched and its game built from source at build time (build-time patches only); the console address space sits at 0x100000000000 because macOS keeps the low 4 GB of an arm64 process unmapped; a headless macOS runner plays the five reference scripts with digests identical to MU's x86-64 references.
- melee-unlocked engine app (`mu-engine/app`, `MeleeUnlockedEngine` in the bundle): the engine with the app's decoder, Metal renderer, SDL3 window, controllers and sound; the dashboard gets an Engine choice (melee-unlocked by default, Classic kept) and the app is named melee-unlocked-macOS. A scripted match in the app with picture and sound gives the same digest as the reference.
- melee-unlocked's launcher on the Mac (`port/runtime/host/mu_launcher.mm`): its layout, palette and words (rail with the MU mark, page nav and Bailey; Play with the Game Build row Source Port / Static Recomp (Legacy) / Vanilla; Settings; Replay Viewer; Mods; the launcher colour wheel), artwork and the app icon converted from the fetched tree at build time (`tools/mu_launcher_art.py`), never committed.
- Install path builds melee-unlocked-macOS: `setup.sh` fetches melee-unlocked and builds its game when Docker is there (classic engine only otherwise), `install.sh` clones the `melee-unlocked-macos` branch and replaces DashDance2.app; README section for players, docs/TECHNICAL.md section, docs/FEATURES.md with a Mac status per melee-unlocked feature.
- URLs point at rkoh46/melee-unlocked-macOS and its `melee-unlocked-macos` branch; `install.sh` moves an older download (rkoh46/dashdance, dashdance2) over when it has no local changes.
- Slippi online in the melee-unlocked engine: this repository's netplay behind the engine's own online code (`mu-engine/runner/netplay_bridge_mac.cpp`, linked as one object with only its interface global), Slippi 3.7.0's ONLINE_INPUTS reply layout for that engine (`reply_370`), a loopback matchmaking stand-in (`tools/mac/mock_matchmaking.cpp`, `MELEE_MM_LOOPBACK_PORT`) and sandboxed loopback tests (`mu-engine/tools/online_loopback_test.sh`, `net_rollback_test.sh`).
- The app's melee-unlocked engine goes online: Slippi's menus in the game (`mu-engine/app/slippi_menus.cpp`, the bundle's slippi_sys), the netplay host with the player's own Slippi sign-in, searches from the game's Online Play page; `mu-engine/tools/app_online_test.sh` plays two app engines on loopback.
- Replays in the melee-unlocked engine (`mu-engine/app/replays.cpp`, from Melee-NX-NetPlay's replays module): every match recorded as .slp with melee-unlocked's code lists, playback with `--replay` and a post-frame fidelity check; the launcher's Watch plays through the engine.
- The launcher's Mods page lists melee-unlocked's catalog and installs, updates and removes mods with the shared installer (`mu-engine/modinstall`, from Melee-NX-NetPlay; downloads through the system libcurl); `mu-engine/tools/mod_install_test.sh` runs its host test on macOS.
- ImGui on the Metal renderer, groundwork: `gx::metal_set_ui_pass` draws a provider last in the present pass; `mu-engine/fetch.sh` fetches ImGui's Metal back end at melee-unlocked's version.
- melee-unlocked's settings panel in the engine, in place of the app's in-game menu: its panel code from the fetched tree (`mu-engine/app/ui`, from Melee-NX-NetPlay's frontend), Dear ImGui on the Metal renderer, Source Sans 3 and its artwork embedded at build time; changes save to launcher.ini; `MELEE_CAPTURE_UI=1` puts the panel in `--capture` screenshots.
- Melee-NX-NetPlay's frontend layer in the engine (`mu-engine/app/fe`): melee-unlocked's game options from settings.ini with the panel's pages, the feature modules (Training, overlays, network overlay, search, spectator, replays with the viewer, takeover, coach tips and library, Fight a pro and the Practice menu on the Switch build's own ONNX engine), mod profiles loaded at an offline start (the launcher's Mods page has Use), controller chords, `--play-script` for tests.
- The Metal renderer draws the picture options (brightness, contrast, vibrance, screen filters, the CRT look, picture shape) and runs display-rate presentation with sub-frames (`gx::metal_set_present_loop`, fe/features/present); all off by default. The Switch's controller setup (fe/controls.cpp: bindings, profiles, Dolphin import, box mode, radius, snapback, trigger cap) is on the panel's CONTROLS page over the app's IOKit adapter and SDL input.
- Replays: a new fighter starts cleared where a replay records it (decomp-0012, 0012: fp+2218 bit 5, and the whole block while a match is recorded), so a menu-started vanilla recording and its playback agree on the state flags; `--play-script` takes melee-unlocked's script format through the app's input path, so play-mode recordings can be made by script. The frontend layer follows Melee-NX-NetPlay main's online2 and training2 merges (Practice menu search, Practice CPU rows, Squad Strike, the game's calendar from the local clock).
- Netplay as on the Switch's online2 (`mu-engine/net_sources.sh` applies `mu-engine/patches/net-0001..0004` to a copy of port/runtime/hle for the engine's netplay object): search info and the kept search (play while you search, the keep-search question, Direct and Teams codes, warm-up partner, search again), same-router fallback, slow first frame, Slippi's services in the engine (display-name refresh, game reports with the disc MD5, replay upload from the replay writer). Tests: `tools/mac/mock_slippi_http.py`, `mock_matchmaking --dead-lan`, `mu-engine/tools/app_online_menus_test.sh` (from the game's menus with Melee-NX-NetPlay's controller scripts in `mu-engine/tests/online`); `--play-script` also takes scripts with `@scene MODE STATE` blocks (port/runtime/host/input_script.cpp from Melee-NX-NetPlay, with `@rearm`).
- Looks and skins from the Switch: the look presets, Lightweight, Visual effects, the look's background and True widescreen on the Metal renderer (`gx::LookHooks` in gx_metal.mm, rules in `mu-engine/app/gx_looks_mac.cpp`, the decoder copy reads the game's draw owner tokens 0xF0/F1/F2/F7/F8), fe/features/looks.cpp and settings rows; skins (fe/features/cosmetics.cpp) with melee-unlocked's own online rule compiled from the fetched tree (`mu-engine/tools/app_skins_test.sh`); TM-CE's Export/Import fix (`mu-engine/patches/0013`). All off by default.
- Slippi 3.7.0 online in the melee-unlocked engine: the netplay object is now Melee-NX-NetPlay's 3.7.0 netplay (`mu-engine/net`: the WebSocket matchmaking queue with STUN, 9-byte pads, player-drop reports, input suggestions received, Slippi's HTTP services over libcurl, its patches net-0001..0010: search info and kept searches, same-router fallback, slow first frame, display-name refresh, report replay upload, port colours and pick previews between its builds) with `mu-engine/net/patches/mac-0001..0002` (runtime loopback stand-ins for tests). Slippi 3.7.0 refuses 3.6.4 clients, which this repository's port/runtime/hle still is (legacy engine). Discord Rich Presence in the engine (the app's discord_rpc). Tests: `tools/mac/mock_matchmaking_ws.py` replaces the ENet `mock_matchmaking.cpp`; net checks compare the checkpoints both sides reached.
- Replays: a vanilla recording started from the menus now plays back with 0 differing records. The playback's first-frame spawn correction reset the fighter's collision data even when it did not move the fighter, which nudged frame -118 by one ulp (`mu-engine/patches/0014`, docs/MAC_FIXES.md #10). `mu-engine/build.sh game` stops on a compile error instead of carrying on with the old image.
- Online: this player's Slippi 3.7.0 input suggestions are sent. The netplay object's sampler reads the controller beside the game's poll through the app's input path (`mu-engine/app/mu_play_main.cpp` suggest_read, handed over with `netplay_bridge::set_suggestion_hooks`; the controller setup keeps a second filter lane). `app_online_menus_test.sh` takes `MU_SCRIPT_PLAYS=1` (the scripts play the match; `--fake-user-index` names the made-up user) and `MU_SUGGEST_LEAD_MS`.
- Mac desktop: Background input (settings.ini `background_input`, a panel row), the pointer hides over the game, the display stays awake while the engine's window is up, Control-Command-F and Alt+Return toggle full screen (the panel row follows), and a paused replay keeps the window answering. The launcher's Replays page gets Open a replay... and Watch live (the Slippi Launcher's spectate folder; the engine plays a file still being written, `tools/mac/slp_live_writer.py` tests it). The Play page checks this repository's branch on GitHub against the built commit (Info.plist MUBuildCommit) and says when a newer build is there.
- The sound follows Slippi's time-sync speed online (the Switch's 0110 in port/runtime/host/audio_sdl.cpp), an Audio buffer setting (`audio_buffer_ms`), melee-unlocked's "Settings: F1" reminder in the menus (`settingsreminder`, off by default), and the adapter line no longer says the Mac does not read it. docs/FEATURES.md: rows that were already working (training tools, 20XX TE and mod-disc features wired as on the Switch, overlays, pipeline cache, launcher pages) say so, and PC-only rows say not applicable.
- Launcher: a Disc line on the Play page from the image's header (NTSC 1.02 or what is wrong), and `MELEE_LAUNCHER_SHOT=<file.png>` draws the launcher off-screen into a file for screenshots (with `MELEE_LAUNCHER_PAGE`).
- Use custom music (melee-unlocked's jukebox and music packs, `musicpacks`, off by default) with MP3/OGG/WAV/FLAC decoders fetched at pinned commits and AudioToolbox for M4A/AIFF, and melee-unlocked's Auto audio buffer (per output device) beside Low latency, from the Switch build (`fe/features/audio.cpp`, `mu-engine/tools/app_audio_test.sh`).
- Player Gecko codes from `melee-unlocked/GeckoCodes.ini`, offline, as melee-unlocked runs them on the Source Port (its user_gecko.cpp from the fetched tree, the game image's writer), listed and switched on the panel's GECKO CODES page (`fe/features/gecko.cpp`, `mu-engine/tools/app_gecko_test.sh`).
- Auto L-cancel online in a Direct match when every player offers it, through the build message flags the Switch build uses (`mu-engine/net/patches/net-0011`), so Mac and Switch builds agree; the Direct test script `mu-engine/tests/online/online_direct_a.txt`.
- Replay library in the engine panel finished as the Switch has it: search, watch queue, as it was played from the session trace, Compress all (.slpz), delete to .trash with undo; a text prompt for the search (`ui/software_keyboard_mac.mm`); `mu-engine/tools/app_replay_library_test.sh`.
- Launcher: "Save crash report" writes melee-unlocked's crash report zip and .md to `melee-unlocked/Logs` with its privacy scrubbing (no upload); the engine keeps the previous session's log as `engine.prev.log` (`mu-engine/app/crash_report.cpp`, `mu-engine/tools/crash_report_test.sh`).
- RVZ and WIA discs: the engine converts one into GALE01.iso once at start with melee-unlocked's disc_rvz.cpp (`mu-engine/app/disc_convert.cpp`, `mu-engine/tools/rvz_check.sh`).
- The static-recompilation engine is removed: the app is melee-unlocked's engine and its launcher only. Gone: the recompiler (`port/recomp`), the translated-game runtime (PowerPC runtime, HLE, Slippi 3.6.4 netplay, the classic dashboard and in-game menu), the DashdancePlayback replay build, the input manifest, the decomp checkout and DOL extraction in `setup.sh`, the iPhone, iPad and Vision Pro targets, Aurora (SDL 3.4.10 is now fetched directly), and the Windows files. The launcher (`port/app/main_mac.cpp`) has no engine picker (Game Build: Source Port or Vanilla), Watch always plays in the engine, launcher.ini keeps the engine's own keys when the launcher saves it, and an earlier install's settings carry over (the Static Recomp choice, widescreen into settings.ini, the memory card into `melee-unlocked/Card A`).
- A fresh install applies every melee-unlocked patch again: `mu-engine/patches/0012`'s hunk for `mu_native.h` had lost that file's CRLF line endings, so `fetch.sh` stopped at it on a new clone; `.gitattributes` keeps patch files byte for byte.
- Skins: MODS > Choose skins (costume and skin rows, as the character select cycle), each mod disc's skin count in `mods-cache/disc-skins.txt` for the Mods page, and the count of picked skins served standard online (for the panel's note), from the Switch's cosmetics module.
- melee-unlocked's character select easter egg (its walter_egg.cpp from the fetched tree through `fe/egg.cpp`, the 0xDA frame command in `fe/features/egg_command.cpp`), from the Switch; `mu-engine/tools/app_egg_test.sh`.
- Settings panel: all of melee-unlocked's menu styles (Icon tiles, GD Melee with its kit art, Radial, Wide tabs, Simple, the Classic and Legacy Old menus; F11), palettes as swatches, Window transparency, the custom accent colour wheel and the style gallery on CUSTOMIZE, menu sounds (`menusounds`, mixed into the game output) and the note when skins are off online, merged from the Switch's panel; the panel takes the pointer and scroll wheel; launcher.ini reads are cached; `MELEE_MENU_PAGE` takes a category name and `MELEE_MENU_LEGACY`, `MELEE_TEST_STYLE_GALLERY`, `MELEE_TEST_ACCENT_WHEEL` are screenshot aids; `app_audio_test.sh menusounds`.
- Launcher: melee-unlocked's nine languages (Language button, its lang/ tables in the bundle), What's new in a card (first start of a version and the version line's link, `mu-engine/app/whats-new.txt`), the rail picture (click Bailey), each mod disc's skin count on the Mods page; `MELEE_LAUNCHER_NOTES` for screenshots.
- Mods catalog refreshed from melee-unlocked's raw catalog once per launcher start (same rules, kept in `Mods/.cache`, used by the engine's MODS page too; `MU_NO_CATALOG_REFRESH`, `MU_CATALOG_URL`).
- Test aids from the Switch: the network condition simulator (`MELEE_NET_LOSS_PCT`, `MELEE_NET_LAG_MS`, `MELEE_NET_JITTER_MS`, `mu-engine/net/patches/net-0012`) and the Slippi command trace (`MELEE_TRACE_SLIPPI_CMDS=1`).
- Player Gecko codes reach the fighters: the codes also run as each disc file opens, so a start-data value (model scale) is in place when the VS scene start builds the fighters within one frame (`fe/features/gecko.cpp`; `app_gecko_test.sh` takes a match screenshot, `MU_GECKO_OFF=1` for the picture without).
- `mu-engine/tools/app_te_test.sh` (20XX TE input display and menu music from Sound Test with the TE save as a profile); `app_online_test.sh` takes `MU_INI` (settings.ini lines for both sides).
- Settings panel: upstream's inline search box, typed on the keyboard (SDL text input fed to ImGui while a box has the cursor); upstream's `MELEE_TEST_*` hooks read the environment.
- Training Mode CE: `mu-engine/tools/app_tmce_test.sh` (the TM-CE disc profile into the Training Lab); the panel's MODS page names the built-in packs (TM-CE, 20XX TE) and whether they are loaded, as upstream's.
- Tests: `mu-engine/tests/online/online_marks_a.txt` (trace marks pressed in an online match) and `app_online_menus_test.sh` `MU_COSMETICS_A` (the skins-off-online note); `app_online_test.sh` `MU_COSMETICS`.
- `app_te_test.sh lock` (`--feature-test te-lock`, `fe/features/te_lock_test.cpp`): 20XX TE's lock settings through command 0xF8.
- `mu-engine/tools/app_save_import_test.sh`: a profile's .gci goes onto its card once.
- Launcher Replay Viewer: a picked game's card with melee-unlocked's stats and the session trace graph (`mu-engine/app/replay_stats_facade.cpp`), Move to Trash; `MELEE_LAUNCHER_PICK` for screenshots.
- Netplay: `MELEE_SYNC_SMOOTH=1` (upstream's experimental smoother time sync, `mu-engine/net/patches/mac-0003`); `mu-engine/tools/slp_ranks.py` reads the ranked info and bot flags from a replay's game start.
- Keyboard key names in the game's prompts (command 0xDF from the keyboard layout, `fe/features/key_labels.cpp`).
- `mu-engine/tools/slp_strip_codes.py`: a legacy-style copy of a replay (no Gecko list) for the legacy code list path.
- Replays: a replay on a mod stage is refused with upstream's message (launcher and engine), and one recorded with other mod content is refused as upstream's check_replay_content does; `mu-engine/tools/slp_set_stage.py`.
- Reset CPU training options (Computer players), the replay viewer's mute (`replaymute`), the game thread at user-interactive QoS (`fe/features/extras.cpp`, `mu_play_main.cpp`).
- Diagnostics: the engine's summary lines and startup timing; launcher.log kept across starts up to 256 KB, dated, with the reason when the engine did not start.
- Crash report: the launcher log in the zip, a GPU-driver line when the crashed thread is in AGX/Metal; the test reads a made-up crash report in a home of its own.
- Restart the game from the panel (confirmed with a second A, offline) and the Arcade look preset/reset rows (`fe/features/extras.cpp`, `--feature-test extras`).
- AUDIO page: the output's gaps this session and its buffer (upstream's read-out).
