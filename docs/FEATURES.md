# melee-unlocked features on the Mac

Every row of the melee-unlocked feature list (the same list the Switch build keeps, from melee-unlocked 1.0.5), with its
status in melee-unlocked-macOS. The exactness gate comes first: the five reference scripts give digests identical to
melee-unlocked's x86-64 references, in the headless runner and in the app with picture and sound.

| Mac status | Meaning |
|---|---|
| active | On in the Mac app with the melee-unlocked engine. |
| in the engine | Part of the game image and on by default; the Mac app has no setting for it yet (the melee-unlocked settings panel is not ported). |
| engine option, no setting | In the game image behind an option word; the Mac app does not set it yet. |
| not wired | Exists upstream; the Mac host, renderer or launcher does not provide it yet. |
| out of scope | YouGame. |


## 2. Frame-rate unlock

| Feature | Side | Mac status |
|---|---|---|
| Sub-frame animation: Off | renderer | not wired |
| Sub-frame animation: Predict ahead | game image + host + renderer | engine option, no setting |
| Sub-frame animation: Interpolate | same | not wired |
| Generic matrix modes | renderer | not wired |
| Frame cap | renderer | not wired |
| Sub-frame pause when the game falls behind online | host | not wired |

## 3. Online

| Feature | Side | Mac status |
|---|---|---|
| Slippi Unranked, Direct, Teams, Party | game image + host + external | not wired: online play is the next milestone (netplay host as on the Switch) |
| Slippi system files | host | not wired: online play is the next milestone (netplay host as on the Switch) |
| Slippi Ranked | host | refused, as melee-unlocked refuses it |
| Rollback savestates | host | not wired: online play is the next milestone (netplay host as on the Switch) |
| Slippi account | host | not wired: online play is the next milestone (netplay host as on the Switch) |
| Matchmaking | host + external | not wired: online play is the next milestone (netplay host as on the Switch) |
| Online input delay | host | not wired: online play is the next milestone (netplay host as on the Switch) |
| Time sync, frame advance/skip | host | not wired: online play is the next milestone (netplay host as on the Switch) |
| Desync detection and notice | game image + host | not wired: online play is the next milestone (netplay host as on the Switch) |
| Player drop handling (3.7.0) | game image + host | not wired: online play is the next milestone (netplay host as on the Switch) |
| Slippi input suggestions (3.7.0) | host | not wired: online play is the next milestone (netplay host as on the Switch) |
| Version gate | host | not wired: online play is the next milestone (netplay host as on the Switch) |
| Replay recording (.slp) | game image + host | not wired: online play is the next milestone (netplay host as on the Switch) |
| Compressed replays (.slpz) | host | not wired: online play is the next milestone (netplay host as on the Switch) |
| Replay playback | game image + host | not wired: online play is the next milestone (netplay host as on the Switch) |
| Replay viewer controls | host + renderer | not wired: online play is the next milestone (netplay host as on the Switch) |
| Replay takeover | game image + host | not wired: online play is the next milestone (netplay host as on the Switch) |
| Replay coach tips | game image + host + renderer | not wired: online play is the next milestone (netplay host as on the Switch) |
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
| Play while you search | host + game image + renderer | not wired: online play is the next milestone (netplay host as on the Switch) |
| Warm-up room | host | not wired: online play is the next milestone (netplay host as on the Switch) |
| Training after online | host | not wired: online play is the next milestone (netplay host as on the Switch) |
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
| Unlock everything | game image | engine option, no setting |
| Show results screen | game image | engine option, no setting |
| Freeze Pokemon Stadium | game image | engine option, no setting |
| Freeze Final Destination | game image | engine option, no setting |
| Fountain of Dreams reflections | game image | engine option, no setting |
| Full FoD reflections | game image | engine option, no setting |
| Widescreen 16:9 (Slippi code) | game image | engine option, no setting |
| True widescreen | game image + renderer | engine option, no setting |
| Disable screen shake | game image | engine option, no setting |
| Screen shake strength | game image | engine option, no setting |
| PAL stock icons | game image | engine option, no setting |
| Stock icon and damage number size | game image | engine option, no setting |
| Always show player tags | game image | engine option, no setting |
| Low poly fighters | game image | engine option, no setting |
| Hide KO explosions | game image | engine option, no setting |
| Hide emblems behind percents | game image | engine option, no setting |
| Training HUD on/off | game image | engine option, no setting |
| Character select background plays through | game image | engine option, no setting |
| Game language | game image | engine option, no setting |
| Shield colours per port | game image + host | engine option, no setting |
| Community codes: HUD transparency, Nana's percent, ally indicators, respawn platform by stocks, hide GO!, V-cancel sound, hide fighter shadows, centre the 2P HUD, hitbox colours by id, priority order, knockback angles | game image (+ renderer for Nana and ally overlays via `hud_extras`) | engine option, no setting |
| Offline codes: Instant Melee, unrestricted pause camera, Boot to CSS | game image | engine option, no setting |
| Zelda starts as Sheik | game image + host | engine option, no setting |
| Offline input delay | host | not wired |
| Mute character voices | game image | engine option, no setting |
| Faster sound effects | game image | engine option, no setting |
| Auto L-cancel | host | not wired |
| L-cancel flash (MU missed / success / both) | host + renderer | not wired |
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
| 20XX CPUs | game image | engine option, no setting |
| CPU training options | game image | engine option, no setting |
| Vanilla-only session | host | not wired |
| Code-free retail game | game image | engine option, no setting |

## 5. Training tools

| Feature | Side | Mac status |
|---|---|---|
| Practice menu | renderer + host | not wired |
| Practice CPU | game image + host | engine option, no setting |
| Save states in Training | game image + host | engine option, no setting |
| Record and play back the CPU | game image | engine option, no setting |
| Collision bubbles | game image | engine option, no setting |
| Frame data and input overlay | renderer + host | not wired |
| Lab dummy DI, after-hitstun reaction, tech choice | game image | engine option, no setting |
| Frame advance | host + game image | engine option, no setting |
| Fixed or frozen random seed | host + game image | engine option, no setting |
| Fight a pro (Phillip) | host + game image + external | engine option, no setting |
| Practice ghost AI | host | not wired |
| Pro habit tables | host | not wired |
| Training Mode CE events | game image + host | engine option, no setting |
| Training Mode CE on-screen displays | game image | engine option, no setting |
| TM-CE console global OSD shortcut | n/a | not wired |
| Practice from replays | game image + host + renderer | engine option, no setting |
| Lossless adapter lag test | game image + host | engine option, no setting |
| Display latency test | renderer | not wired |

## 6. Mods, added content, cosmetics, music

| Feature | Side | Mac status |
|---|---|---|
| Mod profiles and disc-file overlay | host | not wired |
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
| Per-profile and vanilla-only memory cards | host | not wired |
| Costume, stage and effect imports | host + renderer | not wired |
| Skin cycling on the select screens | game image + host | engine option, no setting |
| Voice mods | host + game image | engine option, no setting |
| Texture packs | renderer | not wired |
| Game music | game image | engine option, no setting |
| Music volume | host / game image | engine option, no setting |
| Custom music packs / jukebox | host | not wired |
| Look presets | renderer | not wired |
| Lightweight levels and visual effects tiers | renderer (+ game image for low poly) | engine option, no setting |
| "Get mod looks" | launcher / external | not wired |
| Arcade look: attack and projectile hitboxes | game image | engine option, no setting |
| Arcade look: grid or plain background | game image + renderer | engine option, no setting |
| Arcade look: 2D camera | game image offline; renderer online | engine option, no setting |
| Arcade look: combo counter | renderer | not wired |
| 2D-style m-ex build | game image | engine option, no setting |
| Lab view | renderer | not wired |
| Looping video backgrounds | renderer | not wired |
| Border art | renderer | not wired |
| Built-in CRT look | renderer | not wired |
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
| Remapping | host + renderer | not wired |
| Controller profiles | host | active: the app's controller path (keyboard, SDL gamepads, GameCube adapter over IOKit, rumble) |
| Dolphin profile import | host | not wired |
| Deadzone, radius, snapback filter, trigger cap and click point | host | not wired |
| Box controllers | host | active: the app's controller path (keyboard, SDL gamepads, GameCube adapter over IOKit, rumble) |
| PhobGCC settings on a stock controller | host + renderer | active: the app's controller path (keyboard, SDL gamepads, GameCube adapter over IOKit, rumble) |
| PhobScope and Phob Check | renderer + host | not wired |
| "Play as P2/P3/P4" | host + renderer | not wired |
| Rumble and its switch | host | active: the app's controller path (keyboard, SDL gamepads, GameCube adapter over IOKit, rumble) |
| Button chords | host | not wired |
| Background input | host | not wired |

## 8. Display

| Feature | Side | Mac status |
|---|---|---|
| Picture from the GX stream | renderer | not wired |
| Aspect ratio | renderer | not wired |
| V-Sync | renderer | not wired |
| Quality presets | renderer | not wired |
| "Low spec" switch | renderer | not wired |
| Internal resolution | renderer | active: the app's Metal renderer and launcher settings |
| Anti-aliasing | renderer | not wired |
| Anisotropic filtering | renderer | not wired |
| Sharpening | renderer | not wired |
| Brightness, contrast, vibrance | renderer | not wired |
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
| PC settings overlay | renderer | not wired |
| Settings appearances | renderer | not wired |
| Settings search | renderer | not wired |
| Settings file | host | not wired |
| Pause menu on Esc | renderer | not wired |
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
