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
| Sub-frame pause when the game falls behind online | host | partial (as on the Switch): fe/features/present pauses sub-frames online when 90% of the presented frames of the last 3 s overran a refresh and resumes below 50%, `present: sub-frames paused (online, ...)`; not upstream's late-frame count. Not run online on the Mac |

## 3. Online

| Feature | Side | Mac status |
|---|---|---|
| Slippi Unranked, Direct, Teams, Party | game image + host + external | active: from the game's Online Play page with the player's Slippi sign-in, on Slippi 3.7.0's netplay (Melee-NX-NetPlay's, mu-engine/net: the WebSocket queue at matchmaking.slippi.gg, STUN, 3.7.0's wire format). Tested on loopback only against tools/mac/mock_matchmaking_ws.py under the sandbox profile: two engines from the menus (Unranked, Direct by a remembered code), two runners (Unranked, same-router fallback, slow first frame, rollbacks through a delay relay): net checks equal, 0 desync messages. Not yet played against a real opponent on Slippi's service from the Mac |
| Slippi system files | host | active: Slippi's menu files served from the bundle's slippi_sys (`mu-engine/app/slippi_menus.cpp`) |
| Slippi Ranked | host | refused, as melee-unlocked refuses it |
| Rollback savestates | host | active: the hot-page store with macOS write tracking (580 rollbacks, net check equal, in the loopback rollback test) |
| Slippi account | host | active: the app's own sign-in file, or the Slippi Launcher's when the app has none |
| Matchmaking | host + external | active: the app's matchmaking client (ENet); tested against the loopback stand-in only |
| Online input delay | host | active: the launcher's setting (1-4, or the in-game menu's larger value) is passed to the engine |
| Time sync, frame advance/skip | host | active (the app's netplay; frame-rate nudges go to the host's pacing) |
| Desync detection and notice | game image + host | active: the game's checksum comparison (0 desync messages in the tests) |
| Player drop handling (3.7.0) | game image + host | partial: Slippi 3.7.0 despawn frames in the ONLINE_INPUTS reply; not tested with a real drop |
| Slippi input suggestions (3.7.0) | host | active: sent and used (net-0001). The netplay object's sampler thread reads the local controller about a thousand times a second through the app's input path (mu_play_main.cpp suggest_read: adapter, SDL gamepads, keyboard, remap and controller setup on their own filter lane; nothing while the settings panel holds the controllers), never at the same time as the game's poll; settings.ini `input_suggestions` (on by default, as upstream). Loopback test from the game's menus with the scripts playing (`MU_SCRIPT_PLAYS=1 MU_SUGGEST_LEAD_MS=8 app_online_menus_test.sh`): `input suggestions: sent and used`, 6 and 4 sent, the other side `6 replies predicted from one`, net check equal, 0 desync messages. Not yet played against a real opponent |
| Version gate | host | active: the engine's netplay reports Slippi 3.7.0 (Melee-NX-NetPlay's 3.7.0 netplay: the WebSocket queue, appVersion 3.7.0 in the join) and says so when Slippi names a newer release; loopback queue stand-in: `join ... appVersion 3.7.0`. The legacy engine (port/runtime/hle) is still 3.6.4, which Slippi's service now refuses |
| Replay recording (.slp) | game image + host | active: every match, offline and online (`mu-engine/app/replays.cpp`) |
| Compressed replays (.slpz) | host | active: settings.ini compressreplays 1 (melee-unlocked's zstd) |
| Replay playback | game image + host | active: `--replay` and the launcher's Watch, through the replays module (`fe/features/replays.cpp`); a replay without a code list plays as Vanilla. Recordings with codes (online, offline play mode) and vanilla recordings started from the menus play back with 0 differing post-frame records (online loopback games: 6844 and 13120 equal; vanilla play-mode recordings of parity_vs_onett and vs4_busy: 14648 and 29296 equal, after mu-engine/patches/0014, docs/MAC_FIXES.md #10). Recordings made by the legacy engine (Dashdance) are not exact: one from 2026-10-09 differs from frame 2336 on |
| Replay viewer controls | host + renderer | active: Start/Z pause, D-pad seek and step, L speed, R fast forward, X/Y tips (the replays module, drawn by `fe/overlay_draw.cpp`) |
| Replay takeover | game image + host | active: A takes over (the replays module) |
| Replay coach tips | game image + host + renderer | active: melee-unlocked's coach (replay_coach.cpp from the fetched tree) through the replays module |
| Game reporting | host + external | active: game reports with the disc MD5 and the replay upload when Slippi asks (app_online_test.sh against tools/mac/mock_slippi_http.py: `game report ... isoHash 0e63d422...`, `replay upload ... a Slippi replay`); match status reports for Ranked are not used (Ranked is refused) |
| Quick chat | game image + host | active: Slippi's message ids both ways, settings.ini quick chat 0/1/2 (`set_chat`), custom chat lines from the profile refresh; not run on the Mac with a real opponent |
| Stage pool and random stage | game image + host | active: the netplay object's stage pool from the queue's stage list (Slippi 3.7.0 rules); loopback matches picked from it |
| Pause in Direct and Friends | host | active: Slippi's rule (Start pauses Direct); between two Melee-NX-NetPlay-family builds the build message can turn it off (settings.ini, net-0004) |
| Skip in Unranked | host | wired: settings.ini blockcode1..3 go to the netplay object's skip list (net-0004); not run on the Mac |
| Early inputs ("subframe rollback") | host + game image | active: see "Slippi input suggestions (3.7.0)": sent and received |
| Early rollback from early pads | host | active: received suggestions are used as Slippi 3.7.0 uses them, and this player's are sent (net-0001) |
| Build message and setup hash | host | active between Melee-NX-NetPlay-family builds (net-0004, message 0xD0): loopback log `opponent 1 is a Melee-NX-NetPlay build: Direct pause on, shield colour none, port colours -1/-1`; ignored by Slippi Dolphin. No setup hash |
| Opponent shield colour | game image + host | wired (net-0004, between our builds): settings.ini online shield colour; not run on the Mac |
| Online port colours | game image + host | wired (net-0004, between our builds); not run on the Mac |
| Live pick previews | host + game image | wired (net-0005, command 0xED, between our builds); not run on the Mac |
| Opponent origin label | host | not applicable: the label is for YouGame's Both search (YouGame is out of scope) |
| Play while you search | host + game image + renderer | active for Unranked, Direct and Teams: the Switch's search module (fe/features/search.cpp) on the netplay object's kept search (mu-engine/net: Melee-NX-NetPlay's net-0005/0010). L+R held on Slippi's select with a search running and the fighter locked in: Training while the search goes on; B asks "Keep searching while you play?" (settings.ini `keep_search`); found: back to the select and the match. `mu-engine/tools/app_online_menus_test.sh` from the game's menus on loopback: online_keep_search (Unranked, Training), online_keep_ask (the question, A), online_direct_keep (Direct by a remembered code): each found, back on the select locked in, match played, net check equal, 0 desync messages. Re-run on Slippi 3.7.0's netplay (mu-engine/net): online_keep_search equal; online_direct_keep equal on the second run (the first time the game did not reach the main menu on the way back: "the select never came back"; 1 of 2) |
| Warm-up room | host | wired, not shown on the Mac: the same search module and kept connection; the sparring partner reads the opponent's locked-in pick (net-0010). The Switch's test (`app_online_menus_test.sh` with `MU_ARGS_A="--feature-test unranked-as-direct"` and online_warmup_a/b) ran on the Mac with net check equal, but its first match did not end before the script's L+R, so the warm-up itself was not reached |
| Training after online | host | wired, not run on the Mac: settings.ini `online_training_after` / `online_return_training` and "Search again?" (search module, net-0010 `code`); the Switch's test needs `--feature-test unranked-as-direct` |
| Arena rooms and invites | host + renderer + game image | not wired: only between melee-unlocked peers (its own messages and a second viewer process); not started on the Switch either |
| Slippi spectator server | host | wired: fe/features/spectator.cpp from the Switch (settings.ini `slippi_spectator 1`, off by default); not run on the Mac |
| Ping readout | renderer | wired: the HUD and the network overlay read the bridge's ping (the round trip from the game's pad acknowledgements); not run on the Mac |
| Network and timing overlay | renderer + host | wired: fe/features/netoverlay.cpp (settings.ini `netoverlay 1`) on the bridge's frame trace (time sync waits, halts, offset) and the rollback store; not run on the Mac |
| Session trace file | host | active: `Game_*.trace` beside an online match's replay when the match had a mark or a long wait (settings.ini `session_trace`, upstream's CSV); the menus test logs `session trace of ... not kept (no mark, no long wait)` |
| Agreed Gecko codes online | host | not wired (an upstream stub) |
| YouGame online: sign-in | host + renderer + external | out of scope |
| YouGame Ranked | game image + host + external | out of scope |
| YouGame Unranked, with "Both" search | game image + host + external | out of scope |
| YouGame Friends | game image + host + external | out of scope |
| Friends live handoff | host | out of scope (YouGame Friends) |
| YouGame Doubles | game image + host + external | out of scope |
| Local partner in Doubles | host + game image | not wired (the Switch build leaves it out too) |
| Squad Strike | game image + host | partial: offline VS only, as a setting, off by default (fe/settings.cpp from the Switch); not online |
| YouGame tournament / CPU bracket | game image + host + external | out of scope |
| Console link | game image + host + external | not wired |
| YouGame relay and UDP transports | host + external | out of scope |
| YouGame version pin | host + launcher | out of scope |
| mu_net P2P Direct | host + game image | not wired: melee-unlocked's own peer-to-peer Direct (between its builds); Slippi Direct is active |
| Discord Rich Presence and Join | host | partial: the app's Discord presence (port/runtime/hle/discord_rpc.cpp, the Discord desktop app's local socket) is in the engine's netplay object: menus, searching, opponent, stage, characters and stocks from the replay events; launcher.ini `discord` (on by default) and `discord_rank`; never for a test's made-up user. Not looked at with Discord running. No Join button |

## 4. Gameplay options and code packs

| Feature | Side | Mac status |
|---|---|---|
| Slippi General Codes + Lagless FoD | game image | in the engine |
| UCF 0.84 | game image | in the engine: part of the General Codes, always on (as upstream and on the Switch) |
| UCF 0.8 | game image | wired with replay playback: a replay whose code list carries UCF 0.8 gets MU_REPLAY_CODE_UCF08 (replays.cpp, as on the Switch); no UCF 0.8 replay tried |
| Unlock everything | game image | active: settings.ini (melee-unlocked's key, `mu-engine/app/fe/settings.cpp`), with a row in the settings panel where the Switch has one |
| Show results screen | game image | active: settings.ini (melee-unlocked's key, `mu-engine/app/fe/settings.cpp`), with a row in the settings panel where the Switch has one |
| Freeze Pokemon Stadium | game image | active: settings.ini (melee-unlocked's key, `mu-engine/app/fe/settings.cpp`), with a row in the settings panel where the Switch has one |
| Freeze Final Destination | game image | active: settings.ini (melee-unlocked's key, `mu-engine/app/fe/settings.cpp`), with a row in the settings panel where the Switch has one |
| Fountain of Dreams reflections | game image | active: settings.ini (melee-unlocked's key, `mu-engine/app/fe/settings.cpp`), with a row in the settings panel where the Switch has one |
| Full FoD reflections | game image | active: settings.ini (melee-unlocked's key, `mu-engine/app/fe/settings.cpp`), with a row in the settings panel where the Switch has one |
| Widescreen 16:9 (Slippi code) | game image | active: settings.ini (melee-unlocked's key, `mu-engine/app/fe/settings.cpp`), with a row in the settings panel where the Switch has one |
| True widescreen | game image + renderer | active: settings.ini `truewidescreen 1` (melee-unlocked's key): the Metal renderer widens the world cameras' projections by 219/320 and the P1/P2 tag layer with them, not the render-to-texture views, flashes, pinned layers, frustum backdrops or the 2D layers (melee-unlocked's build_projection rules, mu-engine/app/gx_looks_mac.cpp); the picture fills the window as with the 16:9 code. Replay screenshots in build/looks (crystal-ports, wireframe, potato, 64-style, widescreen) from a recorded game played back with each setting |
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
| Auto L-cancel | host | active offline (the options module, settings.ini `autolcancel`, the panel), and online in a Direct match when every player offered it: each side's `autolcancel 1` goes out in the build message with the Switch's flags (`mu-engine/net/patches/net-0011`, copied from Melee-NX-NetPlay: 0x08 reads the agreement, 0x10 offers it, so Mac and Switch builds agree with each other; `runner/netplay_bridge` `set_auto_lcancel`, `auto_lcancel_agreed`); online the press goes on the controller the local inputs come from, for the local fighter only. Never in Unranked, Teams or Party, or against Slippi Dolphin. `app_online_menus_test.sh ... online_direct_a online_direct_b` with `autolcancel 1` on both sides (`MU_INI_A`/`MU_INI_B`): `opponent ... is a Melee-NX-NetPlay build: ... Auto L-cancel offered`, `automatic L-cancel on in this Direct match: every player offered it`, 5 and 1 presses, net check EQUAL (12 checkpoints), 0 desync messages |
| L-cancel flash (MU missed / success / both) | host + renderer | active: the overlays (`fe/overlay_draw.cpp`), settings.ini. Not ported yet: the Switch's flash on the fighter's model (its frontend/patches/0120 tints Aurora's GX draws; here it needs the same tint in `port/runtime/gx/gx_metal.mm`) and 20XX TE's own flash modes |
| Player-pasted Gecko codes | host + game image | active offline: melee-unlocked's user_gecko.cpp compiled from the fetched tree (`mu-engine/app/fe/patches/0122`, `fe/features/gecko.cpp` from the Switch) with the game image's own `mu_user_gecko_write` / `mu_user_gecko_read` (found by name, `mu-engine/patches/0016`; bound at the first frame, after the host has loaded the image); codes from `melee-unlocked/GeckoCodes.ini` (Dolphin's format), each switched on the panel's GECKO CODES > Your codes (greyed with upstream's reason when it cannot run), kept as upstream's `geckocode NAME` lines and `geckochosen 1`; applied once per frame offline, suspended online, in replays and in a vanilla-only session. Not here: typing or pasting a code in the panel, and upstream's function rewrites ("return N at once" by overwriting the native function's entry), refused by name. `mu-engine/tools/app_gecko_test.sh`: P1's model scale code (044805B0 3FC00000) runs ("1 can run on the Source Port, 1 switched on", "player codes run (offline)") and the game image hands the variable back as 3FC00000 through the match; it also runs as each disc file opens (the host's disc_entrynum, wrapped only while a code is on), because a VS scene start refills the start data and builds the fighters within one frame on the Mac (the disc answers at once): with it P1 is drawn at 1.5 times its size (screenshots build/shots-ui/gecko-on.png against gecko-off.png, the same scripted match without the code); parity_vs_onett stays IDENTICAL; a C2 injection and waxo's No Character Shadows are refused with their reasons |
| Gecko code kinds accepted | host | active (row above): upstream's own compiler and plan decide, unchanged; the "function returns N" kind is refused on the Mac |
| Gecko code kinds refused | host | refused by design, as upstream, with its reasons on each greyed row (`app_gecko_test.sh`: "injects PowerPC code (C2), which this build cannot run"); here also the function rewrites |
| 20XX TE: master switch and Tournament Mode | game image + host | wired offline as on the Switch: the released TE save in a mod profile sets MU_GAME_OPTION_TE (`mod_te_enabled 0` keeps it off; fe/settings.cpp); not tried on the Mac |
| 20XX TE word 1: hold Start to pause, frozen stages, no Star KO, infinite shields, taunt cancel, play after GAME!, fixed camera, CPU Zelda as Sheik, handicap sets stocks | game image | wired offline: TE's own settings menu or settings.ini `te_options` (hex), kept in settings.ini.state (fe/settings.cpp, as on the Switch); not tried on the Mac |
| 20XX TE word 2: neutral spawns, v1.00 rules, quieter DL64, reset tournament rules, Y toggles frozen, skip results, random music, shield colours on L/R, no screen rumble, TE L-cancel flash, spoof plugins, collision bubbles, CPU smart DI, colour overlays, hand-warmers, stage striking | game image (quieter DL64 is host: jukebox gain) | wired offline: as word 1 (`te_options2`); not tried on the Mac |
| 20XX TE input display, lock settings | renderer / host | not wired |
| 20XX TE in-game settings menu | game image + host | wired offline: VS Mode > Tournament Melee with the TE profile; changes arrive through 0xF8 (fe/settings.cpp, as on the Switch); not tried on the Mac |
| 20XX TE menu music from Sound Test | game image + host | wired: command 0xF7 and `te_menu_music`; not tried |
| 20XX TE L-cancel training wheels | game image | upstream stub |
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
| Record and play back the CPU | game image | active: settings.ini `lab 1`, `lab_loop 1` (the Training module, fe/features/training.cpp) |
| Collision bubbles | game image | active: settings.ini `lab_bubbles 1` (over the model) or `2` (bubbles only), the Training module |
| Frame data and input overlay | renderer + host | active: the overlays module and `fe/overlay_draw.cpp` |
| Lab dummy DI, after-hitstun reaction, tech choice | game image | upstream stub |
| Frame advance | host + game image | active: the Training module |
| Fixed or frozen random seed | host + game image | active: settings.ini `practice_rng 1` (the same seed at each Training start and state load) or `2` (frozen), `practice_rng_seed` (the Training module) |
| Fight a pro (Phillip) | host + game image + external | active: Phillip's policies on the Switch build's own ONNX engine (`fe/proai`, onnxruntime-shaped calls, no onnxruntime library); downloaded when the player asks (Hugging Face, SHA-256 checked); tested: Zain's Marth policy fought in Training (0.6 ms per step) |
| Practice ghost AI | host | active: `fe/features/proai.cpp` |
| Pro habit tables | host | active: `fe/features/proai.cpp` (melee-unlocked's tables) |
| Training Mode CE events | game image + host | engine option, no setting; the Lab's Export and the Lab character select's Import now finish (mu-engine/patches/0013, Melee-NX-NetPlay's game-image fix: TM-CE's card wait had no native completion); not run on the Mac |
| Training Mode CE on-screen displays | game image | engine option, no setting |
| TM-CE console global OSD shortcut | n/a | not wired |
| Practice from replays | game image + host + renderer | active: X / Y jump to a player's last / next coach tip, A takes over there (the replays module; see section 3) |
| Lossless adapter lag test | game image + host | engine option, no setting |
| Display latency test | renderer | active: the overlays (latencytest) |

## 6. Mods, added content, cosmetics, music

| Feature | Side | Mac status |
|---|---|---|
| Mod profiles and disc-file overlay | host | active: the profile chosen for the next start (settings.ini mod_profile: the launcher's Mods page Use button, or the panel's MODS page) is loaded at an offline start (`fe/mods.cpp`); tested with a save layer |
| Content view switch | game image + host | wired as on the Switch: command 0xF5 (fe/mods.cpp); an online mode that plays the retail game gets the retail files back; not tried on the Mac |
| m-ex content | game image | wired with a mod disc's profile (Mods page, Use), as on the Switch; not tried on the Mac |
| m-ex stages | game image | wired offline with an m-ex disc's profile (built into the game image), as on the Switch; not tried on the Mac |
| Akaneia fighters | game image + host | wired offline with the Akaneia disc's profile, as on the Switch; not tried on the Mac |
| ACE fighters | game image | upstream stub ("experimental") |
| ACE Games menu | game image | upstream stub (all but Volleyball) |
| Rogue Mode | game image + host | wired offline with the rogueMelee disc's profile, as on the Switch; not tried on the Mac |
| Melee Maker | game image + host | wired offline with its disc's profile (bone tables through the host), as on the Switch; not tried on the Mac |
| 20XX Hack Pack: stage variants and stage select pages | game image + host | wired offline with the Hack Pack's profile, as on the Switch; not tried on the Mac |
| 20XX Hack Pack: music playlists | game image + host | wired offline with the Hack Pack's profile, as on the Switch; not tried on the Mac |
| 20XX Hack Pack: debug menu, training displays, hidden characters | game image | wired offline with the Hack Pack's profile, as on the Switch; not tried on the Mac |
| Per-profile and vanilla-only memory cards | host | active: `fe/mods.cpp` (card-<profile>), settings.ini vanilla_only |
| Costume, stage and effect imports | host + renderer | partial (fe/features/cosmetics.cpp from the Switch): costume and stage files copied to `~/Library/Application Support/Dashdance/melee-unlocked/Cosmetics/Costumes` and `Stages` are skins; the mod discs in the launcher's Mods folder add theirs. No import window, effect variants, pause screens or portraits |
| Skin cycling on the select screens | game image + host | active offline: L / R on a picked costume (command 0xF9), X / Y on a highlighted stage (0xFB), the title demo's memory check (0xFC); picks kept in Cosmetics/selection.txt; settings.ini `cosmetics 0` turns it off. Not tried by hand on the Mac yet |
| Voice mods | host + game image | engine option, no setting |
| Texture packs | renderer | not wired |
| Game music | game image | active: the game's own music through the app's sound output |
| Music volume | host / game image | active: settings.ini (melee-unlocked's key, `mu-engine/app/fe/settings.cpp`), with a row in the settings panel where the Switch has one |
| Custom music packs / jukebox | host | active: settings.ini `musicpacks 1` (off by default) and the AUDIO page's "Use custom music (next start)": the game hands its songs to melee-unlocked's jukebox.cpp (compiled from the fetched tree, `mu-engine/app/fe/patches/0118`, `mu-engine/patches/0015`; `fe/features/audio.cpp` from the Switch), which plays the disc's HPS or a file of the same name from `melee-unlocked/MusicPacks/audio/` (or a folder of them, a random pick per start): MP3, OGG, WAV, FLAC through minimp3, stb_vorbis, dr_wav and dr_flac (fetched at pinned commits with SHA-256 checks by `mu-engine/fetch.sh`), M4A/AAC, ALAC, AIFF and CAF through AudioToolbox (`fe/media_decode.cpp`; `mu-engine/tools/music_decode_test.sh`: 9 of 9). Tracks.txt and README.txt are written into the folder. `mu-engine/tools/app_audio_test.sh packs` (a hidden start driven into a VS match on Onett; SDL's disk driver records the output): `jukebox: music pack picked .../onetto+onetto2/tone.m4a` when the match starts, and a 440 Hz tone in every second of the recording from then on (26 seconds, 22..47); earlier runs picked the main menu's tone.flac from `menu01+...` (36 seconds); `off`: no tone. Music only, played locally (allowed online, as upstream) |
| Look presets | renderer | active: the Looks page of the settings panel and settings.ini (`look`, `look_fighter`, `look_stage`, `look_menus`, `look_unfiltered`, `look_colours`, `look_background`), fe/features/looks.cpp from the Switch; the rules (surfaces flat colour, white, posterize, 64 x 64, wire; port colours; the look's background) run per draw on the Metal renderer (gx::LookHooks, mu-engine/app/gx_looks_mac.cpp) on the game's draw owner tokens (0xF1, 0xF2, 0xF7, asked for only while a look needs them). Off by default; display only. The three presets that need the Lab's stick figures are left out, as on the Switch. Replay screenshots in build/looks (crystal-ports, wireframe, potato, 64-style, widescreen) from a recorded game played back with each setting |
| Lightweight levels and visual effects tiers | renderer (+ game image for low poly) | active: settings.ini `lightweight 0..5` (1 sets the game's low poly fighters) and `effects 0..3`, Looks page rows; skipped scenery and decorative particles per melee-unlocked's rules (gx_looks_mac.cpp). The end of a session logs `looks: N match frames: the look rules took 0.03-0.12 ms a frame ...; X draws skipped` |
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
| Easter egg | renderer + game image | active: melee-unlocked's character select egg (its walter_egg.cpp from the fetched tree, `fe/egg.cpp` and `fe/features/egg_command.cpp` from the Switch: the code is read from the panel's controller poll, the character select frame command 0xDA gives it the hands and camera; its corner sound offline only). `mu-engine/tools/app_egg_test.sh`: `walter egg: on (code on port 1, Bailey, ...)`, then Walter, `2 out`; screenshot build/shots-ui/egg.png |

## 7. Controllers

| Feature | Side | Mac status |
|---|---|---|
| Controller input to the game | host | active: the app's controller path (keyboard, SDL gamepads, GameCube adapter over IOKit, rumble) |
| GameCube adapter | host | active: the app's controller path (keyboard, SDL gamepads, GameCube adapter over IOKit, rumble) |
| Adapter polling-rate readout | host + renderer | active: the app's controller path (keyboard, SDL gamepads, GameCube adapter over IOKit, rumble) |
| Xbox, DualShock 4, DualSense, Switch Pro, Joy-Con, generic HID pads | host | active through SDL3's gamepad support in the app's controller path (mapping on the panel's CONTROLS page); not tried with each pad |
| Keyboard | host | active: the app's controller path (keyboard, SDL gamepads, GameCube adapter over IOKit, rumble) |
| Remapping | host + renderer | active: the panel's CONTROLS page (the app's remap, and the bindings page of fe/controls.cpp) |
| Controller profiles | host | active: fe/controls.cpp profiles (ControllerProfiles folder), on the CONTROLS page |
| Dolphin profile import | host | active: fe/controls.cpp; reads the app's Dolphin folder, else Slippi Dolphin's or Dolphin's GCPad profiles |
| Deadzone, radius, snapback filter, trigger cap and click point | host | active: fe/controls.cpp's setup page, applied after the app's input (IOKit adapter and SDL pads) |
| Box controllers | host | active: fe/controls.cpp box mode (melee-unlocked's box_melee.h) |
| PhobGCC settings on a stock controller | host + renderer | partial: the app's controller path (keyboard, SDL gamepads, GameCube adapter over IOKit, rumble) with its own calibration; melee-unlocked's PhobGCC filter on the adapter's raw reports (the Switch's phob.cpp and patches/0123) is not ported yet |
| PhobScope and Phob Check | renderer + host | not wired |
| "Play as P2/P3/P4" | host + renderer | active in the app's form: a controller's fixed port in its configuration (input_config, the launcher's controller pages; window_sdl.cpp gives fixed ports first) |
| Rumble and its switch | host | active: the app's controller path (keyboard, SDL gamepads, GameCube adapter over IOKit, rumble) |
| Button chords | host | active: Start + D-pad Down/Up + Z (ui/chords_mac.cpp, melee-unlocked's settings_chord.h) |
| Background input | host | active: settings.ini `background_input` (on by default, as upstream) with a row on the panel's Game page; off, every controller port reads neutral while another window has focus (port/runtime/host/window_sdl.cpp); the keyboard only counts with focus either way. Not tried by hand |

## 8. Display

| Feature | Side | Mac status |
|---|---|---|
| Picture from the GX stream | renderer | active: the app's GX decoder and Metal renderer |
| Aspect ratio | renderer | active: settings.ini aspect (the picture's shape) and the widescreen code |
| V-Sync | renderer | active: the launcher and the panel |
| Quality presets | renderer | active: the Picture page's Quality preset row sets internal resolution, sharpening and anisotropy (fe/features/overlays.cpp, from the Switch); its MSAA value is not used (the Mac's Metal renderer has no MSAA) |
| "Low spec" switch | renderer | upstream stub (replaced by Lightweight, which is active) |
| Internal resolution | renderer | active: the app's Metal renderer and launcher settings |
| Anti-aliasing | renderer | partial: supersampling through Internal resolution above the display's size; no MSAA in the Metal renderer. The Switch's `ssaa 2..4` setting (6e87ad8) is not copied: the Mac's Internal resolution already renders above the display |
| Anisotropic filtering | renderer | active: the panel's texture filtering row |
| Sharpening | renderer | active: settings.ini sharpness (the present pass's sharpening) |
| Brightness, contrast, vibrance | renderer | active: settings.ini brightness / contrast / vibrance, in the Metal present pass; off by default |
| Screen-space ambient occlusion | renderer | not wired |
| Window size, borderless and exclusive fullscreen | renderer | active: the app's Metal renderer and launcher settings |
| Graphics backend picker | renderer | not applicable: one renderer on the Mac (Metal) |
| DLSS, DLAA, XeSS, FSR upscaling | renderer | not applicable on the Mac (PC vendor libraries) |
| Frame generation | renderer | not applicable on the Mac (PC vendor libraries) |
| NVIDIA Reflex and latency readout | renderer | not applicable (NVIDIA PC driver); the overlays' Display latency test is active |
| HDR output | renderer | not wired |
| Path tracing, RTX Pack, RTX Remix, Ray Reconstruction | renderer + external | not applicable on the Mac (NVIDIA runtimes) |
| DLSS 5 neural rendering | renderer + external | not applicable on the Mac (NVIDIA model and runtime) |
| Shader pipeline precompile | renderer | active (the Metal renderer's own cache): every pipeline the game needed is kept in `melee-unlocked/shaders/pipelines.bin` and compiled ahead at the next start, on a background thread |

## 9. Audio

| Feature | Side | Mac status |
|---|---|---|
| Audio mixing | game image + host | active: the app's audio output (32 kHz, volume from the launcher) |
| Sound output | host | active: the app's audio output (32 kHz, volume from the launcher) |
| Master volume | host | active: the app's audio output (32 kHz, volume from the launcher) |
| Audio buffer | host | active: melee-unlocked's Auto (its AudioBufferPolicy from the fetched tree: the ring's slack grows 4 ms after each real gap in a match, settles back 1 ms per 10 s without one, never under 10 ms, remembered per Core Audio output device in `melee-unlocked/audio-auto.txt`) or Low latency (`audio_buffer_ms` 5..120, fixed), settings.ini `audio_mode` (0 Auto, the default) and two AUDIO rows; applied while the output plays (`port/runtime/host/audio_sdl.cpp` audio_set_buffer_ms; `fe/features/audio.cpp`). `app_audio_test.sh auto`: `audio: buffer Auto for the <the output device>: 20 ms (nothing learned yet)`, `ends at 16 ms (kept for the next session)`, the second start `16 ms (what it settled on last time)`; `fixed`: `audio: buffer Low latency, 15 ms`. WASAPI exclusive and ASIO are Windows-only |
| Speed nudging for online time sync | host | active: Slippi's time-sync speed goes to the host's pacing and to the sound output's resampling ratio (audio_set_speed, the Switch's frontend/patches/0110 applied to port/runtime/host/audio_sdl.cpp); not run online |
| Mono / Stereo | game image + host | in the engine |
| Audio mode: shared, low latency, exclusive, ASIO; output device picker | host | active: the app's audio output (32 kHz, volume from the launcher) |
| Menu sounds in the settings UI | renderer | active: launcher.ini `menusounds` (AUDIO > Menu, off by default, as upstream): upstream's ticks (740 Hz for a move, 990 Hz when the panel or a page opens, 590 Hz when it closes) mixed into the game's output before the sound device (`mu_play_main.cpp` mu_frontend_audio, ui/mu_panel.cpp). `app_audio_test.sh ISO menusounds`: `settings: menu sound 2` as the panel opens on CUSTOMIZE, then `menu sound 1`; the recording's strongest 990 Hz share is at that second (0.022, against a median of 0.001; the tick is quiet, as upstream's) |

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
| Settings appearances | renderer | active: every menu style of melee-unlocked's panel, drawn by its own code from the fetched tree (`ui/mu_ui_extract.py`): Clean side, Icon tiles, GD Melee (its kit pictures and font pages embedded at build time), Radial, Wide tabs, Simple, and the two legacy menus (Classic, Legacy Old; F11 switches when "Enable Legacy Menu" is on); each style's four palettes, Window transparency, Use custom accent with upstream's colour wheel, and upstream's style gallery (its six screenshots) on CUSTOMIZE, taken with the pointer or a controller (the panel now takes clicks and the scroll wheel, `ui/ui_metal.mm`); launcher.ini keys as upstream's (`overlaystyle`, `overlaypalette0..6`, `settingstransparency`, `menucustom`, `menuhue/menusat/menuval`, `legacymenu`, `legacymenustyle`). Screenshots: build/shots-ui/customize-clean.png, gallery.png, accent-wheel.png, transparency-accent.png, style1-home.png .. style5-video.png, style2-game.png, legacy-old.png, legacy-classic.png |
| Settings search | renderer | active: melee-unlocked's search code in the panel |
| Settings file | host | active: what the panel changes is saved to launcher.ini for the next start |
| Pause menu on Esc | renderer | active: Escape opens the settings panel |
| "Settings: F1" reminder | renderer | active: settings.ini `settingsreminder 1` (off by default, as upstream) or the Overlays row: "Settings: F1 / START + DOWN + Z" top right in the menus, never over a match, Training or a replay (fe/overlay_draw.cpp); seen in a screenshot |
| Frame counter and performance graph | renderer | active: settings.ini `showfps 1` (presented and game frames a second, simulation time) and `performance 1` (the last seconds of simulation time), the overlays module (fe/features/overlays.cpp, fe/overlay_draw.cpp); seen in a playback screenshot |
| Player nicknames above fighters | renderer | wired: settings.ini `playernicknames 1` draws the netplay host's names over the fighters online (the overlays module); not run online on the Mac |
| Controller input overlay | renderer | active: settings.ini `inputoverlay 1` with `inputoverlayvalues`, `inputoverlaystick`, `inputoverlayports`, `inputoverlayhideborder` (the overlays module); seen in a playback screenshot. Fixed place (no dragging) |
| Note when skins are off online | renderer | active: launcher.ini `skin_online_notice` (on by default, OVERLAYS > Notices): once per visit to the online screens, for 15 seconds, when a picked skin is served as the standard costume there (fe/features/cosmetics.cpp `cosmetics_off_online`, upstream's draw_skin_online_notice); not seen online on the Mac |
| Accessibility-type options | see sections 4, 8 | not wired |
| Text size or colour-blind setting | n/a | upstream stub (does not exist) |
| Logs | host | active: one timestamped session log per launch in ~/Library/Application Support/Dashdance/Logs (newest 50, latest.log), the engine's engine.log and engine.log.host in melee-unlocked/ |
| Trace-mark keys | host | not wired |
| Frame capture burst | renderer | not wired |
| One-frame glitch detector | renderer | not wired |
| Frame time CSV, simulation time CSV, latency trace | host / renderer | partial, in this app's own form: `--digest` CSVs, the `[frame N] ... sim:` log lines, `MELEE_METAL_LATENCY=1` |
| Scripted runs | host | active: `--script` (the reference scripts, digests) and `--play-script` (play mode through the app's input path) |
| Benchmark mode | n/a | upstream stub (does not exist in the game) |

## 12. Launcher and desktop

| Feature | Side | Mac status |
|---|---|---|
| Play page | launcher | active: melee-unlocked's Play page in the Mac launcher (port/runtime/host/mu_launcher.mm): disc, Game Build, PLAY, version and update line, Slippi account, mods |
| Engine picker | launcher | active: the Play page's Game Build row (Source Port, Static Recomp (Legacy), Vanilla) |
| CPU build auto-pick | launcher | not wired (left out: the Mac app is built from source) |
| Disc check | launcher + host | active: the launcher's Play page has a Disc line from the image's header (game code GALE01, revision 2 = NTSC 1.02; another revision, another game or a compressed image is named with what is needed), and the engine checks the disc again at start (`disc: retail NTSC 1.02, ...`); online the disc MD5 goes in game reports. No full MD5 check in the launcher |
| RVZ and WIA conversion | launcher | active: a Dolphin RVZ or WIA disc chosen in the launcher becomes GALE01.iso once when the engine starts (beside it, or in the engine's folder when that one cannot be written): melee-unlocked's disc_rvz.cpp and its bzip2 from the fetched tree, checked GameCube GALE01 first, written as .part then renamed (`mu-engine/app/disc_convert.cpp`, as the Switch's app.cpp). `mu-engine/tools/rvz_check.sh` runs upstream's own test and fixtures on this Mac: 46 checks, 0 failures. Progress goes to engine.log only (no window while it converts) |
| CISO, GCZ | launcher | upstream stub |
| Pipeline precompile page | launcher | not applicable: the Metal pipeline cache compiles ahead at each start (section 8); no page |
| Update check and one-click update | launcher + host + external | partial: the launcher's Play page compares the commit the app was built from (Info.plist MUBuildCommit, written by tools/package_macos_app.sh) with this repository's branch on GitHub (rkoh46/melee-unlocked-macOS, never melee-unlocked's updater) and says "N newer changes on GitHub" with How to update (the install line again) or "up to date"; `MELEE_UPDATE_CHECK=0` turns it off. No one-click update (the app is built from source) |
| Version rollback | launcher | not wired (left out: the Mac app is built from source) |
| Optional part downloads | launcher + host + external | not wired |
| Lab view and Pro AI asset downloads | host + external | not wired |
| "What's new" | launcher | active: this app's notes (`mu-engine/app/whats-new.txt`, in the bundle) in a card over the launcher, by themselves at the first start of a new version (launcher.ini `notesseen`, upstream's key) and from the version line's What's new link; screenshot build/shots-ui/launcher-notes.png |
| Launcher languages | launcher | active: upstream's Language button on the Play page with its nine languages, each named in itself (English, Español, Português (Brasil), Français, Deutsch, Русский, 日本語, 简体中文, 한국어); its lang/*.txt tables copied from the fetched tree into the bundle at packaging and looked up by English wording as upstream does (launcher_lang.h); Japanese, Chinese and Korean through the system's fonts; launcher.ini `language`. The Mac's own lines (Slippi sign-in, the keyboard hint) stay English, as upstream's lines without a translation do. Screenshots build/shots-ui/launcher-play-de.png, launcher-play-ja.png, launcher-rail.png (Korean) |
| Launcher colour and rail picture | launcher | active: the launcher colour wheel, and upstream's rail picture: a click on Bailey offers Choose a picture... (copied in as `melee-unlocked/rail-image.<ext>`) and Bring back Bailey; screenshot build/shots-ui/launcher-rail.png |
| Mods page | launcher | active: the launcher's Mods page lists, installs and removes mods with the shared installer (mu-engine/modinstall through mu-engine/app/mods_facade.cpp), and Use picks a profile for an offline start |
| Mods catalog | launcher + external | active: melee-unlocked's mods-catalog-2.json from the fetched tree, installed by the shared installer (`mu-engine/tools/mod_install_test.sh`) |
| Melee 3D | launcher | not wired |
| Replay library | launcher | active: the launcher's Replays page lists the games and Watch plays one in the melee-unlocked engine; in the engine, the panel's Replays page is melee-unlocked's library as the Switch has it (`fe/features/replay_library.cpp`): each replay's line and Slippi-Launcher-style stats, sort, hide short games, the search (a text prompt, `ui/software_keyboard_mac.mm`), the watch queue, "Show it as it was played" (`replayasplayed`, from a replay's .trace), Compress all (.slpz, each read back before its .slp goes; `compressreplays 1` also compresses earlier sessions at the start), Delete to the folder's .trash with Undo the last delete. `mu-engine/tools/app_replay_library_test.sh`: stats read, deleted to .trash and back with Undo, search "fox" 4 shown, watch queue 2, Compress all 4 of 4 to .slpz. The as-played pacing has no Mac test yet (no .trace in the test folder) |
| Replay stats | launcher | partial: the replays module's coach tips are logged at playback (fe/features/replay_coach_glue.cpp); no stats page |
| Watch live | launcher | active: the launcher's Replays page Watch live follows the Slippi Launcher's spectate folder (its Settings spectateSlpPath, else rootSlpPath/Spectate, else ~/Documents/Slippi/Spectate) and opens each new game file as it appears; the engine plays a file still being written, waiting at the top of each frame for it to arrive and ending at Game End or after 30 s without growth (replays.cpp, after melee-unlocked's source_host.cpp). Tested with tools/mac/slp_live_writer.py: a 7202-frame game written at 3x played to its Game End (0 differ); one written slower than real time waited for its frames and ended when it stopped growing. Not tried with the Slippi Launcher itself |
| Multiplayer Lobby | launcher + external | not wired |
| Hosted lobby service | launcher | upstream stub |
| Slippi account line | launcher | active: the Play page names the account (name and connect code) or says there is none |
| YouGame connect | launcher + external | out of scope |
| Crash report upload | launcher + external | partial: the launcher's Play page "Save crash report" writes melee-unlocked's report (`melee_crash_report.zip` and `.md` in `melee-unlocked/Logs`) with its privacy scrubbing (its launcher_crash_*.h from the fetched tree; `mu-engine/app/crash_report.cpp`, from the Switch's diagnostics.cpp): the newest macOS crash report of the engine or app from the last week, the engine's log and the session before it (the engine now keeps `engine.prev.log`), the last day's session traces. Nothing is uploaded (upstream's relay is not contacted). `mu-engine/tools/crash_report_test.sh`: a connect code, Slippi name, uid, IP and e-mail address, this Mac's user name and full file locations put in made-up logs are all gone from the zip; ordinary lines stay |
| Settings window from the launcher | launcher | partial: the launcher's Settings page (window, picture, sound, online); the rest is the in-game panel |
| Standalone double-click start | host | active: the app opens from Finder or the Dock into the launcher |

## Additions from the full audit of melee-unlocked 1.0.5

Items the audit of melee-unlocked's source found that the list above does not have (its "MISSING" tables, by audit and
ID). All are not wired on the Mac unless the status says otherwise; the note is the audit's macOS port note.

### audit-hle-abi

| ID | Item | Mac status and port note |
|---|---|---|
| M1 | Connect-code history and autocomplete (Direct and Teams) | active: the netplay object answers 0xBE as Slippi Dolphin does (direct-codes.json, teams-codes.json beside user.json); app_online_menus_test.sh online_direct_keep: the entry offered MOCK#002, Z took it, `matchmaking started (mode 2, code 'MOCK#002')` |
| M2 | Custom quick-chat messages | not wired; easy / easy. FEATURES "Quick chat" only covers message ids |
| M3 | Display name refresh from Slippi's user API | active (mu-engine/net, Melee-NX-NetPlay's net-0008): after sign-in the display name, connect code, latest version and chat lines come from Slippi's user service on a thread of its own; user.json is never written. Loopback test: `profile refreshed from Slippi's user service: Mock 2 (refreshed) (MOCK#002), its 16 chat messages` |
| M4 | Slippi account auto-discovery (desktop) | not wired; not applicable on Switch (sign-in screen) / easy and important on macOS: players already signed in with the Slippi Launcher play without signing in again |
| M5 | Replay upload when Slippi asks, and disc MD5 in the report | active (mu-engine/net, net-0008): Slippi 3.7.0's reporter is in the engine's netplay object; the report carries the disc MD5 (cached in iso_md5_cache.txt) and the replay is asked for on the reporter's thread when Slippi's reply has uploadUrl. app_online_test.sh against tools/mac/mock_slippi_http.py: `game report ... isoHash 0e63d422...`, `replay upload /upload/1: 1266301 bytes (gzip) ... a Slippi replay` |
| M6 | Same-router fallback addresses | active (mu-engine/net, net-0006): same public address: the LAN address first, after 2 s the public one too, in the same 8 s. online_loopback_test.sh with `MU_MOCK_ARGS=--dead-lan`: `the same-router (LAN) address has not answered in 2 s; trying the public address too`, connected, net check equal |
| M7 | Play while searching for a Slippi Direct or Teams code | active: Direct and Teams searches are kept like Unranked (search module + net-0005); online_direct_keep run on the Mac (see "Play while you search") |
| M8 | Warm-up sparring partner (sub-item) | wired (net-0010 `remote_character`; fe/features/search.cpp, Fight a pro for the CPU); not run on the Mac (see "Warm-up room") |
| M9 | "Search again?" after an online session (sub-item) | wired (search module, net-0010 `code`); not run on the Mac (see "Training after online") |
| M10 | Kept-search question on leaving the select (sub-item) | active: settings.ini `keep_search` 0 ask, 1 always, 2 never; online_keep_ask run on the Mac (B, the question, A, found, back, match) |
| M11 | Auto delay from a pre-match ping | not wired; easy / easy |
| M12 | Opponent's automatic L-cancel announced | partial: between Melee-NX-NetPlay and Mac builds the build message says whether each player offers it and the log names it (net-0011); nothing on the select; not with a Slippi 3.7.0 peer |
| M13 | Keyboard key names in prompts | not wired; not applicable on Switch (no keyboard on the console) / easy on macOS |
| M14 | Legacy replays (no stored code list) | not wired; easy / easy (map the legacy list to `MU_REPLAY_CODE_*`; the static engine serves bytes) |
| M15 | Slippi 3.7.0 rank, MMR and bot flags in replays | not wired; easy / easy |
| M16 | Player drop read by own frames (sub-item) | not wired; easy / easy |
| M17 | Slow first frame is not "gone" | active (mu-engine/net, net-0007): 30 s before the other player's first online frame, Slippi's 7 s after. online_loopback_test.sh with `MU_B_ENV=MELEE_NET_TEST_SLOW_FIRST_FRAME_MS=12000`: side a waited 714 retraces (12 s) at frame 8, no drop, net check equal |
| M18 | Network condition simulator (developer) | active (test aid): `MELEE_NET_LOSS_PCT`, `MELEE_NET_LAG_MS`, `MELEE_NET_JITTER_MS` as upstream's hle/slippi_net.cpp (`mu-engine/net/patches/net-0012`, the Switch's). `app_online_test.sh` with 5 % loss, 40 ms lag and 10 ms jitter on both sides: `dropping 5% of received datagrams (testing)`, 463 and 464 rollbacks, net check EQUAL (9 checkpoints), 0 desync messages |
| M19 | Slippi command trace (developer) | active (test aid): `MELEE_TRACE_SLIPPI_CMDS=1` logs each menu and online command with its payload and the reply's FNV-1a, runs collapsed with a count, the per-frame streams left out (`mu_play_main.cpp`, as the Switch's play_main.cpp); the same online test: `slippi-cmd: seq 1 retrace 2 cmd F5 len 1 payload FE reply 1 fnv ...`, `cmd B3 ... (x106)` |
| M20 | Smoother time sync (experimental) | not wired; easy / easy |

### audit-app-host

| ID | Item | Mac status and port note |
|---|---|---|
| M1 | Replays: "Open replay" (browse) | active: the launcher's Replays page "Open a replay..." (an open panel for .slp and .slpz) |
| M2 | Replays: "Compress all replays now (.slpz)" | active: the engine panel's Replays page (Replay library row) |
| M3 | Replays: refuse a replay played on a mod stage | not wired; easy |
| M4 | Replays: content fingerprint check | not wired; easy |
| M5 | Replays: delete to Recycle Bin, right-click "Show in Explorer" | not wired; easy: NSWorkspace recycle / reveal in Finder |
| M6 | Mods catalog refreshed online | active: the launcher fetches melee-unlocked's raw `mods-catalog-2.json` once per start (HTTPS only, 1 MB cap, metadata only), takes it under the same rules as the built-in one (`Manager::replace_catalog`, upstream's merge of credits and notes) and keeps the copy in `Mods/.cache` for the next start and the engine's MODS page (`mu-engine/app/mods_facade.cpp`, as the Switch's mods_service.cpp); `MU_NO_CATALOG_REFRESH=1` turns it off, `MU_CATALOG_URL` points it elsewhere. Tested without the network: a kept copy with a changed version is taken (`mods: catalog refreshed from the copy kept from the last refresh: 10 mods`, the card shows it: build/shots-ui/launcher-catalog.png); a file:// URL is refused by the fetcher (`only https links are fetched`) |
| M7 | Mods: "Choose skins" button and skin count on a mod card | active: MODS > Choose skins (two rows: the costume that has skins, then its skin, as the character select's L / R cycle; fe/features/cosmetics.cpp from the Switch); each mod disc's skin count is written to `melee-unlocked/mods-cache/disc-skins.txt` at each start for the launcher's Mods page |
| M8 | Mod disc costumes as skins in the normal game | active (fe/features/cosmetics.cpp, melee-unlocked's disc_skin_scan rule): every mod disc in the launcher's Mods folder but the active profile's adds each costume that differs from the retail disc's as a skin "<profile> disc"; the scan is cached in `melee-unlocked/mods-cache` |
| M9 | Skin thumbnails | not wired; easy-medium |
| M10 | Pack skin rule online: look-only costumes stay on | active: in a session that can go online a mod profile serves only its costumes that pass melee-unlocked's own pack skin rule and skeleton check (compiled from the fetched tree's pack_skin_rule.* and cosmetic_mods.cpp), in their online form (the standard rest pose bit for bit). `mu-engine/tools/app_skins_test.sh`: a costume nudged by 1e-4 `stays on online (67 joints match)`, one moved by 5 units `the standard costume online (Skeleton joint 1 rest pose differs from the vanilla costume (moved).)` |
| M11 | Stage graft: mod stage meshes on the original stage online | not wired; hard |
| M12 | Alternate stage pick by held buttons | not wired; medium |
| M13 | Keep the display awake during controller-only play | active: the engine holds a PreventUserIdleDisplaySleep assertion while its window is up (mu_play_main.cpp; `power: the display stays awake while the game runs`) |
| M14 | Mouse cursor hides over the game | active: the pointer hides after two seconds at rest over the focused game window and shows while the settings panel is open (window_sdl.cpp, melee-unlocked's cursor_idle rule) |
| M15 | Settings-panel controller ownership rules | not wired; easy |
| M16 | Game calendar date from the real clock | not wired; easy |
| M17 | DSP coefficient table, player dump first | not wired; easy |
| M18 | Crash report privacy scrubbing | active: melee-unlocked's own rules (Crash report row) |
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
| M33 | Alt+Enter fullscreen toggle | active: Control-Command-F (the Mac's shortcut) and Alt+Return toggle full screen, and the green button works as everywhere; the panel's Fullscreen row follows the window and launcher.ini keeps it |
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
| A51 | Menu styles | active (Settings appearances, section 11) |
| A53 | Radial wheel navigation | active: the Radial style's wheel, with the stick and the D-pad (settings_radial_nav from the fetched tree) |
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
| D17 | Lightweight drawn/skipped counter | active: the end of a session logs the looks' match frames, rule time, draws skipped and drawn, tinted, drawn as lines and textures treated (fe/features/looks.cpp run_end) |
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

