# melee-unlocked features on the Mac

Every row of the melee-unlocked feature list (the same list the Switch build keeps, from melee-unlocked 1.0.5), with its
status in melee-unlocked-macOS. The exactness gate comes first: the five reference scripts give digests identical to
melee-unlocked's x86-64 references, in the headless runner and in the app with picture and sound.

| Mac status | Meaning |
|---|---|
| active | On in the Mac app with the melee-unlocked engine. |
| in the engine | Part of the game image and on by default; the Mac app has no setting for it yet. |
| engine option, no setting | In the game image behind an option word; the Mac app does not set it yet. |
| not wired | Exists upstream; the Mac host, renderer or launcher does not provide it yet. |
| out of scope | YouGame. |


## 2. Frame-rate unlock

| Feature | Side | Mac status |
|---|---|---|
| Sub-frame animation: Off | renderer | active: display-rate presentation on the Metal renderer (fe/features/present through `gx::metal_set_present_loop`); off by default |
| Sub-frame animation: Predict ahead | game image + host + renderer | active: settings.ini subframe 1 (or --frame-mode predict); tested at 120 presents per second (1797 presents for 894 game frames) |
| Sub-frame animation: Interpolate | same | active: settings.ini subframe 2 |
| Generic matrix modes | renderer | active: the sub-frame solver's matrix modes (port/runtime/gx/subframe.cpp) |
| Frame cap | renderer | active: settings.ini fps N |
| Sub-frame pause when the game falls behind online | host | not wired |

## 3. Online

| Feature | Side | Mac status |
|---|---|---|
| Slippi Unranked, Direct, Teams, Party | game image + host + external | active: from the game's Online Play page with the player's Slippi sign-in; tested on loopback only (two engines, Unranked, net check equal, 0 desyncs) |
| Slippi system files | host | active: Slippi's menu files served from the bundle's slippi_sys (`mu-engine/app/slippi_menus.cpp`) |
| Slippi Ranked | host | refused, as melee-unlocked refuses it |
| Rollback savestates | host | active: the hot-page store with macOS write tracking (580 rollbacks, net check equal, in the loopback rollback test) |
| Slippi account | host | active: the app's own sign-in file, or the Slippi Launcher's when the app has none |
| Matchmaking | host + external | active: the app's matchmaking client (ENet); tested against the loopback stand-in only |
| Online input delay | host | active: the launcher's setting (1-4, or the in-game menu's larger value) is passed to the engine |
| Time sync, frame advance/skip | host | active (the app's netplay; frame-rate nudges go to the host's pacing) |
| Desync detection and notice | game image + host | active: the game's checksum comparison (0 desync messages in the tests) |
| Player drop handling (3.7.0) | game image + host | partial: Slippi 3.7.0 despawn frames in the ONLINE_INPUTS reply; not tested with a real drop |
| Slippi input suggestions (3.7.0) | host | not wired: the reply carries the latest real input as every prediction (Slippi's behaviour without suggestions) |
| Version gate | host | not wired: online play is the next milestone (netplay host as on the Switch) |
| Replay recording (.slp) | game image + host | active: every match, offline and online (`mu-engine/app/replays.cpp`) |
| Compressed replays (.slpz) | host | active: settings.ini compressreplays 1 (melee-unlocked's zstd) |
| Replay playback | game image + host | active: `--replay` and the launcher's Watch, through the replays module (`fe/features/replays.cpp`); a replay without a code list plays as Vanilla |
| Replay viewer controls | host + renderer | active: Start/Z pause, D-pad seek and step, L speed, R fast forward, X/Y tips (the replays module, drawn by `fe/overlay_draw.cpp`) |
| Replay takeover | game image + host | active: A takes over (the replays module) |
| Replay coach tips | game image + host + renderer | active: melee-unlocked's coach (replay_coach.cpp from the fetched tree) through the replays module |
| Game reporting | host + external | not wired: online play is the next milestone (netplay host as on the Switch) |
| Quick chat | game image + host | not wired: online play is the next milestone (netplay host as on the Switch) |
| Stage pool and random stage | game image + host | not wired: online play is the next milestone (netplay host as on the Switch) |
| Pause in Direct and Friends | host | not wired: online play is the next milestone (netplay host as on the Switch) |
| Skip in Unranked | host | not wired: online play is the next milestone (netplay host as on the Switch) |
| Early inputs ("subframe rollback") | host + game image | not wired: online play is the next milestone (netplay host as on the Switch) |
| Early rollback from early pads | host | not wired: online play is the next milestone (netplay host as on the Switch) |
| Build message and setup hash | host | not wired: online play is the next milestone (netplay host as on the Switch) |
| Opponent shield colour | game image + host | not wired: online play is the next milestone (netplay host as on the Switch) |
| Online port colours | game image + host | not wired: online play is the next milestone (netplay host as on the Switch) |
| Live pick previews | host + game image | not wired: online play is the next milestone (netplay host as on the Switch) |
| Opponent origin label | host | not wired: online play is the next milestone (netplay host as on the Switch) |
| Play while you search | host + game image + renderer | active for Unranked, Direct and Teams: the Switch's search module (fe/features/search.cpp) on the netplay object's kept search (mu-engine/patches/net-0001). L+R held on Slippi's select with a search running and the fighter locked in: Training while the search goes on; B asks "Keep searching while you play?" (settings.ini `keep_search`); found: back to the select and the match. `mu-engine/tools/app_online_menus_test.sh` from the game's menus on loopback: online_keep_search (Unranked, Training), online_keep_ask (the question, A), online_direct_keep (Direct by a remembered code): each found, back on the select locked in, match played, net check equal, 0 desync messages |
| Warm-up room | host | wired, not run on the Mac: the same search module and kept connection; the sparring partner reads the opponent's locked-in pick (net-0001 `remote_character`). The Switch's test needs `--feature-test unranked-as-direct` (the loopback stand-in pairs Unranked), which the Mac engine does not take yet |
| Training after online | host | wired, not run on the Mac: settings.ini `online_training_after` / `online_return_training` and "Search again?" (search module, net-0001 `code`); the Switch's test needs `--feature-test unranked-as-direct` |
| Arena rooms and invites | host + renderer + game image | not wired: online play is the next milestone (netplay host as on the Switch) |
| Slippi spectator server | host | not wired: online play is the next milestone (netplay host as on the Switch) |
| Ping readout | renderer | not wired: online play is the next milestone (netplay host as on the Switch) |
| Network and timing overlay | renderer + host | not wired: online play is the next milestone (netplay host as on the Switch) |
| Session trace file | host | not wired: online play is the next milestone (netplay host as on the Switch) |
| Agreed Gecko codes online | host | not wired: online play is the next milestone (netplay host as on the Switch) |
| YouGame online: sign-in | host + renderer + external | out of scope |
| YouGame Ranked | game image + host + external | out of scope |
| YouGame Unranked, with "Both" search | game image + host + external | out of scope |
| YouGame Friends | game image + host + external | out of scope |
| Friends live handoff | host | not wired: online play is the next milestone (netplay host as on the Switch) |
| YouGame Doubles | game image + host + external | out of scope |
| Local partner in Doubles | host + game image | not wired: online play is the next milestone (netplay host as on the Switch) |
| Squad Strike | game image + host | not wired: online play is the next milestone (netplay host as on the Switch) |
| YouGame tournament / CPU bracket | game image + host + external | out of scope |
| Console link | game image + host + external | not wired: online play is the next milestone (netplay host as on the Switch) |
| YouGame relay and UDP transports | host + external | out of scope |
| YouGame version pin | host + launcher | out of scope |
| mu_net P2P Direct | host + game image | not wired: online play is the next milestone (netplay host as on the Switch) |
| Discord Rich Presence and Join | host | not wired: online play is the next milestone (netplay host as on the Switch) |

## 4. Gameplay options and code packs

| Feature | Side | Mac status |
|---|---|---|
| Slippi General Codes + Lagless FoD | game image | in the engine |
| UCF 0.84 | game image | engine option, no setting |
| UCF 0.8 | game image | engine option, no setting |
| Unlock everything | game image | active: settings.ini (melee-unlocked's key, `mu-engine/app/fe/settings.cpp`), with a row in the settings panel where the Switch has one |
| Show results screen | game image | active: settings.ini (melee-unlocked's key, `mu-engine/app/fe/settings.cpp`), with a row in the settings panel where the Switch has one |
| Freeze Pokemon Stadium | game image | active: settings.ini (melee-unlocked's key, `mu-engine/app/fe/settings.cpp`), with a row in the settings panel where the Switch has one |
| Freeze Final Destination | game image | active: settings.ini (melee-unlocked's key, `mu-engine/app/fe/settings.cpp`), with a row in the settings panel where the Switch has one |
| Fountain of Dreams reflections | game image | active: settings.ini (melee-unlocked's key, `mu-engine/app/fe/settings.cpp`), with a row in the settings panel where the Switch has one |
| Full FoD reflections | game image | active: settings.ini (melee-unlocked's key, `mu-engine/app/fe/settings.cpp`), with a row in the settings panel where the Switch has one |
| Widescreen 16:9 (Slippi code) | game image | active: settings.ini (melee-unlocked's key, `mu-engine/app/fe/settings.cpp`), with a row in the settings panel where the Switch has one |
| True widescreen | game image + renderer | engine option, no setting |
| Disable screen shake | game image | active: settings.ini (melee-unlocked's key, `mu-engine/app/fe/settings.cpp`), with a row in the settings panel where the Switch has one |
| Screen shake strength | game image | active: settings.ini (melee-unlocked's key, `mu-engine/app/fe/settings.cpp`), with a row in the settings panel where the Switch has one |
| PAL stock icons | game image | active: settings.ini (melee-unlocked's key, `mu-engine/app/fe/settings.cpp`), with a row in the settings panel where the Switch has one |
| Stock icon and damage number size | game image | active: settings.ini (melee-unlocked's key, `mu-engine/app/fe/settings.cpp`), with a row in the settings panel where the Switch has one |
| Always show player tags | game image | active: settings.ini (melee-unlocked's key, `mu-engine/app/fe/settings.cpp`), with a row in the settings panel where the Switch has one |
| Low poly fighters | game image | active: settings.ini (melee-unlocked's key, `mu-engine/app/fe/settings.cpp`), with a row in the settings panel where the Switch has one |
| Hide KO explosions | game image | active: settings.ini (melee-unlocked's key, `mu-engine/app/fe/settings.cpp`), with a row in the settings panel where the Switch has one |
| Hide emblems behind percents | game image | active: settings.ini (melee-unlocked's key, `mu-engine/app/fe/settings.cpp`), with a row in the settings panel where the Switch has one |
| Training HUD on/off | game image | active: settings.ini (melee-unlocked's key, `mu-engine/app/fe/settings.cpp`), with a row in the settings panel where the Switch has one |
| Character select background plays through | game image | active: settings.ini (melee-unlocked's key, `mu-engine/app/fe/settings.cpp`), with a row in the settings panel where the Switch has one |
| Game language | game image | active: settings.ini (melee-unlocked's key, `mu-engine/app/fe/settings.cpp`), with a row in the settings panel where the Switch has one |
| Shield colours per port | game image + host | active: the options module (`fe/features/options.cpp`), settings.ini |
| Community codes: HUD transparency, Nana's percent, ally indicators, respawn platform by stocks, hide GO!, V-cancel sound, hide fighter shadows, centre the 2P HUD, hitbox colours by id, priority order, knockback angles | game image (+ renderer for Nana and ally overlays via `hud_extras`) | active: settings.ini (melee-unlocked's key, `mu-engine/app/fe/settings.cpp`), with a row in the settings panel where the Switch has one |
| Offline codes: Instant Melee, unrestricted pause camera, Boot to CSS | game image | active: settings.ini (melee-unlocked's key, `mu-engine/app/fe/settings.cpp`), with a row in the settings panel where the Switch has one |
| Zelda starts as Sheik | game image + host | active: the options module, settings.ini and the panel |
| Offline input delay | host | active: the options module, settings.ini and the panel |
| Mute character voices | game image | active: settings.ini (melee-unlocked's key, `mu-engine/app/fe/settings.cpp`), with a row in the settings panel where the Switch has one |
| Faster sound effects | game image | active: settings.ini (melee-unlocked's key, `mu-engine/app/fe/settings.cpp`), with a row in the settings panel where the Switch has one |
| Auto L-cancel | host | active: the options module, settings.ini and the panel |
| L-cancel flash (MU missed / success / both) | host + renderer | active: the overlays (`fe/overlay_draw.cpp`), settings.ini |
| Player-pasted Gecko codes | host + game image | engine option, no setting |
| Gecko code kinds accepted | host | not wired |
| Gecko code kinds refused | host | not wired |
| 20XX TE: master switch and Tournament Mode | game image + host | engine option, no setting |
| 20XX TE word 1: hold Start to pause, frozen stages, no Star KO, infinite shields, taunt cancel, play after GAME!, fixed camera, CPU Zelda as Sheik, handicap sets stocks | game image | engine option, no setting |
| 20XX TE word 2: neutral spawns, v1.00 rules, quieter DL64, reset tournament rules, Y toggles frozen, skip results, random music, shield colours on L/R, no screen rumble, TE L-cancel flash, spoof plugins, collision bubbles, CPU smart DI, colour overlays, hand-warmers, stage striking | game image (quieter DL64 is host: jukebox gain) | engine option, no setting |
| 20XX TE input display, lock settings | renderer / host | not wired |
| 20XX TE in-game settings menu | game image + host | engine option, no setting |
| 20XX TE menu music from Sound Test | game image + host | engine option, no setting |
| 20XX TE L-cancel training wheels | game image | engine option, no setting |
| 20XX TE memory-card replay browser, eight-character tags, save extensions | n/a | not wired |
| 20XX CPUs | game image | active: settings.ini (melee-unlocked's key, `mu-engine/app/fe/settings.cpp`), with a row in the settings panel where the Switch has one |
| CPU training options | game image | active: settings.ini (melee-unlocked's key, `mu-engine/app/fe/settings.cpp`), with a row in the settings panel where the Switch has one |
| Vanilla-only session | host | active: settings.ini vanilla_only (its own memory card folder, no mods) |
| Code-free retail game | game image | active: settings.ini (melee-unlocked's key, `mu-engine/app/fe/settings.cpp`), with a row in the settings panel where the Switch has one |

## 5. Training tools

| Feature | Side | Mac status |
|---|---|---|
| Practice menu | renderer + host | active: melee-unlocked's Practice menu (Start + D-pad Up + Z in offline Training, or the panel's Training row), drawn with ImGui; tested in Training |
| Practice CPU | game image + host | active: from the Practice menu (the Training module, `fe/features/training.cpp`) |
| Save states in Training | game image + host | active: the Training module (snapshot store) |
| Record and play back the CPU | game image | engine option, no setting |
| Collision bubbles | game image | engine option, no setting |
| Frame data and input overlay | renderer + host | active: the overlays module and `fe/overlay_draw.cpp` |
| Lab dummy DI, after-hitstun reaction, tech choice | game image | engine option, no setting |
| Frame advance | host + game image | active: the Training module |
| Fixed or frozen random seed | host + game image | engine option, no setting |
| Fight a pro (Phillip) | host + game image + external | active: Phillip's policies on the Switch build's own ONNX engine (`fe/proai`, onnxruntime-shaped calls, no onnxruntime library); downloaded when the player asks (Hugging Face, SHA-256 checked); tested: Zain's Marth policy fought in Training (0.6 ms per step) |
| Practice ghost AI | host | active: `fe/features/proai.cpp` |
| Pro habit tables | host | active: `fe/features/proai.cpp` (melee-unlocked's tables) |
| Training Mode CE events | game image + host | engine option, no setting |
| Training Mode CE on-screen displays | game image | engine option, no setting |
| TM-CE console global OSD shortcut | n/a | not wired |
| Practice from replays | game image + host + renderer | engine option, no setting |
| Lossless adapter lag test | game image + host | engine option, no setting |
| Display latency test | renderer | active: the overlays (latencytest) |

## 6. Mods, added content, cosmetics, music

| Feature | Side | Mac status |
|---|---|---|
| Mod profiles and disc-file overlay | host | active: the profile chosen for the next start (settings.ini mod_profile: the launcher's Mods page Use button, or the panel's MODS page) is loaded at an offline start (`fe/mods.cpp`); tested with a save layer |
| Content view switch | game image + host | engine option, no setting |
| m-ex content | game image | engine option, no setting |
| m-ex stages | game image | engine option, no setting |
| Akaneia fighters | game image + host | engine option, no setting |
| ACE fighters | game image | engine option, no setting |
| ACE Games menu | game image | engine option, no setting |
| Rogue Mode | game image + host | engine option, no setting |
| Melee Maker | game image + host | engine option, no setting |
| 20XX Hack Pack: stage variants and stage select pages | game image + host | engine option, no setting |
| 20XX Hack Pack: music playlists | game image + host | engine option, no setting |
| 20XX Hack Pack: debug menu, training displays, hidden characters | game image | engine option, no setting |
| Per-profile and vanilla-only memory cards | host | active: `fe/mods.cpp` (card-<profile>), settings.ini vanilla_only |
| Costume, stage and effect imports | host + renderer | not wired |
| Skin cycling on the select screens | game image + host | engine option, no setting |
| Voice mods | host + game image | engine option, no setting |
| Texture packs | renderer | not wired |
| Game music | game image | engine option, no setting |
| Music volume | host / game image | active: settings.ini (melee-unlocked's key, `mu-engine/app/fe/settings.cpp`), with a row in the settings panel where the Switch has one |
| Custom music packs / jukebox | host | not wired |
| Look presets | renderer | not wired |
| Lightweight levels and visual effects tiers | renderer (+ game image for low poly) | engine option, no setting |
| "Get mod looks" | launcher / external | not wired |
| Arcade look: attack and projectile hitboxes | game image | active: settings.ini (melee-unlocked's key, `mu-engine/app/fe/settings.cpp`), with a row in the settings panel where the Switch has one |
| Arcade look: grid or plain background | game image + renderer | active: settings.ini (melee-unlocked's key, `mu-engine/app/fe/settings.cpp`), with a row in the settings panel where the Switch has one |
| Arcade look: 2D camera | game image offline; renderer online | active: settings.ini (melee-unlocked's key, `mu-engine/app/fe/settings.cpp`), with a row in the settings panel where the Switch has one |
| Arcade look: combo counter | renderer | active: the overlays |
| 2D-style m-ex build | game image | engine option, no setting |
| Lab view | renderer | not wired |
| Looping video backgrounds | renderer | not wired |
| Border art | renderer | active: the overlays (a .bmp in the Borders folder) |
| Built-in CRT look | renderer | active: settings.ini crt_filter 1/2 and crt_curve, drawn in the Metal present pass; off by default |
| RetroArch shader presets | renderer | not wired |
| Easter egg | renderer + game image | engine option, no setting |

## 7. Controllers

| Feature | Side | Mac status |
|---|---|---|
| Controller input to the game | host | active: the app's controller path (keyboard, SDL gamepads, GameCube adapter over IOKit, rumble) |
| GameCube adapter | host | active: the app's controller path (keyboard, SDL gamepads, GameCube adapter over IOKit, rumble) |
| Adapter polling-rate readout | host + renderer | active: the app's controller path (keyboard, SDL gamepads, GameCube adapter over IOKit, rumble) |
| Xbox, DualShock 4, DualSense, Switch Pro, Joy-Con, generic HID pads | host | not wired |
| Keyboard | host | active: the app's controller path (keyboard, SDL gamepads, GameCube adapter over IOKit, rumble) |
| Remapping | host + renderer | active: the panel's CONTROLS page (the app's remap, and the bindings page of fe/controls.cpp) |
| Controller profiles | host | active: fe/controls.cpp profiles (ControllerProfiles folder), on the CONTROLS page |
| Dolphin profile import | host | active: fe/controls.cpp; reads the app's Dolphin folder, else Slippi Dolphin's or Dolphin's GCPad profiles |
| Deadzone, radius, snapback filter, trigger cap and click point | host | active: fe/controls.cpp's setup page, applied after the app's input (IOKit adapter and SDL pads) |
| Box controllers | host | active: fe/controls.cpp box mode (melee-unlocked's box_melee.h) |
| PhobGCC settings on a stock controller | host + renderer | active: the app's controller path (keyboard, SDL gamepads, GameCube adapter over IOKit, rumble) |
| PhobScope and Phob Check | renderer + host | not wired |
| "Play as P2/P3/P4" | host + renderer | not wired |
| Rumble and its switch | host | active: the app's controller path (keyboard, SDL gamepads, GameCube adapter over IOKit, rumble) |
| Button chords | host | active: Start + D-pad Down/Up + Z (ui/chords_mac.cpp, melee-unlocked's settings_chord.h) |
| Background input | host | not wired |

## 8. Display

| Feature | Side | Mac status |
|---|---|---|
| Picture from the GX stream | renderer | active: the app's GX decoder and Metal renderer |
| Aspect ratio | renderer | active: settings.ini aspect (the picture's shape) and the widescreen code |
| V-Sync | renderer | active: the launcher and the panel |
| Quality presets | renderer | not wired |
| "Low spec" switch | renderer | not wired |
| Internal resolution | renderer | active: the app's Metal renderer and launcher settings |
| Anti-aliasing | renderer | not wired |
| Anisotropic filtering | renderer | active: the panel's texture filtering row |
| Sharpening | renderer | active: settings.ini sharpness (the present pass's sharpening) |
| Brightness, contrast, vibrance | renderer | active: settings.ini brightness / contrast / vibrance, in the Metal present pass; off by default |
| Screen-space ambient occlusion | renderer | not wired |
| Window size, borderless and exclusive fullscreen | renderer | active: the app's Metal renderer and launcher settings |
| Graphics backend picker | renderer | not wired |
| DLSS, DLAA, XeSS, FSR upscaling | renderer | not wired |
| Frame generation | renderer | not wired |
| NVIDIA Reflex and latency readout | renderer | not wired |
| HDR output | renderer | not wired |
| Path tracing, RTX Pack, RTX Remix, Ray Reconstruction | renderer + external | not wired |
| DLSS 5 neural rendering | renderer + external | not wired |
| Shader pipeline precompile | renderer | not wired |

## 9. Audio

| Feature | Side | Mac status |
|---|---|---|
| Audio mixing | game image + host | active: the app's audio output (32 kHz, volume from the launcher) |
| Sound output | host | active: the app's audio output (32 kHz, volume from the launcher) |
| Master volume | host | active: the app's audio output (32 kHz, volume from the launcher) |
| Audio buffer | host | not wired |
| Speed nudging for online time sync | host | not wired |
| Mono / Stereo | game image + host | in the engine |
| Audio mode: shared, low latency, exclusive, ASIO; output device picker | host | active: the app's audio output (32 kHz, volume from the launcher) |
| Menu sounds in the settings UI | renderer | not wired |

## 10. Saves

| Feature | Side | Mac status |
|---|---|---|
| Memory card as a folder of .gci files | host | active: memory card files in ~/Library/Application Support/Dashdance/melee-unlocked/Card A |
| Automatic save backups | host | active: memory card files in ~/Library/Application Support/Dashdance/melee-unlocked/Card A |
| Unlock everything without touching the save | game image | active: memory card files in ~/Library/Application Support/Dashdance/melee-unlocked/Card A |
| Save import for mods | host | active: memory card files in ~/Library/Application Support/Dashdance/melee-unlocked/Card A |

## 11. In-game UI, overlays, accessibility, diagnostics

| Feature | Side | Mac status |
|---|---|---|
| PC settings overlay | renderer | active: melee-unlocked's own panel code (VIDEO, AUDIO, GAME, CONTROLS, OVERLAYS, CUSTOMIZE, GECKO CODES, MODS) drawn with Dear ImGui on the Metal renderer; L + R + Start, F1 or Escape opens it; controller and keyboard navigation (`mu-engine/app/ui`) |
| Settings appearances | renderer | partial: the Clean side style with melee-unlocked's palettes and accents; the other menu styles are not ported |
| Settings search | renderer | active: melee-unlocked's search code in the panel |
| Settings file | host | active: what the panel changes is saved to launcher.ini for the next start |
| Pause menu on Esc | renderer | active: Escape opens the settings panel |
| "Settings: F1" reminder | renderer | not wired |
| Frame counter and performance graph | renderer | not wired |
| Player nicknames above fighters | renderer | not wired |
| Controller input overlay | renderer | not wired |
| Note when skins are off online | renderer | not wired |
| Accessibility-type options | see sections 4, 8 | not wired |
| Text size or colour-blind setting | n/a | not wired |
| Logs | host | not wired |
| Trace-mark keys | host | not wired |
| Frame capture burst | renderer | not wired |
| One-frame glitch detector | renderer | not wired |
| Frame time CSV, simulation time CSV, latency trace | host / renderer | not wired |
| Scripted runs | host | not wired |
| Benchmark mode | n/a | not wired |

## 12. Launcher and desktop

| Feature | Side | Mac status |
|---|---|---|
| Play page | launcher | not wired |
| Engine picker | launcher | not wired |
| CPU build auto-pick | launcher | not wired (left out: the Mac app is built from source) |
| Disc check | launcher + host | not wired |
| RVZ and WIA conversion | launcher | not wired (left out: the Mac app is built from source) |
| CISO, GCZ | launcher | not wired |
| Pipeline precompile page | launcher | not wired |
| Update check and one-click update | launcher + host + external | not wired (left out: the Mac app is built from source) |
| Version rollback | launcher | not wired (left out: the Mac app is built from source) |
| Optional part downloads | launcher + host + external | not wired |
| Lab view and Pro AI asset downloads | host + external | not wired |
| "What's new" | launcher | not wired |
| Launcher languages | launcher | not wired |
| Launcher colour and rail picture | launcher | active: launcher colour wheel |
| Mods page | launcher | not wired: Mods page has the folder; the catalog installer comes with the shared library |
| Mods catalog | launcher + external | not wired: Mods page has the folder; the catalog installer comes with the shared library |
| Melee 3D | launcher | not wired |
| Replay library | launcher | partial: Replay Viewer lists games; Watch uses the classic engine's playback build |
| Replay stats | launcher | partial: Replay Viewer lists games; Watch uses the classic engine's playback build |
| Watch live | launcher | not wired |
| Multiplayer Lobby | launcher + external | not wired |
| Hosted lobby service | launcher | not wired |
| Slippi account line | launcher | not wired |
| YouGame connect | launcher + external | out of scope |
| Crash report upload | launcher + external | not wired |
| Settings window from the launcher | launcher | not wired |
| Standalone double-click start | host | not wired |

## Additions from the full audit of melee-unlocked 1.0.5

Items the audit of melee-unlocked's source found that the list above does not have (its "MISSING" tables, by audit and
ID). All are not wired on the Mac unless the status says otherwise; the note is the audit's macOS port note.

### audit-hle-abi

| ID | Item | Mac status and port note |
|---|---|---|
| M1 | Connect-code history and autocomplete (Direct and Teams) | active: the netplay object answers 0xBE as Slippi Dolphin does (direct-codes.json, teams-codes.json beside user.json); app_online_menus_test.sh online_direct_keep: the entry offered MOCK#002, Z took it, `matchmaking started (mode 2, code 'MOCK#002')` |
| M2 | Custom quick-chat messages | not wired; easy / easy. FEATURES "Quick chat" only covers message ids |
| M3 | Display name refresh from Slippi's user API | active (mu-engine/patches/net-0004): after sign-in the display name, connect code, latest version and chat lines come from Slippi's user service on a thread of its own; user.json is never written. Loopback test: `profile refreshed from Slippi's user service: Mock 2 (refreshed) (MOCK#002), its 16 chat messages` |
| M4 | Slippi account auto-discovery (desktop) | not wired; not applicable on Switch (sign-in screen) / easy and important on macOS: players already signed in with the Slippi Launcher play without signing in again |
| M5 | Replay upload when Slippi asks, and disc MD5 in the report | active (net-0004): the app's reporter is linked into the engine's netplay object; the report carries the disc MD5 (cached in iso_md5_cache.txt) and the replay is asked for on the reporter's thread when Slippi's reply has uploadUrl. app_online_test.sh against tools/mac/mock_slippi_http.py: `game report ... isoHash 0e63d422...`, `replay upload /upload/1: 1266301 bytes (gzip) ... a Slippi replay` |
| M6 | Same-router fallback addresses | active (net-0002): same public address: the LAN address first, after 2 s the public one too, in the same 8 s. online_loopback_test.sh with `MU_MOCK_ARGS=--dead-lan`: `the same-router (LAN) address has not answered in 2 s; trying the public address too`, connected, net check equal |
| M7 | Play while searching for a Slippi Direct or Teams code | active: Direct and Teams searches are kept like Unranked (search module + net-0001); online_direct_keep run on the Mac (see "Play while you search") |
| M8 | Warm-up sparring partner (sub-item) | wired (net-0001 `remote_character`; fe/features/search.cpp, Fight a pro for the CPU); not run on the Mac (see "Warm-up room") |
| M9 | "Search again?" after an online session (sub-item) | wired (search module, net-0001 `code`); not run on the Mac (see "Training after online") |
| M10 | Kept-search question on leaving the select (sub-item) | active: settings.ini `keep_search` 0 ask, 1 always, 2 never; online_keep_ask run on the Mac (B, the question, A, found, back, match) |
| M11 | Auto delay from a pre-match ping | not wired; easy / easy |
| M12 | Opponent's automatic L-cancel announced | not wired; easy / easy |
| M13 | Keyboard key names in prompts | not wired; not applicable on Switch (no keyboard on the console) / easy on macOS |
| M14 | Legacy replays (no stored code list) | not wired; easy / easy (map the legacy list to `MU_REPLAY_CODE_*`; the static engine serves bytes) |
| M15 | Slippi 3.7.0 rank, MMR and bot flags in replays | not wired; easy / easy |
| M16 | Player drop read by own frames (sub-item) | not wired; easy / easy |
| M17 | Slow first frame is not "gone" | active (net-0003): 30 s before the other player's first online frame, Slippi's 7 s after. online_loopback_test.sh with `MU_B_ENV=MELEE_NET_TEST_SLOW_FIRST_FRAME_MS=12000`: side a waited 714 retraces (12 s) at frame 8, no drop, net check equal |
| M18 | Network condition simulator (developer) | not wired; easy / easy (FEATURES only lists this repo's `--net-clock-skew`) |
| M19 | Slippi command trace (developer) | not wired; easy / easy |
| M20 | Smoother time sync (experimental) | not wired; easy / easy |

### audit-app-host

| ID | Item | Mac status and port note |
|---|---|---|
| M1 | Replays: "Open replay" (browse) | not wired; easy: NSOpenPanel / drag onto the app |
| M2 | Replays: "Compress all replays now (.slpz)" | not wired; easy |
| M3 | Replays: refuse a replay played on a mod stage | not wired; easy |
| M4 | Replays: content fingerprint check | not wired; easy |
| M5 | Replays: delete to Recycle Bin, right-click "Show in Explorer" | not wired; easy: NSWorkspace recycle / reveal in Finder |
| M6 | Mods catalog refreshed online | not wired; easy |
| M7 | Mods: "Choose skins" button and skin count on a mod card | not wired; medium |
| M8 | Mod disc costumes as skins in the normal game | not wired; medium |
| M9 | Skin thumbnails | not wired; easy-medium |
| M10 | Pack skin rule online: look-only costumes stay on | not wired; medium |
| M11 | Stage graft: mod stage meshes on the original stage online | not wired; hard |
| M12 | Alternate stage pick by held buttons | not wired; medium |
| M13 | Keep the display awake during controller-only play | not wired; easy: IOPMAssertion (Dashdance may already do this) |
| M14 | Mouse cursor hides over the game | not wired; easy: NSCursor hide/unhide on idle |
| M15 | Settings-panel controller ownership rules | not wired; easy |
| M16 | Game calendar date from the real clock | not wired; easy |
| M17 | DSP coefficient table, player dump first | not wired; easy |
| M18 | Crash report privacy scrubbing | not wired; easy |
| M19 | Crash zip contents and fallback | not wired; easy |
| M20 | Readable crash text companion | not wired; easy |
| M21 | GPU-driver attribution in crash reports | not wired; easy: name Metal/AGX frames |
| M22 | Crashed-start recovery | not wired; medium: same idea for the Metal pipeline cache |
| M23 | Frame generation off after a Streamline crash | not wired; n/a |
| M24 | Game thread priority and timer resolution | not wired; easy: real-time thread policy (Dashdance has one) |
| M25 | Diagnostic summary lines | not wired; easy |
| M26 | Launcher logs | not wired; easy |
| M27 | Shared disc memory outside the game folder | not wired; easy |
| M28 | Version mismatch warning | not wired; n/a |
| M29 | Engine switch requested by the game | not wired; n/a |
| M30 | Launcher replay choice honoured by direct starts | not wired; easy if a launcher and game are separate |
| M31 | Windows audio session details | not wired; n/a (CoreAudio has no ducking) |
| M32 | Replay viewer mute | not wired; easy |
| M33 | Alt+Enter fullscreen toggle | not wired; easy: Cmd+Ctrl+F / green button |
| M34 | "Reset CPU training options" button | not wired; easy |
| M35 | Lobby state files | not wired; medium |
| M36 | Release start scripts | not wired; n/a (Dashdance has its own playback app) |
| M37 | Source build from the launcher's Build tab | not wired; n/a |
| M38 | Environment variables not in FEATURES | not wired; n/a |
| M39 | Release versioning rule | not wired; easy, if Dashdance compares versions |
| M40 | Mods: GitHub API fallback, cancel, progress, free-space check | partial: the shared installer cancels, reports progress and checks free space; no GitHub API fallback |
| M41 | Lobby: clock-skew warning, "Copy code" fallback | not wired; easy |

### audit-gx-game

| ID | Item | Mac status and port note |
|---|---|---|
| A5 | Opponent auto L-cancel notice | not wired; Switch: easy - text line, but needs MU build message (0xE0) which we do not speak; our own build message 0xD0 could carry it. macOS: same |
| A13 | Input overlay lone-port fallback | not wired; Switch: easy; macOS: easy |
| A22 | Switch Pro picture binder | not wired; Switch: medium - most relevant on Switch (Pro/Joy-Con); art + hit-testing; macOS: medium |
| A23 | Deadzone visualiser | not wired; Switch: easy - overlay draw of raw stick; macOS: easy |
| A40 | Notices in Training | not wired; Switch: easy - text cards over the picture; macOS: easy |
| A41 | Corner hint and intro | not wired; Switch: easy; macOS: easy |
| A47 | Warm-up picture-in-picture window | not wired; Switch: hard - two game pictures at once (second process upstream); macOS: hard |
| A51 | Menu styles | not wired; Switch: hard - large ImGui art port; macOS: hard. Low player value |
| A53 | Radial wheel navigation | not wired; Switch: medium; macOS: medium |
| A56 | "PC Settings" row in Melee's Options | not wired; Switch: medium - game image exposes the row; host must answer and open our menu; macOS: medium |
| A57 | "Restart now" button | not wired; Switch: medium - app relaunch of the game program; macOS: easy |
| A59 | Settings view memory | not wired; Switch: easy; macOS: easy |
| A60 | UI scale | not wired; Switch: easy (720/1080 fixed); macOS: easy (Retina backing scale) |
| A62 | 20XX TE pair link | not wired; Switch: easy - two bit syncs in settings code; macOS: easy |
| A66 | Advanced groups | not wired; Switch: easy; macOS: easy |
| A67 | Engine-only and experimental rules | not wired; Switch: easy (n/a mostly: one engine); macOS: easy |
| A71 | `widescreen` key | not wired; Switch: easy - doc fix |
| A72 | Retired keys | not wired; doc only |
| B7 | Restart game from the panel | not wired; Switch: easy - relaunch the app's game process or re-enter play mode from the in-game menu; macOS: easy - Dashdance already restarts the game loop from its dashboard |
| B33b | Arcade preset / Reset buttons | not wired; Switch: easy - menu row setting five existing keys; macOS: easy - same in the dashboard/in-game menu |
| B62 | Audio latency and underrun read-out | not wired; Switch: easy - audout queue depth and an underrun counter from the app's audio path; macOS: easy - AVAudioEngine/CoreAudio IO buffer and dropout count |
| B71 | Per-mod-disc random stage-skin pool | not wired; Switch: medium - needs the stage-skin store (FEATURES: imports not wired) first; macOS: medium - same store, then a list row |
| B75 | Costume packs: apply a whole set | not wired; Switch: medium - depends on the cosmetic store; UI is a list; macOS: medium - same |
| B78 | Fighter move-sound replacement | not wired; Switch: medium - SSM bank swap through the disc overlay, after imports exist; macOS: medium - same |
| B111 | Press a button to pick the controller to edit; duplicate-pad detection | not wired; Switch: easy - libnx HID gives per-pad states; duplicates rarely occur (no Steam); macOS: easy - GameController framework, and duplicates do occur (Steam Input) |
| B113 | Driver-conflict notices | not wired; Switch: n/a - no competing drivers; macOS: easy - report IOKit exclusive-access failure on the adapter |
| B115 | Reset GameCube adapter | not wired; Switch: easy - close and reopen the usb:hs interface, resend 0x13; macOS: easy - re-open the IOKit interface (Dashdance's gc_adapter_iokit.cpp) |
| C2 | Four looks: "Main Menu bars", "Side rails", "Character select slots", "HUD strip" (+ "Window (mouse and keyboard)") | not wired; Switch: medium - pure 2D drawing, needs gradient quads and the game font; macOS: medium - same |
| C3 | Melee's own font from main.dol | not wired; Switch: easy - read dol data, upload one 1024x288 RGBA texture; macOS: easy - same into the glyph atlas |
| C4 | GameCube button glyph legends | not wired; Switch: easy; macOS: easy |
| C6 | On-screen keyboard | not wired; Switch: easy - or use libnx swkbd; macOS: easy - native text field or same grid |
| C8 | Training corner hint | not wired; Switch: easy; macOS: easy |
| C10 | "CPU now" / plan readout | not wired; Switch: easy - practice_cpu::status() is host code we already build?; macOS: easy |
| C40 | Five looks | not wired; as C39 |
| C41 | Arena button mode | not wired; as C39 |
| C42 | Hand into the panel | not wired; as C39 |
| C43 | Room modes | not wired; as C39 |
| C44 | Ask to play next / give up spot prompt | not wired; as C39 |
| C45 | Doubles vote | not wired; as C39 |
| C46 | Add a friend's code | not wired; as C39 |
| C47 | Roster details | not wired; as C39 |
| C66 | Skip the 3D scene when covered | not wired; Switch: easy; macOS: easy |
| C71 | COMING SOON! stickers | not wired; skip |
| D17 | Lightweight drawn/skipped counter | not wired; Switch: easy - two counters; macOS: easy |
| D38 | Bundled CRT preset pack | not wired; Switch: hard - as D37; macOS: medium - ship slang-shaders (licence: MPL-2.0 librashader, presets' own licences) |
| D39 | Missing-file diagnosis | not wired; Switch: easy once D36 exists; macOS: easy |
| D40 | YouGame data folder for presets | not wired; skip (YouGame) |
| D48 | "+ Add" opens TexturePacks | not wired; Switch: n/a (no file browser; SD card path documented); macOS: easy - NSWorkspace reveal |
| D49 | Legacy-name pack report | not wired; Switch: easy; macOS: easy |
| D53 | Sub-frames skipped at one present per tick | not wired; Switch: easy - the console is 60 Hz, so this is the default case there; macOS: easy |
| D55 | Variable refresh detection and cap | not wired; Switch: n/a - fixed 60 Hz panel/dock; macOS: medium - ProMotion/adaptive-sync displays via CAMetalDisplayLink or `preferredFrameRateRange`; detection by the OS, not by counts |
| D56 | Scanline sync | not wired; Switch: n/a - the compositor already presents per vblank; macOS: medium - CVDisplayLink/CAMetalDisplayLink timing |
| D57 | Presenter latency trims | not wired; Switch: medium - submit-cancel needs Aurora to poll a flag between draws; spin wait unsuitable on 4 A57 cores (upstream skips it under 8 cores); macOS: medium |
| D58 | Uneven-cap warning | not wired; Switch: n/a; macOS: easy |
| D66 | Developer presentation switches | not wired; Switch: easy - only if the solver is taken (checklist c); macOS: easy |
| D76 | Test env hooks | not wired; Switch: easy where relevant; macOS: easy |
| D79 | Old shader caches pruned | not wired; Switch: easy; macOS: easy |
| D80 | Dedicated GPU on laptops | not wired; Switch: n/a; macOS: easy - pick the discrete MTLDevice on dual-GPU MacBook Pros (or opt in to integrated for battery) |
| D82 | Vulkan developer switches | not wired; Switch: n/a (Aurora's own); macOS: easy |
| E95 | Title-demo stage fallback (cmd 0xFC skin growth) | not wired; Switch: easy - answer 0xFC with the skin growth once a skin store exists (0 today, so harmless); macOS: easy - same |
| E99 | Keyboard key names in prompts (`mu_host_button_label(port, mask)`) | not wired; Switch: easy/n-a - no keyboard on console; return 0 keeps GameCube names; macOS: easy - map the keyboard layout's binding to a label |
| E100 | Card boot auto-answers (`card_boot_confirm`, host API >= 22) | not wired; Switch: n/a now; macOS: n/a now |
| F22 | Keyboard key names in prompts (host answers 0xDF from live key bindings: "Hold E to disconnect") | not wired; Switch: easy (answer nothing: GameCube names stay; no keyboard). macOS: easy, a lookup in our key bindings. |
| F45 | Develop-mode physics display ("Player N X Y", sticks, SelfVel, KBVel, Pos, ECB, State) | not wired; Switch: easy (game text); macOS: easy. |
| F80 | YouGame page pause (freeze scene from the app, query pause state) | not wired; Switch/macOS: skip. |
| F82 | "Coming soon" beside YouGame rows (game text) | not wired; skip |
| F93 | `melee_game supports Windows x64 and native Linux x86-64` fatal error | not wired; Switch: medium, patch the check (aarch64 Horizon toolchain). macOS: medium, patch the check and Mach-O specifics. |
| F94 | `MELEE_CPU_BASELINE` AVX2 default; TM-CE objects get `-mavx2 -mfma` | not wired; Switch: easy, set SSE2/none path or guard on MU_AARCH64; macOS: easy (arm64). |
| F95 | `MU_AARCH64` auto-detect from CMAKE_SYSTEM_PROCESSOR | not wired; Helps both: easy. |
| F96 | Console-compatible image window: linker script keeps game addresses low (32-bit slots, `mu_addr32_failed` panic), ELF checks, LP64 long overlay | not wired; Switch: hard, NRO/NSO loader and ASLR must place the image and arena in low 4 GB (or this repo's existing approach). macOS: hard, Mach-O has no linker script; needs -segaddr / pagezero tricks or a custom loader. |
| F97 | `YOUGAME_RANKED_BRANCH` ON, `YOUGAME_AGENT_TEST_HOOK` OFF, `MU_MATH_AUDIT` OFF, `MU_UI_PREVIEWS` (MELEE_TEST_UI) | not wired; Switch/macOS: set ranked OFF; easy. |
| G28 | Slippi "DynamicsFix" (Required) | not wired; Switch: none (in the image); macOS recomp: easy (Dashdance's Slippi code set already applies C209E090) |
| G29 | Slippi "NanaDeterminism" | not wired; as G28 |
| G63 | KO stars counted when the results screen is skipped | not wired; Switch: none (in the image); macOS recomp: easy (Slippi's "CSS KO Stars") |
| G64 | Fresh card folder: save created without prompts; YouGame card confirm | not wired; Switch: none (in the image); macOS recomp: easy (an HLE card answer) |
| G103 | Experimental deferred native sound | not wired; Switch/macOS: skip (an upstream experiment) |

