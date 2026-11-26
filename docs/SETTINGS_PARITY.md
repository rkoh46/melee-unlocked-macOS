# Settings parity with melee-unlocked

Every setting melee-unlocked 1.0.5 exposes (its settings panel, the panel's Practice menu, its launcher and the files they
write), read from its source at the pinned commit, against melee-unlocked-macOS. One row per setting (per-port and
per-device families are one row). Where the Mac has a setting, it is in the launcher's Settings window (Settings > All
settings…, `port/runtime/host/mu_settings_window.mm`, grouped as melee-unlocked's panel groups them), in the in-game panel
(F1 or Esc) or both; both write the same files the engine reads (`melee-unlocked/settings.ini`, `launcher.ini`).

| Status | Rows | Meaning |
|---|---|---|
| There | 231 | The Mac app already had it with a working effect (location in Notes). |
| There, row added | 24 | The engine already honoured the key but nothing let the player change it; now a row. |
| Added | 15 | New in this change, with its effect in the engine or launcher. |
| Not added | 0 | Applies on the Mac but is not there yet (reason in Notes). |
| N/A | 96 | Cannot apply on the Mac (Windows APIs, NVIDIA, the Static Recomp, melee-unlocked-only network features, internal markers). Not shown. |
| Excluded | 21 | Left out on purpose: YouGame, the BitTorrent Multiplayer Lobby, crash upload to melee-unlocked's relay. |


Fixed along the way: the panel's Launcher colour now changes the launcher's colour; an automatic online delay stays automatic
after the panel is used; Direct pause, port colours, the opponent's shield colour and Skip in Unranked reach the netplay;
Sharpening reaches the Metal present pass; watching a replay keeps the profile folder when the replay folder is moved.

| melee-unlocked setting | Where in melee-unlocked | Mac status | Notes |
|---|---|---|---|
| Quality (`custompreset`) | Panel: Video | There | Launcher Settings page > DISPLAY |
| Frame rate (`fps`) | Panel: Video | There, row added | the engine read it with no row; now Settings window |
| VSync (prevents tearing) (`vsync`) | Panel: Video | There | Launcher Settings page > DISPLAY |
| - (hidden: marks VSync as an explicit player choice) (`vsyncchoice`) | Panel: Video | N/A | internal marker, not a setting |
| - (hidden: VRR display detected, keeps fps under refresh) (`vrrfound`) | Panel: Video | N/A | internal marker (DXGI VRR detection) |
| Display = Borderless fullscreen (`fullscreen`) | Panel: Video > Display | There | Launcher Settings page > DISPLAY |
| Display = Exclusive fullscreen (experimental) (`exclusivefullscreen`) | Panel: Video > Display | N/A | Windows-only (DXGI exclusive fullscreen) |
| Widescreen (`truewidescreen`) | Panel: Video | There, row added | the engine read it with no row; now Settings window |
| Widescreen shape (`truewsaspect`) | Panel: Video > Widescreen | Added | 16:9, 16:10, Custom and Fit the window: the widened projection is scaled by (73/60)/aspect (fe/looks_live.h true_aspect, gx_looks_mac.cpp), the picture takes that shape, FoD reflections follow (picture_widen); Settings window and panel. Replay captures at 16:9/16:10/21:9 differ as expected with 0 differing records |
| Custom shape (W:H) (`truewscustomw / truewscustomh`) | Panel: Video > Widescreen (Custom) | Added | Custom shape width and height (1..100 each), Settings window and panel |
| - (hidden migration marker) (`truewsfit`) | Panel: Video | N/A | migration marker |
| - (hidden: old Slippi 16:9 Gecko code; migrated to True widescreen 16:9 at load) (`widescreen`) | Panel: Video | There | Launcher Settings page > DISPLAY; in-game panel > VIDEO > Display (Aspect ratio) |
| Aspect ratio (`aspect`) | Panel: Video | There | in-game panel > VIDEO > Picture |
| Window size (`window`) | Panel: Video | Added | settings.ini window WxH: the game window opens at that size (mu_play_main.cpp), still resizable; Follow window is the default. Log: window: 960x720 points |
| Internal resolution (`scale`) | Panel: Video | There | Launcher Settings page > DISPLAY |
| Anti-aliasing (`msaa / ssaa`) | Panel: Video | There | in-game panel > VIDEO > Picture (Anti-aliasing (MSAA), Quality preset); VIDEO > Display row hidden; MSAA no longer leaves black specks (MAC_FIXES 11) |
| Anisotropic filtering (`anisotropy`) | Panel: Video | There | in-game panel > VIDEO > Display (Anisotropic filtering); VIDEO > Picture Quality preset |
| Get mod looks (Install: Diet Melee / Animelee / Animelee x DBZ) (`-`) | Panel: Video > Get mod looks | N/A | Windows-only (xdelta3.exe tool, ShellExecute); hidden in YouGame app |
| Look (`- (sets lightweight + look_* keys)`) | Panel: Video > Low-Detail Modes | There | in-game panel > GAME > Looks |
| Crystal fighter colors (`- (look preset 5/6)`) | Panel: Video > Low-Detail Modes | Added | with a Crystal look, switches between Crystal (original colours) and Crystal (player colours), presets 5 and 6: the Settings window's Look rows and the panel's Looks > Crystal fighter colours |
| Visual effects (`effects`) | Panel: Video > Low-Detail Modes | There | in-game panel > GAME > Looks |
| Lightweight (`lightweight`) | Panel: Video > Low-Detail Modes > Advanced lightweight levels | There | in-game panel > GAME > Looks |
| Set up mod artwork (Diet and Crystal): Patch folder / Prepare mod artwork (`-`) | Panel: Video > Low-Detail Modes > Set up mod artwork | N/A | Windows-only (Browse dialog, xdelta3.exe) |
| Fighters (`look_fighter`) | Panel: Video > Low-Detail Modes > Customise the look | There | in-game panel > GAME > Looks |
| Stage (`look_stage`) | Panel: Video > Low-Detail Modes > Customise the look | There | in-game panel > GAME > Looks |
| Menus and HUD (`look_menus`) | Panel: Video > Low-Detail Modes > Customise the look | There | in-game panel > GAME > Looks |
| Unfiltered textures (`look_unfiltered`) | Panel: Video > Low-Detail Modes > Customise the look | There | in-game panel > GAME > Looks |
| Fighter colours (`look_colours`) | Panel: Video > Low-Detail Modes > Customise the look | There | in-game panel > GAME > Looks |
| Background (`look_background`) | Panel: Video > Low-Detail Modes > Customise the look | There | in-game panel > GAME > Looks |
| Background colour (`look_background_rgb`) | Panel: Video > Low-Detail Modes > Customise the look | There, row added | the engine read it with no row; now Settings window |
| Lab view (Ctrl+F3) (`labview`) | Panel: Video > Low-Detail Modes > Customise the look | There | the Mac's own: in-game panel > VIDEO > Picture (Static Recomp upstream) |
| Stage drawing (`look_lab_stage`) | Panel: Video > Low-Detail Modes > Customise the look > Stick figures, capsules and silhouettes | There | the Mac's own: in-game panel > VIDEO > Picture (Static Recomp upstream) |
| P1 colour..P4 colour (4 keys) (`lab_colour_p1..p4`) | Panel: Video > Low-Detail Modes > Customise the look > Stick figures, capsules and silhouettes | There | the Mac's own: in game (Static Recomp upstream) |
| Background colour (Lab) (`lab_colour_background`) | Panel: Video > Low-Detail Modes > Customise the look > Stick figures, capsules and silhouettes | There | the Mac's own: none (Static Recomp upstream) |
| Stage colour (`lab_colour_stage`) | Panel: Video > Low-Detail Modes > Customise the look > Stick figures, capsules and silhouettes | There | the Mac's own: none (Static Recomp upstream) |
| - (hidden: Lab fighter drawing set by Look presets: 0 3D, 1 stick figures, 2 capsules) (`look_lab_fighters`) | Panel: Video > Low-Detail Modes | There | the Mac's own: in-game panel > VIDEO > Picture (Static Recomp upstream) |
| - (hidden: set by Look presets: own renderer version, full models at Lightweight, frame cap lifted) (`look_own / look_full_models / look_cap_lifted`) | Panel: Video > Low-Detail Modes | N/A | internal markers set by melee-unlocked's Look presets |
| - (hidden: fps cap and sub-frame mode kept while a preset lifts the cap) (`look_saved_fps / look_saved_subframe`) | Panel: Video > Low-Detail Modes | N/A | internal: values kept while a Look preset lifts the cap |
| - (hidden legacy: low poly fighters) (`lowpoly`) | Panel: Video > Low-Detail Modes | There, row added | the engine read it with no row; now Settings window |
| - (hidden legacy: Retro (64 style); now the 64 Style look) (`retro`) | Panel: Video > Low-Detail Modes | N/A | legacy key, now the 64 Style look (look_fighter 1), which is there |
| - (hidden migration marker for Low spec -> Lightweight) (`lowspec_look`) | Panel: Video > Low-Detail Modes | N/A | migration marker |
| - (hidden: Low spec on, Lightweight 4+) (`lowspec`) | Panel: Video > Low-Detail Modes | There | as Lightweight (lightweight), which is what it migrates to |
| - (hidden: image settings saved while Low spec holds them) (`lowspec_prev_backend/fps/scale/ssaa/anisotropy/effects/dlss/subframe (8 keys)`) | Panel: Video > Low-Detail Modes | N/A | Windows-only (backend/dlss values) |
| Show attack hitboxes (`arcade_hitboxes`) | Panel: Video > Arcade look | There | in-game panel > VIDEO > Arcade look |
| Projectile hitboxes too (`arcade_projectile_hitboxes`) | Panel: Video > Arcade look | There | in-game panel > VIDEO > Arcade look |
| Draw hitboxes behind the fighter (`arcade_hitboxes_behind`) | Panel: Video > Arcade look | There | in-game panel > VIDEO > Arcade look |
| Combo counter (`arcade_combo_counter`) | Panel: Video > Arcade look | There | in-game panel > VIDEO > Arcade look |
| 2D camera (`arcade_camera_2d`) | Panel: Video > Arcade look | There | in-game panel > VIDEO > Arcade look |
| Background (`arcade_backdrop`) | Panel: Video > Arcade look | There | in-game panel > VIDEO > Arcade look |
| Background colour (`arcade_background_rgb`) | Panel: Video > Arcade look | There, row added | the engine read it with no row; now Settings window |
| Grid colour (`arcade_grid_rgb`) | Panel: Video > Arcade look | There, row added | the engine read it with no row; now Settings window |
| Arcade preset / Reset (`-`) | Panel: Video > Arcade look | There, row added | the panel had them (Arcade look: every switch on / reset); now they also set the combo counter as upstream does, and the Settings window has the two buttons |
| Graphics backend (`backend`) | Panel: Video | N/A | Windows-only (D3D12/D3D11) |
| Upscaling (DLSS / XeSS / FSR) (`dlss`) | Panel: Video | N/A | Windows-only (DLSS/XeSS need D3D12; FSR D3D12/Vulkan Windows) |
| Download Intel XeSS / Download AMD FSR (`-`) | Panel: Video | N/A | Windows-only (Intel/AMD DLLs); hidden in YouGame app |
| Frame generation (`framegen`) | Panel: Video | N/A | Windows-only (NVIDIA Streamline / AMD FSR FG) |
| - (hidden: cached driver frame-generation capability) (`fgmax / fgdynamic`) | Panel: Video | N/A | Windows-only (Streamline) |
| NVIDIA Reflex Low Latency (`reflex`) | Panel: Video | N/A | Windows-only (NVIDIA Reflex, D3D12) |
| HDR output (`hdroutput`) | Panel: Video > HDR | There | the Mac's own: in-game panel > VIDEO > Picture (Windows-only (D3D12 + Windows HDR) upstream) |
| Paper white (`hdrpaperwhite`) | Panel: Video > HDR | There | the Mac's own: in-game panel > VIDEO > Picture (Windows-only (D3D12 + Windows HDR) upstream) |
| Peak brightness (`hdrpeak`) | Panel: Video > HDR | There | the Mac's own: in-game panel > VIDEO > Picture (Windows-only (D3D12 + Windows HDR) upstream) |
| Path tracing (`pathtracing`) | Panel: Video > Ray tracing | N/A | Windows-only (D3D12, RTX GPU, needs DLSS/DLAA) |
| Lighting (`rtxintegrator`) | Panel: Video > Ray tracing | N/A | Windows-only (D3D12, RTX GPU, needs DLSS/DLAA) |
| Preset (`rtxpreset`) | Panel: Video > Ray tracing | N/A | Windows-only (D3D12, RTX GPU, needs DLSS/DLAA) |
| Look (`rtxlook`) | Panel: Video > Ray tracing | N/A | Windows-only (D3D12, RTX GPU, needs DLSS/DLAA) |
| Brightness (ray tracing) (`rtxbrightness`) | Panel: Video > Ray tracing | N/A | Windows-only (D3D12, RTX GPU, needs DLSS/DLAA) |
| Reflection strength (`rtxreflstrength`) | Panel: Video > Ray tracing | N/A | Windows-only (D3D12, RTX GPU, needs DLSS/DLAA) |
| Floor reflection strength (`rtxfloorstrength`) | Panel: Video > Ray tracing | N/A | Windows-only (D3D12, RTX GPU, needs DLSS/DLAA) |
| Shadow strength (`rtxshadowstrength`) | Panel: Video > Ray tracing | N/A | Windows-only (D3D12, RTX GPU, needs DLSS/DLAA) |
| Bounce light strength (`rtxbouncestrength`) | Panel: Video > Ray tracing | N/A | Windows-only (D3D12, RTX GPU, needs DLSS/DLAA) |
| Ray-traced shadows (`rtxshadows`) | Panel: Video > Ray tracing | N/A | Windows-only (D3D12, RTX GPU, needs DLSS/DLAA) |
| Ambient occlusion (`rtxao`) | Panel: Video > Ray tracing | N/A | Windows-only (D3D12, RTX GPU, needs DLSS/DLAA) |
| Bounce light (`rtxbounce`) | Panel: Video > Ray tracing | N/A | Windows-only (D3D12, RTX GPU, needs DLSS/DLAA) |
| Reflections (`rtxreflections`) | Panel: Video > Ray tracing | N/A | Windows-only (D3D12, RTX GPU, needs DLSS/DLAA) |
| Glossy stage floors (`rtxglossyfloors`) | Panel: Video > Ray tracing | N/A | Windows-only (D3D12, RTX GPU, needs DLSS/DLAA) |
| Soft skin (`rtxskin`) | Panel: Video > Ray tracing | N/A | Windows-only (D3D12, RTX GPU, needs DLSS/DLAA) |
| Photo mode (`rtxphotomode`) | Panel: Video > Ray tracing | N/A | Windows-only (D3D12, RTX GPU, needs DLSS/DLAA) |
| Fighter shadows on the stage (`rtxfightershadows`) | Panel: Video > Ray tracing | N/A | Windows-only (D3D12, RTX GPU, needs DLSS/DLAA) |
| Stage shadows and contact shade (`rtxstageshade`) | Panel: Video > Ray tracing | N/A | Windows-only (D3D12, RTX GPU, needs DLSS/DLAA) |
| Path tracing on stage mods (`rtxmoddedstages`) | Panel: Video > Ray tracing | N/A | Windows-only (D3D12, RTX GPU, needs DLSS/DLAA) |
| Shiny floors on stage mods (`rtxmodfloors`) | Panel: Video > Ray tracing | N/A | Windows-only (D3D12, RTX GPU, needs DLSS/DLAA) |
| Effects light the stage (`rtxeffectlight`) | Panel: Video > Ray tracing | N/A | Windows-only (D3D12, RTX GPU, needs DLSS/DLAA) |
| Glowing stage art lights the fighters (`rtxglowlight`) | Panel: Video > Ray tracing | N/A | Windows-only (D3D12, RTX GPU, needs DLSS/DLAA) |
| Radiance cache (SHaRC) (`rtxradiancecache`) | Panel: Video > Ray tracing | N/A | Windows-only (D3D12, RTX GPU, needs DLSS/DLAA) |
| NVIDIA Ray Reconstruction (`rayreconstruction`) | Panel: Video > Ray tracing | N/A | Windows-only (D3D12, RTX GPU, needs DLSS/DLAA) |
| DLSS 5 Neural Rendering (`dlss5`) | Panel: Video > DLSS 5 (below Ray tracing) | N/A | Windows-only (NVIDIA DLSS 5, D3D12, GX_DLSS5 builds) |
| DLSS 5 model (`dlss5style`) | Panel: Video > DLSS 5 (below Ray tracing) | N/A | Windows-only (NVIDIA DLSS 5, D3D12, GX_DLSS5 builds) |
| Overall intensity (`dlss5intensity`) | Panel: Video > DLSS 5 (below Ray tracing) | N/A | Windows-only (NVIDIA DLSS 5, D3D12, GX_DLSS5 builds) |
| Structure intensity (`dlss5detail`) | Panel: Video > DLSS 5 (below Ray tracing) | N/A | Windows-only (NVIDIA DLSS 5, D3D12, GX_DLSS5 builds) |
| Local tone intensity (`dlss5tone`) | Panel: Video > DLSS 5 (below Ray tracing) | N/A | Windows-only (NVIDIA DLSS 5, D3D12, GX_DLSS5 builds) |
| Character intensity (`dlss5skin`) | Panel: Video > DLSS 5 (below Ray tracing) | N/A | Windows-only (NVIDIA DLSS 5, D3D12, GX_DLSS5 builds) |
| Character mask (`dlss5automask`) | Panel: Video > DLSS 5 (below Ray tracing) | N/A | Windows-only (NVIDIA DLSS 5, D3D12, GX_DLSS5 builds) |
| DLSS 5 processing (`- (dlss5resolution/passes/filters)`) | Panel: Video > DLSS 5 (below Ray tracing) | N/A | Windows-only (NVIDIA DLSS 5, D3D12, GX_DLSS5 builds) |
| Neural resolution scale (`dlss5resolution`) | Panel: Video > DLSS 5 (below Ray tracing) | N/A | Windows-only (NVIDIA DLSS 5, D3D12, GX_DLSS5 builds) |
| Pass count (`dlss5passes`) | Panel: Video > DLSS 5 (below Ray tracing) | N/A | Windows-only (NVIDIA DLSS 5, D3D12, GX_DLSS5 builds) |
| Profile (Save as... / Delete / Reset to NVIDIA defaults) (`- (Dlss5Profiles/<name>.txt)`) | Panel: Video > DLSS 5 (below Ray tracing) | N/A | Windows-only (NVIDIA DLSS 5, D3D12, GX_DLSS5 builds) |
| - (hidden: DLSS 5 defaults version marker) (`dlss5defaults`) | Panel: Video > DLSS 5 (below Ray tracing) | N/A | Windows-only (NVIDIA DLSS 5, D3D12, GX_DLSS5 builds) |
| Border art (`borderart`) | Panel: Video | There | in-game panel > VIDEO > Picture |
| Border edge (`borderedge`) | Panel: Video | There | in-game panel > VIDEO > Picture |
| Built-in CRT look (`crt_filter`) | Panel: Video > Built-in CRT | There | in-game panel > VIDEO > Picture |
| Screen curve (`crt_curve`) | Panel: Video > Built-in CRT | There | in-game panel > VIDEO > Picture |
| Shader preset (Browse shader presets / Apply shader preset) (`shaderpreset`) | Panel: Video > RetroArch shader presets | There | in-game panel > VIDEO > Picture |
| Sharpening (`sharpness`) | Panel: Video (below shader presets) | There | in-game panel > VIDEO > Picture |
| Screen-space ambient occlusion (`ssao`) | Panel: Video (below shader presets) | There | in-game panel > VIDEO > Picture |
| Brightness (`brightness`) | Panel: Video (below shader presets) | There | in-game panel > VIDEO > Picture |
| Contrast (`contrast`) | Panel: Video (below shader presets) | There | in-game panel > VIDEO > Picture |
| Vibrance (`vibrance`) | Panel: Video (below shader presets) | There | in-game panel > VIDEO > Picture |
| Sub-frame animation (`subframe`) | Panel: Video (below shader presets) | There, row added | the engine read it with no row; now Settings window |
| Use texture packs (`customtextures`) | Panel: Video > HUD and texture packs | There | in-game panel > VIDEO > Picture |
| Load them at startup (`prefetchtextures`) | Panel: Video > HUD and texture packs | There | in-game panel > VIDEO > Picture |
| ON/OFF per installed pack (`texpackoff`) | Panel: Video > HUD and texture packs | There | in-game panel > VIDEO > Picture |
| Use looping MP4 backgrounds (`videobackgrounds`) | Panel: Video > Animated menu backgrounds | There | in-game panel > VIDEO > Picture |
| CSS texture target / SSS texture target (+ Relearn CSS/SSS) (`-`) | Panel: Video > Animated menu backgrounds | There | the Mac video background module finds the menu textures itself (videobackgrounds) |
| Advanced (group open/closed) (`advancedvideo`) | Panel: Video > Advanced | Added | every section of the in-game panel stays open or closed between starts (launcher.ini `panel_closed`, ui/mu_panel.cpp section_remembered); `app_panel_sections_test.sh` |
| Reflex flash indicator (`reflexflash`) | Panel: Video > Advanced | N/A | Windows-only (NVIDIA Reflex, D3D12) |
| Display latency test (`latencytest`) | Panel: Video > Advanced | There | in-game panel > OVERLAYS > Overlays |
| Lossless adapter lag test (next launch only) (`lagtestnext`) | Panel: Video > Advanced | There | in-game panel > VIDEO > Advanced |
| Dump textures (for making a pack) (`dumptextures`) | Panel: Video > Advanced | There | in-game panel > VIDEO > Picture |
| Weight preset (`dlss5preset`) | Panel: Video > Advanced (DLSS 5) | N/A | Windows-only (NVIDIA DLSS 5, D3D12, GX_DLSS5 builds) |
| Character intensity: let the model decide (`dlss5skin (-1)`) | Panel: Video > Advanced (DLSS 5) | N/A | Windows-only (NVIDIA DLSS 5, D3D12, GX_DLSS5 builds) |
| Remove haze (`dlss5tonerestore`) | Panel: Video > Advanced (DLSS 5) | N/A | Windows-only (NVIDIA DLSS 5, D3D12, GX_DLSS5 builds) |
| Downsample filter (`dlss5downsample`) | Panel: Video > Advanced (DLSS 5) | N/A | Windows-only (NVIDIA DLSS 5, D3D12, GX_DLSS5 builds) |
| Upsample filter (`dlss5upsample`) | Panel: Video > Advanced (DLSS 5) | N/A | Windows-only (NVIDIA DLSS 5, D3D12, GX_DLSS5 builds) |
| Scale reconstruction (`dlss5reconstruction`) | Panel: Video > Advanced (DLSS 5) | N/A | Windows-only (NVIDIA DLSS 5, D3D12, GX_DLSS5 builds) |
| - (hidden diagnostic: one-frame glitch scan, file only) (`flickerscan`) | Panel: Video | There | in-game panel > VIDEO > Picture |
| - (hidden: VRAM overlay line; VRAM is shown as a hint on Video) (`showvram`) | Panel: Video | N/A | DXGI video memory query; Apple silicon has unified memory |
| Music (`music`) | Panel: Audio | There | in-game panel > AUDIO > Music and sound |
| Use custom music (+ Open music packs) (`musicpacks`) | Panel: Audio | There | in-game panel > AUDIO > Music and sound |
| Volume (`volume`) | Panel: Audio | There | Launcher Settings page > SOUND; in-game panel > AUDIO > Volume |
| Audio mode (`audio_mode`) | Panel: Audio | There | the Mac's own: in-game panel > AUDIO > Output (Windows-only (WASAPI/ASIO) upstream) |
| ASIO driver (`audio_asio_driver`) | Panel: Audio | N/A | Windows-only (ASIO, registry) |
| ASIO buffer (`audio_asio_buffer`) | Panel: Audio | N/A | Windows-only (ASIO) |
| Audio buffer (`audio_buffer_ms`) | Panel: Audio | There | the Mac's own: in-game panel > AUDIO > Output (Windows-only (WASAPI shared Low latency mode) upstream) |
| Faster sound effects (`audio_early_keyon`) | Panel: Audio | There | in-game panel > AUDIO > Music and sound |
| Mute character voices (`mute_voices`) | Panel: Audio | There | in-game panel > AUDIO > Music and sound |
| Output device (`audio_device`) | Panel: Audio | N/A | Windows-only (Windows audio endpoints) |
| Menu sounds (`menusounds`) | Panel: Audio | There | in-game panel > AUDIO > Menu |
| Auto-open in-game overlay on startup (`autoopenoverlay`) | Panel: Game > In-game overlay | Added | Added: settings.ini autoopenoverlay opens the panel by itself 1.5 s after the game starts (fe/features/options.cpp); Settings window > Game and the panel's Game page. |
| Background input (`backgroundinput`) | Panel: Game > Input | There | in-game panel > GAME > Input |
| Game language (`gamelanguage`) | Panel: Game > Language | There | in-game panel > GAME > Language |
| Unlock everything (`unlock_all`) | Panel: Game | There | in-game panel > GAME > Language |
| Zelda starts as Sheik (`zelda_as_sheik`) | Panel: Game | There | in-game panel > GAME > Input |
| Show results screen (`results_screen`) | Panel: Game | There | in-game panel > GAME > Game |
| P1 shield..P4 shield (4 keys) (`shield_colorp1..p4`) | Panel: Game > Shield colours | There, row added | Settings window |
| Online (you) (`shield_color_online`) | Panel: Game > Shield colours | There, row added | the engine read it with no row; now Settings window |
| Offline input delay (`offline_delay / offline_delay_frames`) | Panel: Game > Input delay | There | in-game panel > GAME > Input delay |
| Online input delay (`onlinedelay / onlinedelayauto`) | Panel: Game > Input delay | There | as launcher.ini online_delay (Automatic or 1..9): launcher Settings, the Settings window and the panel |
| Early inputs (`early_inputs2`) | Panel: Game > Online | N/A | melee-unlocked-to-melee-unlocked only; the Mac netplay is Slippi 3.7.0's, whose early inputs are Input suggestions (there) |
| Quick chat (`quickchat`) | Panel: Game > Online | There | in-game panel > GAME > Online |
| Pause in Direct and Friends (`directpause`) | Panel: Game > Online | There, row added | the engine read it with no row; now Settings window |
| Port colour, first choice (`portcolour1`) | Panel: Game > Online | There, row added | the engine read it with no row; now Settings window |
| Port colour, second choice (`portcolour2`) | Panel: Game > Online | There, row added | the engine read it with no row; now Settings window |
| Skip in Unranked (3 connect-code fields) (`blockcode1..blockcode3`) | Panel: Game > Online | There | Practice menu > Search; now also Settings window > Game > Online, and passed to the netplay (fix: settings_net_prefs had no caller) |
| After an online session, straight back into Training and offer to search again (`online_training_after`) | Panel: Game > Online | There | in-game panel > GAME > Online |
| Warm-up panel look (`warmup_panel_style`) | Panel: Game > Online | N/A | melee-unlocked's own warm-up panel; the Mac warm-up is the Switch search module's |
| Leaving the search screen while searching (`keep_search`) | Panel: Game > Online | There | in-game panel > GAME > Online |
| Searching banner (`search_banner`) | Panel: Game > Online | N/A | melee-unlocked's searching banner looks; the Mac search UI is the Switch module's, one look |
| Warm-up button on the online character select (`warmup_button`) | Panel: Game > Online | N/A | melee-unlocked's warm-up button; the Mac warm-up starts with L+R on the select (Warm-up room row) |
| Arena: go back at once when it is your turn (no 3 second countdown) (`warmup_turn_instant`) | Panel: Game > Online | N/A | arena rooms: melee-unlocked-to-melee-unlocked only, not wired on the Mac |
| Arena: the room's match on the character select (`room_watch`) | Panel: Game > Online | N/A | arena rooms (see above) |
| Arena: the room's match while you're away (`warmup_watch`) | Panel: Game > Online | N/A | arena rooms (see above) |
| Small window size (`warmup_pip_size`) | Panel: Game > Online | N/A | arena rooms (see above) |
| Corner (`warmup_pip_corner`) | Panel: Game > Online | N/A | arena rooms (see above) |
| - (hidden: small window position/width set by dragging) (`warmup_pip_cx / warmup_pip_cy / warmup_pip_cw`) | Panel: Game > Online | N/A | arena window position (arena rooms are melee-unlocked-to-melee-unlocked only) |
| Arena: the room's match view (`spectate_view`) | Panel: Game > Online | N/A | arena rooms (see above) |
| After an online session ends, go back to Training mode (`online_return_training`) | Panel: Game > Online | There | in-game panel > GAME > Online |
| Spectator server (Slippi Launcher broadcast, overlays, experimental) (`slippi_spectator`) | Panel: Game > Online | There | in-game panel > GAME > Replays |
| Port (spectator) (`slippi_spectator_port`) | Panel: Game > Online | There, row added | the engine read it with no row; now Settings window |
| Room look (`arena/panel.txt field 1`) | Panel: Game > Online > Arena rooms | N/A | arena rooms (see above) |
| Arena button (`arena/panel.txt field 3`) | Panel: Game > Online > Arena rooms | N/A | arena rooms (see above) |
| Hide the room panels until the hand comes near (`arena/panel.txt field 2`) | Panel: Game > Online > Arena rooms | N/A | arena rooms (see above) |
| Application ID (`discord_app_id`) | Panel: Game > Discord | Added | Added: launcher.ini discord_app_id (digits, up to 24) goes to the presence as MELEE_DISCORD_APP_ID; empty uses the app's own; Settings window > Game > Discord. mu-engine/tools/discord_app_id_test.sh. |
| Discord presence (show what you are playing; friends can press Join) (`discordon`) | Panel: Game > Discord | Added | Row added: launcher.ini discord (the Mac's key) had no row; now Settings window > Game > Discord. |
| Freeze Final Destination (`freeze_fd`) | Panel: Game > Stages | There | in-game panel > GAME > Stages |
| Freeze Pokemon Stadium (offline only) (`stadium_frozen`) | Panel: Game > Stages | There | in-game panel > GAME > Stages |
| 20XX CPUs (`cpu_20xx`) | Panel: Game > Stages > CPU players | There | in-game panel > GAME > Computer players |
| CPU tech (`cpu_tech`) | Panel: Game > Stages > CPU players | There | in-game panel > GAME > Computer players |
| CPU getup (`cpu_getup`) | Panel: Game > Stages > CPU players | There | in-game panel > GAME > Computer players |
| CPU DI (`cpu_di`) | Panel: Game > Stages > CPU players | There | in-game panel > GAME > Computer players |
| CPU smash DI (`cpu_sdi`) | Panel: Game > Stages > CPU players | There | in-game panel > GAME > Computer players |
| CPUs never taunt (`cpu_notaunt`) | Panel: Game > Stages > CPU players | There | in-game panel > GAME > Computer players |
| CPUs always L-cancel (`cpu_lcancel`) | Panel: Game > Stages > CPU players | There | in-game panel > GAME > Computer players |
| CPU Falcon: no rapid jab (`cpu_nojab`) | Panel: Game > Stages > CPU players | There | in-game panel > GAME > Computer players |
| CPU Zelda and Sheik: no transform (`cpu_notransform`) | Panel: Game > Stages > CPU players | There | in-game panel > GAME > Computer players |
| Fountain of Dreams reflections (`fodreflections`) | Panel: Game > Stage effects | There, row added | the engine read it with no row; now Settings window |
| Character select background plays through (`cssbackgroundfree`) | Panel: Game > Stage effects | There, row added | the engine read it with no row; now Settings window |
| L-cancel flash (`lcancel_flash_mode`) | Panel: Game > L-cancel (also Mods > 20XX TE and Gecko codes page) | There | in-game panel > OVERLAYS > Overlays |
| Success flash (`lcancel_success_flash`) | Panel: Game > L-cancel | There | in-game panel > OVERLAYS > Overlays |
| - (legacy: old L-cancel indicator switch, still written) (`lcancelindicator`) | Panel: Game > L-cancel | There | in-game panel > OVERLAYS > Overlays |
| Auto L-cancel (`autolcancel`) | Panel: Game > L-cancel | There | in-game panel > GAME > L-cancel |
| Stock icon size (`stockhudscale`) | Panel: Game > HUD | There | in-game panel > GAME > HUD |
| Damage number size (`damagehudscale`) | Panel: Game > HUD | There | in-game panel > GAME > HUD |
| PAL style stock icons (`palstockicons`) | Panel: Game > HUD | There | in-game panel > GAME > HUD |
| Always show player tags (`playertags`) | Panel: Game > HUD | There | in-game panel > GAME > HUD |
| Disable screen shake (`noscreenshake`) | Panel: Game > HUD (also Gecko codes page; linked to TE Disable screen rumble) | There | in-game panel > GAME > HUD |
| Screen shake strength (`screenshake`) | Panel: Game > HUD | There | in-game panel > GAME > HUD |
| Hide KO explosions (`hide_ko_explosions`) | Panel: Game > HUD | There | in-game panel > GAME > HUD |
| Hide emblems behind percents (`hide_hud_emblems`) | Panel: Game > HUD | There | in-game panel > GAME > HUD |
| HUD transparency (`community_codes bit HUD_FADE 0x1000`) | Panel: Game > Community codes | There | in-game panel > GAME > Community codes |
| Show Nana's percent (`community_codes bit NANA_PERCENT 0x4000`) | Panel: Game > Community codes | There | in-game panel > GAME > Community codes |
| Ally indicators online (`community_codes bit ALLY_INDICATORS 0x8000`) | Panel: Game > Community codes | There | in-game panel > GAME > Community codes |
| Respawn platform by stocks (`community_codes bit ANGEL_STOCK_COLORS 0x2000`) | Panel: Game > Community codes | There | in-game panel > GAME > Community codes |
| Hide GO! at match start (`community_codes bit NO_GO_TEXT 0x10000`) | Panel: Game > Community codes | There | in-game panel > GAME > Community codes |
| V-cancel sound (`community_codes bit VCANCEL_SOUND 0x200000`) | Panel: Game > Community codes | There | in-game panel > GAME > Community codes |
| Hide fighter shadows (`community_codes bit NO_FIGHTER_SHADOWS 0x1000000`) | Panel: Game > Community codes | There | in-game panel > GAME > Community codes |
| Center the HUD for two players (`community_codes bit CENTER_2P_HUD 0x4000000`) | Panel: Game > Community codes | There | in-game panel > GAME > Community codes |
| Hitbox colours by ID (`community_codes bit HITBOX_ID_COLORS 0x20000`) | Panel: Game > Community codes | There | in-game panel > GAME > Community codes |
| Highest-priority hitbox on top (`community_codes bit HITBOX_ID_ORDER 0x40000`) | Panel: Game > Community codes | There | in-game panel > GAME > Community codes |
| Hitbox knockback angles (`community_codes bit HITBOX_ANGLES 0x80000`) | Panel: Game > Community codes | There | in-game panel > GAME > Community codes |
| Full Fountain of Dreams reflections (`community_codes bit HQ_FOD_REFLECTIONS 0x100000`) | Panel: Game > Community codes | There | in-game panel > GAME > Community codes |
| Instant Melee (`community_codes bit INSTANT_MELEE 0x400000`) | Panel: Game > Community codes | There | in-game panel > GAME > Community codes |
| Unrestricted pause camera (`community_codes bit FREE_PAUSE_CAMERA 0x800000`) | Panel: Game > Community codes | There | in-game panel > GAME > Community codes |
| Boot to character select (`community_codes bit BOOT_TO_CSS 0x2000000`) | Panel: Game > Community codes | There | in-game panel > GAME > Community codes |
| Controller rumble (`rumble`) | Panel: Controls | Added | Added: settings.ini rumble keeps every controller's motor still when off (host_shim.cpp input_rumble); each SDL pad keeps its own switch too. |
| P1..P4 card > Change (what plays as this port) (`port0..port3`) | Panel: Controls > Players | There | the Mac's own: in game (Windows-only (XInput sources) upstream) |
| - (hidden: HID box device name for its port) (`portname0..portname3`) | Panel: Controls > Players | N/A | Windows HID device names |
| Plugged in but not playing > Play as Pn (`-`) | Panel: Controls | Added | in-game CONTROLS > Plays as (First free port, P1..P4) for each controller: launcher.ini controller.<guid> port, taken at once (log: plays as P3 (its fixed port)) |
| Profile (Default / named / New / Delete) (`activeprofile0..activeprofile20`) | Panel: Controls > device editor | There | per controller: in-game CONTROLS > Controller profiles (ControllerProfiles/*.profile) |
| Use GameCube controller picture (`swpro_gc_picture`) | Panel: Controls > Switch controller | N/A | picture for melee-unlocked's own Switch Pro reader; SDL drives Switch pads on the Mac |
| Keyboard bindings (click a button in the picture, right-click clears) (`key_<Action> (27 keys)`) | Panel: Controls > Keyboard | There | the Mac's own: in game (Windows-only (Win32 VK codes) upstream) |
| Xbox controller bindings (`pad<0-3>_<Action> (4x27)`) | Panel: Controls > Xbox | There | the Mac's own: in game (Windows-only (XInput) upstream) |
| - (legacy single-pad format, migrated to pad0) (`pad_<Action>`) | Panel: Controls | N/A | Windows-only (XInput) |
| GameCube adapter bindings (`gc<0-3>_<Action> (4x27)`) | Panel: Controls > GameCube | There | per controller: launcher.ini padx.<id> m<N>/v<N>, in-game CONTROLS |
| PlayStation bindings (`ds4<0-3>_<Action> (4x27)`) | Panel: Controls > PlayStation | There | per controller (padx.<id>), in-game CONTROLS |
| Switch controller bindings (`swpro<0-3>_<Action> (4x27)`) | Panel: Controls > Switch | There | per controller (padx.<id>), in-game CONTROLS |
| B0XX / Frame1 / Box bindings (+ Box layouts: B0XX vJoy / HayBox / Generic) (`hid<0-3>_<Action> (4x27)`) | Panel: Controls > B0XX / Frame1 / Box | There | per controller (padx.<id>), in-game CONTROLS |
| Invert stick left/right, up/down, C-stick left/right, up/down (`hid<0-3>_invert`) | Panel: Controls > B0XX / Frame1 / Box > Box layouts | There | padx.<id> inv, in-game CONTROLS > Controller setup |
| Stick modifier push (`stick_modifier_percent`) | Panel: Controls (Keyboard and Box) | There | padx.<id> mod and the keyboard modifier, in-game CONTROLS |
| Control stick deadzone (`deadzone_<family>_main`) | Panel: Controls > Sticks | There | per controller (padx.<id> dz / controller.<guid> dz), in-game CONTROLS |
| C-stick deadzone (`deadzone_<family>_c`) | Panel: Controls > Sticks | There | per controller (dzc / cdz) |
| Control stick radius (`radius_<family>_main`) | Panel: Controls > Sticks | There | per controller (padx.<id> rad) |
| C-stick radius (`radius_<family>_c`) | Panel: Controls > Sticks | There | per controller (radc) |
| Snapback filter X / Y (`snapback_size_<family>_x / _y`) | Panel: Controls > Sticks | There | per controller (padx.<id> snx/sny) |
| for (ms) (`snapback_ms_<family>_x / _y`) | Panel: Controls > Sticks | There | per controller (snmx/snmy) |
| - (legacy 0.9.113 strength 1..10, still written) (`snapback_<family>_x / _y`) | Panel: Controls > Sticks | N/A | legacy strength key, still written upstream |
| L trigger / R trigger (`trigger_<family>_l / _r`) | Panel: Controls > Triggers | There | per controller (padx.<id> capl/capr) |
| L full press at / R full press at (`trigger_<family>_l_threshold / _r_threshold`) | Panel: Controls > Triggers | There | per controller (padx.<id> capl/capr) |
| <Action> (<source>) pressed at / press depth; list 'Press at %' (`<binding key>_level (key_/pad/gc/ds4/swpro/hid)`) | Panel: Controls > Per button / All buttons as a list | There | per controller (padx.<id> v<N>) |
| PhobGCC (Open PhobGCC menu / Controller test) (`Phob/port<N>.ini: enabled, phob_hardware, l_mode, r_mode, l_offset, r_offset, x/y_snapback, a/c x/y_waveshaping, a/c x/y_smoothing, a/c_cardinal, a/c_scaler, tourney, remap_*`) | Panel: Controls > GameCube > PhobGCC | There | in-game panel > CONTROLS > PhobGCC (GameCube adapter) (Adapter port row picks N) |
| - (legacy, read and ignored) (`editdevice / editdevice_follow`) | Panel: Controls | N/A | legacy, read and ignored upstream |
| Clean replays (`clean_replays`) | Panel: Overlays | Added | Added: replays play without the Arcade look's hitboxes (fe/settings.cpp word_display, display bits only). |
| Show player nicknames above fighters (`playernicknames`) | Panel: Overlays | There | in-game panel > OVERLAYS > Overlays |
| Show the "Settings: F1" reminder in menus (`settingsreminder`) | Panel: Overlays | There | in-game panel > OVERLAYS > Overlays |
| Show the note when skins are off online (`skin_online_notice`) | Panel: Overlays | There | in-game panel > OVERLAYS > Notices |
| Show Training mode's HUD (`training_hud`) | Panel: Overlays | There, row added | the engine read it with no row; now Settings window |
| Show the Practice menu hint in Training (`matchmakinghint`) | Panel: Overlays | There | in-game panel > GAME > Training |
| Practice menu look (`practicestyle`) | Panel: Overlays | There | in-game panel > GAME > Training |
| Performance overlay (`performance`) | Panel: Overlays | There | in-game panel > OVERLAYS > Overlays |
| FPS counter (top left) (`showfps`) | Panel: Overlays | There | in-game panel > OVERLAYS > Overlays |
| FPS counter (inside YouGame) (`showfps_yougame2`) | Panel: Overlays | Excluded | YouGame is left out |
| VSync (inside YouGame) (`vsync_yougame`) | Panel: Video | Excluded | YouGame is left out |
| Ping while online (under the FPS) (`showping`) | Panel: Overlays | There | the Performance HUD and the network overlay show the ping (FEATURES.md, Ping readout) |
| Network and timing (`netoverlay`) | Panel: Overlays | There | in-game panel > GAME > Online |
| Controller overlay (`inputoverlay`) | Panel: Overlays | There | in-game panel > OVERLAYS > Overlays |
| Show values (`inputoverlayvalues`) | Panel: Overlays | There | in-game panel > OVERLAYS > Overlays |
| Stick size (`inputoverlaystick`) | Panel: Overlays | There | in-game panel > OVERLAYS > Overlays |
| P1 P2 P3 P4 (`inputoverlayports`) | Panel: Overlays | There | in-game panel > OVERLAYS > Overlays |
| Hide background (`inputoverlayhideborder`) | Panel: Overlays | There | in-game panel > OVERLAYS > Overlays |
| - (overlay position/size, set by dragging) (`inputoverlayrect0..3`) | Panel: Overlays | N/A | position by dragging; the Mac input display has a fixed place and the Size row |
| Advanced (group open/closed) (`advancedoverlays`) | Panel: Overlays > Advanced | Added | same as advancedvideo |
| Render latency (under the FPS) (`reflexstats`) | Panel: Overlays > Advanced | N/A | Windows-only (Reflex/driver latency report) |
| Skip the 3D scene underneath (`labskipscene`) | Panel: Overlays > Advanced | N/A | Static Recomp only; the Mac runs the Source Port |
| Enable Legacy Menu (F11) (`legacymenu`) | Panel: Customize | There | in-game panel > CUSTOMIZE > Legacy menu |
| Legacy menu (`legacymenustyle`) | Panel: Customize | There | in-game panel > CUSTOMIZE > Legacy menu |
| Menu style (Choose menu style...) (`overlaystyle`) | Panel: Customize | There | in-game panel > CUSTOMIZE > Appearance |
| Color palette (`overlaypalette0..overlaypalette6`) | Panel: Customize | There | in-game panel > CUSTOMIZE > Appearance |
| Window transparency (`settingstransparency`) | Panel: Customize | There | in-game panel > CUSTOMIZE > Appearance |
| Use custom accent (`menucustom`) | Panel: Customize > Custom menu accent | There | in-game panel > CUSTOMIZE > Appearance |
| Show colour picker (`menucolor`) | Panel: Customize > Custom menu accent | There | in-game panel > CUSTOMIZE > Appearance |
| Disable Screen Shake (Slippi's codes, built in) (`noscreenshake`) | Panel: Gecko Codes (Source Port page) | There | in-game panel > GAME > HUD |
| L-cancel flash (Slippi's codes, built in) (`lcancel_flash_mode`) | Panel: Gecko Codes (Source Port page) | There | in-game panel > OVERLAYS > Overlays |
| Your codes (one toggle per code in GeckoCodes.ini) (`geckocode`) | Panel: Gecko Codes | There | the Mac's own: in-game panel > GECKO CODES > Your codes (Static Recomp upstream) |
| - (hidden: the panel has saved a Gecko choice) (`geckochosen`) | Panel: Gecko Codes | There | in-game panel > GECKO CODES > Your codes |
| + Add Gecko code (Name, Code lines) / - remove (`- (GeckoCodes.ini)`) | Panel: Gecko Codes | N/A | Static Recomp only; the Mac runs the Source Port |
| On / Off (per detected pack in Mods folder) (`mod_enabled <id>`) | Panel: Mods > Game mods | There | as the mod profile: Mods page Use / In use and in-game MODS (one profile per start) |
| On / Off (20XX Tournament Edition save) (`mod_te_enabled`) | Panel: Mods > Game mods | There, row added | the engine read it with no row; now Settings window |
| On / Off (Training Mode CE disc) (`mod_tmce_enabled`) | Panel: Mods > Game mods | There | as the mod profile (Use on Training Mode CE) |
| - (active mod profile name; Restart to apply profile) (`mod_profile`) | Panel: Mods | There | Launcher Mods page (Use / In use); in-game panel > MODS > Next start |
| Use this mod in Direct (`mods_in_direct`) | Panel: Mods | N/A | a Mac mod profile plays offline only; a session that can go online plays the retail files |
| Use 20XX TE features (`te_options bit 0x10`) | Panel: Mods > 20XX Tournament Edition | There | game's own menu (20XX TE / Hack Pack, in game) |
| L-cancel flash / Success flash (`lcancel_flash_mode / lcancel_success_flash`) | Panel: Mods > 20XX Tournament Edition | There | in-game panel > OVERLAYS > Overlays |
| Tournament Mode (`te_options bit 0x20`) | Panel: Mods > 20XX Tournament Edition > 20XX TE settings | There | game's own menu (20XX TE / Hack Pack, in game) |
| Pausing requires holding Start (`te_options bit 0x40`) | Panel: Mods > 20XX Tournament Edition > 20XX TE settings | There | game's own menu (20XX TE / Hack Pack, in game) |
| Frozen stages (`te_options bit 0x80`) | Panel: Mods > 20XX Tournament Edition > 20XX TE settings | There | game's own menu (20XX TE / Hack Pack, in game) |
| CPU Zelda is Sheik (`te_options bit 0x2000`) | Panel: Mods > 20XX Tournament Edition > 20XX TE settings | There | game's own menu (20XX TE / Hack Pack, in game) |
| Handicap is stock count (`te_options bit 0x4000`) | Panel: Mods > 20XX Tournament Edition > 20XX TE settings | There | game's own menu (20XX TE / Hack Pack, in game) |
| Disable Star KO (`te_options bit 0x100`) | Panel: Mods > 20XX Tournament Edition > 20XX TE settings | There | game's own menu (20XX TE / Hack Pack, in game) |
| Keep playing after GAME! (`te_options bit 0x800`) | Panel: Mods > 20XX Tournament Edition > 20XX TE settings | There | game's own menu (20XX TE / Hack Pack, in game) |
| Taunt cancelling (`te_options bit 0x400`) | Panel: Mods > 20XX Tournament Edition > 20XX TE settings | There | game's own menu (20XX TE / Hack Pack, in game) |
| Infinite shields (`te_options bit 0x200`) | Panel: Mods > 20XX Tournament Edition > 20XX TE settings | There | game's own menu (20XX TE / Hack Pack, in game) |
| Fixed camera (`te_options bit 0x1000`) | Panel: Mods > 20XX Tournament Edition > 20XX TE settings | There | game's own menu (20XX TE / Hack Pack, in game) |
| Stage striking (`te_options2 bit 0x20000`) | Panel: Mods > 20XX Tournament Edition > 20XX TE settings | There | game's own menu (20XX TE / Hack Pack, in game) |
| Frozen Mode toggle (`te_options2 bit 0x10`) | Panel: Mods > 20XX Tournament Edition > 20XX TE settings | There | game's own menu (20XX TE / Hack Pack, in game) |
| Reset tournament rules (`te_options2 bit 0x8`) | Panel: Mods > 20XX Tournament Edition > 20XX TE settings | There | game's own menu (20XX TE / Hack Pack, in game) |
| v1.00 game rules (`te_options2 bit 0x2`) | Panel: Mods > 20XX Tournament Edition > 20XX TE settings | There | game's own menu (20XX TE / Hack Pack, in game) |
| Quieter Dream Land 64 music (`te_options2 bit 0x4`) | Panel: Mods > 20XX Tournament Edition > 20XX TE settings | There | game's own menu (20XX TE / Hack Pack, in game) |
| Lock these settings (`te_options2 bit 0x40000`) | Panel: Mods > 20XX Tournament Edition > 20XX TE settings | There | game's own menu (20XX TE / Hack Pack, in game) |
| Hand-warmer mode (`te_options2 bit 0x10000`) | Panel: Mods > 20XX Tournament Edition > 20XX TE settings | There | game's own menu (20XX TE / Hack Pack, in game) |
| Random stage music (`te_options2 bit 0x40`) | Panel: Mods > 20XX Tournament Edition > 20XX TE settings | There | game's own menu (20XX TE / Hack Pack, in game) |
| Extra shield colors (`te_options2 bit 0x80`) | Panel: Mods > 20XX Tournament Edition > 20XX TE settings | There | game's own menu (20XX TE / Hack Pack, in game) |
| Disable screen rumble (linked to Disable screen shake) (`te_options2 bit 0x100`) | Panel: Mods > 20XX Tournament Edition > 20XX TE settings | There | game's own menu (20XX TE / Hack Pack, in game) |
| Spoof controller plugins (`te_options2 bit 0x400`) | Panel: Mods > 20XX Tournament Edition > 20XX TE settings | There | game's own menu (20XX TE / Hack Pack, in game) |
| Collision bubbles (`te_options2 bit 0x1000`) | Panel: Mods > 20XX Tournament Edition > 20XX TE settings | There | game's own menu (20XX TE / Hack Pack, in game) |
| Input display (linked to Controller overlay) (`te_options2 bit 0x2000`) | Panel: Mods > 20XX Tournament Edition > 20XX TE settings | There | game's own menu (20XX TE / Hack Pack, in game) |
| CPU smart DI (`te_options2 bit 0x4000`) | Panel: Mods > 20XX Tournament Edition > 20XX TE settings | There | game's own menu (20XX TE / Hack Pack, in game) |
| Color overlays (`te_options2 bit 0x8000`) | Panel: Mods > 20XX Tournament Edition > 20XX TE settings | There | game's own menu (20XX TE / Hack Pack, in game) |
| - (20XX TE menu music choice, set in the game's TE menu) (`te_menu_music`) | Panel: Mods > 20XX Tournament Edition | There | game's own menu (20XX TE / Hack Pack, in game) |
| - (20XX Hack Pack menu choices: playlists, stage variants/page, training displays; set in-game) (`hp_settings`) | Panel: Mods | There | game's own menu (20XX TE / Hack Pack, in game) |
| Random installed stage skin each match (`cosmetics profile.json: random_stage_skins`) | Panel: Mods > Cosmetic mods | There, row added | the engine read it with no row; now Settings window |
| <pack>: N stage skins for M stages, in the random pick (`cosmetics profile.json: per stage pack random`) | Panel: Mods > Cosmetic mods | There, row added | the engine read it with no row; now Settings window |
| Import / Refresh... / Add stage DAT... (Stage to replace) / Add portrait... (Fighter, Costume, Picture) / Refresh catalog (`-`) | Panel: Mods > Cosmetic mods | Added | Settings window > Mods > Cosmetic mods: Import… (costume and stage .dat files or a .zip of them), Add stage DAT… (Stage to replace), Open folder. Not here: Add portrait (the Mac skin picker shows names, not pictures) and Refresh (skins are read at each start) |
| Use cosmetic profile (`cosmetics profile.json: enabled`) | Panel: Mods > Cosmetic mods | There, row added | the engine read it with no row; now Settings window |
| Costume packs > Costume set (fill every slot a pack covers) (`cosmetics profile.json: pack set`) | Panel: Mods > Cosmetic mods | There, row added | the engine read it with no row; now Settings window |
| Characters > <fighter> > <costume> variant (`cosmetics profile.json: per-costume selection`) | Panel: Mods > Cosmetic mods | There, row added | the engine read it with no row; now Settings window |
| Pause screen / Portraits and stock icons / Stages / Fighter move sounds / Effects (Enable project effects) (`cosmetics profile.json: resource slots`) | Panel: Mods > Cosmetic mods | There, row added | the engine read it with no row; now Settings window |
| Restore Vanilla / Restart to apply mods (`-`) | Panel: Mods > Cosmetic mods | There | Mods page Use / In use (In use goes back to the retail game); a profile loads at the next start |
| Practice CPU (`practicecpu: enabled`) | Practice menu (Training, Start+D-Up+Z) > CPU | There | the Practice menu (Training, Start + D-pad Up + Z), settings.ini practicecpu and slots |
| Only react to hits (`practicecpu: freeze`) | Practice menu (Training, Start+D-Up+Z) > CPU | There | the Practice menu (Training, Start + D-pad Up + Z), settings.ini practicecpu and slots |
| Moving target (no attacks) (`practicecpu: no_attacks`) | Practice menu (Training, Start+D-Up+Z) > CPU | There | the Practice menu (Training, Start + D-pad Up + Z), settings.ini practicecpu and slots |
| Aggression (`practicecpu: aggression`) | Practice menu (Training, Start+D-Up+Z) > CPU > Style | There | the Practice menu (Training, Start + D-pad Up + Z), settings.ini practicecpu and slots |
| Attack range (`practicecpu: attack_range`) | Practice menu (Training, Start+D-Up+Z) > CPU > Style | There | the Practice menu (Training, Start + D-pad Up + Z), settings.ini practicecpu and slots |
| Reaction time (`practicecpu: reaction_frames`) | Practice menu (Training, Start+D-Up+Z) > CPU > Style | There | the Practice menu (Training, Start + D-pad Up + Z), settings.ini practicecpu and slots |
| CPU delay (`practicecpu: act_delay`) | Practice menu (Training, Start+D-Up+Z) > CPU > Style | There | the Practice menu (Training, Start + D-pad Up + Z), settings.ini practicecpu and slots |
| Respawn platform (`practicecpu: respawn`) | Practice menu (Training, Start+D-Up+Z) > CPU > Style | There | the Practice menu (Training, Start + D-pad Up + Z), settings.ini practicecpu and slots |
| Movement weights (walk/dash/hops/wavedash... + Waveland/Shield drop/Dash jump/Run past you/Turn around) (`practicecpu: move_odds + move2_odds`) | Practice menu (Training, Start+D-Up+Z) > CPU > Movement | There | the Practice menu (Training, Start + D-pad Up + Z), settings.ini practicecpu and slots |
| Attacks weights (20 attacks + Run shine) (`practicecpu: attack_odds + run_shine`) | Practice menu (Training, Start+D-Up+Z) > CPU > Attacks | There | the Practice menu (Training, Start + D-pad Up + Z), settings.ini practicecpu and slots |
| Throws (F/B/U/D throw weights) / Throw wait (`practicecpu: throw_odds / throw_wait`) | Practice menu (Training, Start+D-Up+Z) > CPU > Throws | There | the Practice menu (Training, Start + D-pad Up + Z), settings.ini practicecpu and slots |
| Drift / Aerial timing (`practicecpu: jump_drift / aerial_timing`) | Practice menu (Training, Start+D-Up+Z) > CPU > Jumps and aerials | There | the Practice menu (Training, Start + D-pad Up + Z), settings.ini practicecpu and slots |
| Full hop / Double jump / Fast fall / Fast fall delay / L-cancel (`practicecpu: full_hop_aerials / double_jump / fast_fall / fast_fall_delay / l_cancel`) | Practice menu (Training, Start+D-Up+Z) > CPU > Jumps and aerials | There | the Practice menu (Training, Start + D-pad Up + Z), settings.ini practicecpu and slots |
| When you attack near it (Nothing/Shield/Spot dodge/Roll in/Roll away/Jump away + roll to ledge/middle) (`practicecpu: defend_odds + def2_odds`) | Practice menu (Training, Start+D-Up+Z) > Defense | There | the Practice menu (Training, Start + D-pad Up + Z), settings.ini practicecpu and slots |
| Out of shield (10 options) (`practicecpu: oos_odds`) | Practice menu (Training, Start+D-Up+Z) > Defense | There | the Practice menu (Training, Start + D-pad Up + Z), settings.ini practicecpu and slots |
| DI weights / Smash DI (None/Light/Medium/Heavy) / Floorhug (`practicecpu: di_odds / sdi_amount / floorhug`) | Practice menu (Training, Start+D-Up+Z) > Hit reactions | There | the Practice menu (Training, Start + D-pad Up + Z), settings.ini practicecpu and slots |
| Tech / After a missed tech / As hitstun ends in the air (`practicecpu: tech_odds + tech2_odds / getup_odds + getup2_odds / after_odds`) | Practice menu (Training, Start+D-Up+Z) > Hit reactions | There | the Practice menu (Training, Start + D-pad Up + Z), settings.ini practicecpu and slots |
| From the ledge / Most time on the ledge / Recovery (Game's own CPU/High/Low/Mix) (`practicecpu: ledge_odds / ledge_delay / recovery`) | Practice menu (Training, Start+D-Up+Z) > Ledge | There | the Practice menu (Training, Start + D-pad Up + Z), settings.ini practicecpu and slots |
| Save states and recording on the D-pad / Loop the recording (`practicecpu: tools DPAD/LOOP`) | Practice menu (Training, Start+D-Up+Z) > Tools | There | the Practice menu (Training, Start + D-pad Up + Z), settings.ini practicecpu and slots |
| Random numbers (Game's own/Same each Training start/Frozen) / Seed (`practicecpu: rng_mode / rng_seed`) | Practice menu (Training, Start+D-Up+Z) > Tools | There | the Practice menu (Training, Start + D-pad Up + Z), settings.ini practicecpu and slots |
| Frame advance (L+R+Z) (`practicecpu: tools FRAME_ADVANCE`) | Practice menu (Training, Start+D-Up+Z) > Tools | There | the Practice menu (Training, Start + D-pad Up + Z), settings.ini practicecpu and slots |
| Hitboxes and hurtboxes / Hitboxes only / Projectile hitboxes / Combo counter / Inputs and frame advantage (`practicecpu: tools HITBOXES/HITBOXES_ONLY/PROJECTILE_HITBOXES/COMBO_COUNTER/OVERLAY`) | Practice menu (Training, Start+D-Up+Z) > Tools | There | the Practice menu (Training, Start + D-pad Up + Z), settings.ini practicecpu and slots |
| Presets (8 cards) / Reset everything to the defaults (`practicecpu: preset`) | Practice menu (Training, Start+D-Up+Z) > Presets | There | the Practice menu (Training, Start + D-pad Up + Z), settings.ini practicecpu and slots |
| Your setups: Setup 1..3 (Save here / Load) (`practicecpuslot1..3`) | Practice menu (Training, Start+D-Up+Z) > Presets | There | in-game panel > GAME > Training; Practice menu > Your setups |
| - (pro AI 'stronger' choice, asked in the Practice menu) (`proaistronger`) | Practice menu (Training, Start+D-Up+Z) | There | Practice menu > Fight a pro > Stronger Pro AI |
| - (hidden: times the Practice menu was opened; hint stops after use) (`practiceopens`) | Practice menu (Training, Start+D-Up+Z) | There | none (counter) |
| - (old Training Lab bits, masked off at load; rebuilt from Practice tools) (`te_options lab bits 0x8000+`) | Panel: Mods | There | game's own menu (20XX TE / Hack Pack, in game) |
| - (removed setting, read and ignored) (`onlineframeprotection`) | Panel: Video | N/A | removed upstream, read and ignored |
| - (panel reopens on the last category, page and expanded sections) (`settings-view.txt: tab/page/node`) | Panel (all) | N/A | remembers the panel's last page; not a setting |
| MELEE NTSC 1.02 DISC IMAGE (Browse...) (`iso`) | Launcher: Play page | There | Launcher Play page |
| GAME BUILD: Source Port / Static Recomp (Legacy) (`engine`) | Launcher: Play page | There | Launcher Play page |
| GAME BUILD: Vanilla (`vanilla`) | Launcher: Play page | There | in-game panel > GECKO CODES > Built-in codes |
| Game's own code only (no UCF, nothing unlocked) (`vanillaowncode`) | Launcher: Play page (Vanilla) | There | the Mac's Vanilla build is always the game's own code (no codes, nothing unlocked) |
| Choose version... (`activeversion`) | Launcher: Play page | N/A | Windows-only (installs Windows release builds) |
| Launcher color (`accent`) | Launcher: Play page | There | Launcher Play page |
| Language (`language`) | Launcher: Play page | There | Launcher Play page |
| Rail picture (click Bailey: Choose a picture... / Bring back Bailey) (`- (rail-image.png/jpg/...)`) | Launcher: rail | There | click Bailey on the rail |
| Send recent game logs (`-`) | Launcher: Play page | Excluded | crash upload to melee-unlocked's relay is left out; Save crash report keeps it on this Mac |
| Crash report offer after a crash (`-`) | Launcher: Play page | Excluded | crash upload to melee-unlocked's relay is left out; Save crash report keeps it on this Mac |
| Update and restart / Get Slippi Launcher (`-`) | Launcher: Play page | N/A | Windows-only (Windows release installer) |
| Launch with YouGame extension (`yougame_extension`) | Launcher: Play page (YouGame row) | Excluded | YouGame is left out |
| YouGame: Connect / Disconnect / Open YouGame (`-`) | Launcher: Play page (YouGame row) | Excluded | YouGame is left out |
| - (hidden: connected through the YouGame app before / declined) (`yougame_app`) | Launcher: Play page | Excluded | YouGame is left out |
| - (hidden: staging environment switch) (`yougame_env`) | Launcher: Play page | Excluded | YouGame is left out |
| - (hidden: protected YouGame session) (`yougame_session`) | Launcher: Play page | Excluded | YouGame is left out |
| - (hidden: What's new already shown for version) (`notesseen`) | Launcher: Play page | There | none |
| - (hidden: DLSS 5 download offered once) (`dlss5asked`) | Launcher: Play page | N/A | Windows-only (NVIDIA DLSS 5) |
| - (hidden override: standard vs compatibility (no AVX2) build) (`cpubuild`) | Launcher: Play page | N/A | Windows-only (x86 AVX2 compat exe) |
| Settings (`- (opens the panel)`) | Launcher: rail > Settings | Added | Added: All settings… on the launcher's Settings page opens the Settings window (port/runtime/host/mu_settings_window.mm). |
| Warm cache from remembered ISO before Play (`warmcache`) | Launcher: Build page | N/A | D3D pipeline warm-up; the Mac compiles Metal pipelines in the background |
| Save replays (`savereplays`) | Launcher: Replay Viewer page | There | in-game panel > GAME > Replays |
| Keep replays compressed (.slpz) (+ Compress all replays now) (`compressreplays`) | Launcher: Replay Viewer page (replay context menu) | There | in-game panel > GAME > Replays |
| Replay folder: Choose folder... / Open / Use the Replays folder next to the launcher (`replaydir`) | Launcher: Replay Viewer page | Added | Added: launcher.ini replaydir (melee-unlocked's key); the launcher's Replay Viewer and the engine (--replay-dir) use it. |
| Sort: Most recent/Least recent/Longest game/Shortest game (`replaysort`) | Launcher: Replay Viewer page | There | in-game panel > GAME > Replays |
| Hide short games (`replayhideshort`) | Launcher: Replay Viewer page | There | in-game panel > GAME > Replays |
| Show it as it was played (uses the .trace file) (`replayasplayed`) | Launcher: Replay Viewer page | There | in-game panel > GAME > Replays |
| Search players, characters, stages, files / Watch live / queue (`-`) | Launcher: Replay Viewer page | There | Watch live and Open a replay are there; the search field is not |
| Open to: Vanilla/Akaneia/ACE/Custom ISO (`lobbyopen`) | Launcher: Multiplayer Lobby page | Excluded | the BitTorrent Multiplayer Lobby is left out (done on the Switch first) |
| Custom ISO (for Open to: Custom) (`lobbyiso / lobbyisoname`) | Launcher: Multiplayer Lobby page | Excluded | the BitTorrent Multiplayer Lobby is left out (done on the Switch first) |
| Display name (`lobby-profile.json: name`) | Launcher: Multiplayer Lobby page > Profile | Excluded | the BitTorrent Multiplayer Lobby is left out (done on the Switch first) |
| Location (`lobby-profile.json: location`) | Launcher: Multiplayer Lobby page > Profile | Excluded | the BitTorrent Multiplayer Lobby is left out (done on the Switch first) |
| Your three mains (`lobby-profile.json: mains`) | Launcher: Multiplayer Lobby page > Profile | Excluded | the BitTorrent Multiplayer Lobby is left out (done on the Switch first) |
| Connection (`lobby-profile.json: mode`) | Launcher: Multiplayer Lobby page > Profile | Excluded | the BitTorrent Multiplayer Lobby is left out (done on the Switch first) |
| Bootstrap peer (optional) (`lobby-profile.json: url`) | Launcher: Multiplayer Lobby page > Profile | Excluded | the BitTorrent Multiplayer Lobby is left out (done on the Switch first) |
| - (advanced ports) (`lobby-profile.json: udp_port / peer_port`) | Launcher: Multiplayer Lobby page > Profile | Excluded | the BitTorrent Multiplayer Lobby is left out (done on the Switch first) |
| Auto reject invites: On/Off (`lobby-profile.json: auto_reject_invites`) | Launcher: Multiplayer Lobby page | Excluded | the BitTorrent Multiplayer Lobby is left out (done on the Switch first) |
| Request sound: On/Off (`lobby-profile.json: request_sound`) | Launcher: Multiplayer Lobby page | Excluded | the BitTorrent Multiplayer Lobby is left out (done on the Switch first) |
| Request volume (`lobby-profile.json: request_volume`) | Launcher: Multiplayer Lobby page | Excluded | the BitTorrent Multiplayer Lobby is left out (done on the Switch first) |
| Match fighter / color (first main by default) (`lobby-profile.json: p2p_ch / p2p_col`) | Launcher: Multiplayer Lobby page | Excluded | the BitTorrent Multiplayer Lobby is left out (done on the Switch first) |
| On/Off switch per installed mod (`mod_enabled / mod_te_enabled / mod_tmce_enabled (port-settings.ini)`) | Launcher: Mods page | There | as the mod profile: Mods page Use / In use and in-game MODS (one profile per start) |
| Get / Update / Remove / Choose skins (opens Settings > Mods) (`-`) | Launcher: Mods page | N/A | Windows-only (xdelta3.exe builds) |
