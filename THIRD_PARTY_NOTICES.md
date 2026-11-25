# Licences and third-party notices

melee-unlocked-macOS (which grew out of Dashdance) is distributed under the GNU General Public License. Its own code is
version 2 or (at your option) any later version (GPL-2.0-or-later); the app as built includes melee-unlocked's engine, which
is GPL-3.0-or-later, so builds are distributed under GPL-3.0. The full GPLv2 text is in [LICENSE](LICENSE). This page lists what the project is built from, the licence
each part carries, and the evidence for it, so the licensing can be checked rather than taken on trust. It was compiled
from the upstream repositories on 2026-09-14. It is a factual record, not legal advice.

## The project itself

| Source | What this project uses | Licence | Evidence |
|---|---|---|---|
| [Hero88go/melee-unlocked](https://github.com/Hero88go/melee-unlocked) | The fork's base (commit `b8360b1`): the GX decoder and host layers in `port/runtime` that the app's renderer and input still use. The static recompiler it started from was removed | GPL-2.0-or-later | Its README at that commit, section "License": "GPL-2.0-or-later." |
| [Hero88go/melee-unlocked](https://github.com/Hero88go/melee-unlocked) v1.0.5 (commit `67868aa`) | The melee-unlocked engine (`mu-engine/`): its native game (the Melee decompilation's C, built from source at build time) and its headless host services, fetched by `mu-engine/fetch.sh` and changed only by the build-time patches in `mu-engine/patches`; nothing from it is committed | GPL-3.0-or-later (builds with the melee-unlocked engine are therefore GPL-3.0) | Its README, section "License": "GPL-3.0-or-later, starting with 0.8.5"; its `LICENSE` is the GPLv3 text. |
| Melee-NX-NetPlay (same author as this fork) | `mu-engine/runner` (ELF loader, runner, rollback store, netplay host), `mu-engine/fetch.sh`, the patches `0001`-`0004`, `0011` and `decomp-0001`-`0003`, the mod installer (`mu-engine/modinstall`, `mu-engine/tools/mod_install_test.*`), the replays module (`mu-engine/app/replays.cpp`) and Slippi menu files (`mu-engine/app/slippi_menus.cpp`), the reference digests and `mu-engine/tools`, its Slippi 3.7.0 netplay (`mu-engine/net`: files of its `port/runtime/hle` and `port/runtime/host/md5.h`, its ENet changes, its `patches/net-0001`-`0012`), `mu-engine/net_sources.sh`, `tools/mac/mock_matchmaking_ws.py`, `tools/mac/mock_slippi_http.py`, the controller scripts in `mu-engine/tests/online`, adapted from its `experiments/native-engine`; `port/runtime/host/input_script.cpp` and `.h` (`@rearm`) from its `port/`; the looks (`mu-engine/app/fe/looks_live.h`, `fe/features/looks.cpp`, and `mu-engine/app/gx_looks_mac.cpp` adapted from its `frontend/patches/0116-gx-looks.patch`, which adapts melee-unlocked's GPL-2.0-or-later look rules), skins (`fe/features/cosmetics.cpp`, which compiles melee-unlocked's pack_skin_rule and skeleton checks from the fetched tree at build time) and `mu-engine/patches/0013` (its `patches/0007`) | GPL-3.0-or-later | Its source files carry `SPDX-License-Identifier: GPL-3.0-or-later`. |
| [xdelta](https://github.com/jmacd/xdelta) 3.1.0 (Joshua P. MacDonald) | The DJW secondary decoder in `mu-engine/modinstall/mi_vcdiff.cpp` (from Melee-NX-NetPlay) is adapted from its `xdelta3/xdelta3-djw.h` and `xdelta3-second.h`; the rest of the VCDIFF decoder is written from RFC 3284; no xdelta3 program is shipped or run | GPL-2.0-or-later, used here under GPL-3.0-or-later | The files' headers at tag v3.1.0: "either version 2 of the License, or (at your option) any later version". |
| [LZMA SDK](https://www.7-zip.org/sdk.html) 25.01 (Igor Pavlov) | `C/LzmaDec.c`, `C/Lzma2Dec.c` and their headers, compiled into the mod installer from the copy in the fetched melee-unlocked tree (`port/third_party/lzma`); not committed | Public domain | The files' headers: "Igor Pavlov : Public domain". |
| zlib | Its raw inflate for the mod installer's zip downloads: the system's library | zlib | The zlib licence. |
| melee-unlocked's mod catalog and its rules | `mu-engine/modinstall/mi_catalog.cpp` follows its `port/app/launcher_mod_catalog.cpp` (field checks; automatic download only with a recorded licence or permission); its `mods-catalog-2.json` is read from the fetched tree at build time and built into the app, not committed | GPL-2.0-or-later, used here under GPL-3.0-or-later | Header `SPDX-License-Identifier: GPL-2.0-or-later`. |
| [Dear ImGui](https://github.com/ocornut/imgui) v1.92.8 (Omar Cornut) | Its Metal back end (`backends/imgui_impl_metal.{h,mm}`), fetched by `mu-engine/fetch.sh` at the version melee-unlocked vendors, for melee-unlocked's ImGui panel on the Mac renderer; not committed | MIT | Its `LICENSE.txt`: "The MIT License (MIT) / Copyright (c) 2014-2025 Omar Cornut". |
| melee-unlocked's settings panel | `mu-engine/app/ui/mu_panel.cpp`, `mu_mods.cpp` and `mu_ui_extract.py` (from Melee-NX-NetPlay's frontend, same author as this fork) compile melee-unlocked's own panel code, copied verbatim from the fetched tree at build time by `mu_ui_extract.py` (`port/runtime/gx/pc_settings_visuals.inl`, `settings_page_navigation.h`, `settings_imgui_input.h`, `settings_search.h`, `radial_navigation.h`, blocks of `pc_settings.cpp` (every menu style's drawing) and `pc_settings_shared.h`, constants of `port/app/launcher.cpp` and `launcher_theme.h`); its `port/app/bailey_cut.bmp` and `melee_mark.bmp`, the GD Melee kit's pictures and font pages (`ui_sources/gd_melee/assets/kit/*.png`, see its `ORIGIN.md`) and the menu style gallery's screenshots (`ui_sources/menu_previews/assets`) are embedded at build time; nothing of it is committed | GPL-2.0-or-later | Each file's header `SPDX-License-Identifier: GPL-2.0-or-later`; melee-unlocked's README, "License": "GPL-2.0-or-later." |
| melee-unlocked's replay data, stats and session trace view | `mu-engine/app/replay_stats_facade.cpp` compiles its `port/app/launcher_replay_data.cpp` (with `launcher_replay_stats.inl`, `launcher_replay_lag.inl`), `launcher_trace_view.cpp`, `replay_slpz.cpp` and `port/runtime/host/net_trace_read.cpp` from the fetched tree into the launcher for the Replay Viewer's card, as the engine's replay library compiles them; nothing of it is committed | GPL-2.0-or-later | Each file's header `SPDX-License-Identifier: GPL-2.0-or-later`; melee-unlocked's README, "License". |
| melee-unlocked's character select easter egg | `mu-engine/app/fe/egg.cpp` (from Melee-NX-NetPlay's frontend) compiles its `port/runtime/gx/walter_egg.cpp`, `walter_egg.h` and `walter_egg_picture.h`, copied from the fetched tree at build time with three include lines renamed; nothing of it is committed | GPL-2.0-or-later | `walter_egg.h`: `SPDX-License-Identifier: GPL-2.0-or-later`. |
| [Dear ImGui](https://github.com/ocornut/imgui) v1.92.8 core | `imgui.cpp`, `imgui_draw.cpp`, `imgui_tables.cpp`, `imgui_widgets.cpp` from the fetched melee-unlocked tree (`port/third_party/imgui`), compiled into the engine for the panel; not committed | MIT | Its `LICENSE.txt`: "The MIT License (MIT) / Copyright (c) 2014-2025 Omar Cornut". |
| [Source Sans 3](https://github.com/adobe-fonts/source-sans) Black and Bold (Adobe) | The panel's fonts, from melee-unlocked's `port/runtime/gx/ui_sources/gd_melee/assets/kit/`, embedded at build time; not committed | SIL Open Font License 1.1 | "Copyright 2010-2024 Adobe (http://www.adobe.com/), with Reserved Font Name 'Source'. [...] This Font Software is licensed under the SIL Open Font License, Version 1.1." (`SourceSans3-OFL.md` beside the fonts) |
| Melee-NX-NetPlay's frontend layer (same author as this fork) | `mu-engine/app/fe`: `settings.cpp`, `mods.cpp`, `app_menu.cpp`, `overlay_draw.cpp`, the feature modules in `fe/features` (options, Training, replays and their library and viewer, overlays, search, spectator, network overlay, Fight a pro), `fe/proai` (its own ONNX engine `onnx_lite`, written from the ONNX specification, and the onnxruntime-shaped calls `ort_lite`), `mu-engine/runner/features/snapshot_store.*`; `app_menu.cpp` and `mods.cpp` changed for the Mac where marked `mumac` | GPL-3.0-or-later | Each file's header `SPDX-License-Identifier: GPL-3.0-or-later`. |
| melee-unlocked's Practice menu and Fight a pro | Its `port/runtime/gx/practice_menu.cpp` and `port/runtime/hle/phillip_ai.cpp` compiled from the fetched tree with `mu-engine/app/fe/patches/0112` and `0113` (build-time copies by `fe/gen_fe.sh`; nothing copied into this repository); its replay coach (`replay_coach.cpp`, `replay_scan.cpp`), replay writer pieces (`replay_slpz.cpp`, `launcher_replay_data.cpp`) and `native_replay_stream.cpp` compiled from the fetched tree by the feature modules | GPL-2.0-or-later | Headers `SPDX-License-Identifier: GPL-2.0-or-later`. |
| [slippi-ai](https://github.com/vladfi1/slippi-ai) (Phillip, Vlad Firoiu) | The policy input and output packing that melee-unlocked's `phillip_ai.cpp` follows | MIT | Its `LICENSE`: "MIT License / Copyright (c) 2020 Vlad Firoiu". |
| [vladfi/phillip-models](https://huggingface.co/vladfi/phillip-models) (Hugging Face) | Phillip's published ONNX policies: never committed or shipped; the engine downloads the ones the player asks for from the pinned URLs in `mu-engine/app/fe/proai/catalog.json` (SHA-256 checked), or the player copies them to `ProAI/` | MIT | The model card's metadata: "license: mit". |
| [libmelee](https://github.com/altf4/libmelee) | Randall's positions on Yoshi's Story, as the table melee-unlocked generated (`port/runtime/hle/phillip_randall.h`), compiled from the fetched tree | LGPL-3.0 (GPL-3.0 compatible) | The generated header: "from libmelee's melee.stages.randall_position (LGPL-3.0, https://github.com/altf4/libmelee)". |
| [zstd](https://github.com/facebook/zstd) 1.5.7 | The copy melee-unlocked vendors (`port/third_party/zstd`), compiled into the engine for `.slpz` replays; not committed | BSD-3-Clause (or GPL-2.0) | Its `LICENSE` (BSD) and the fetched tree's `SOURCES.txt`. |
| Dolphin's free DSP coefficient table | melee-unlocked's `port/slippi_sys/GC/dsp_coef.bin`, embedded at build time for the options module's DSP low-pass setting; not committed | GPL-2.0-or-later | melee-unlocked's `docs/licenses/dsp-coefficients.txt`. |
| [Dolphin](https://github.com/dolphin-emu/dolphin) | Ports: GX register layouts and shader generation (`gx_regs.h`, `gx_shader.cpp`) | GPL-2.0-or-later | Ported files carry `SPDX-License-Identifier: GPL-2.0-or-later` (for example `VideoCommon/PixelShaderGen.cpp`, `VideoCommon/BPMemory.h`). Its COPYING says the repository as a whole "is compatible with the GPLv3 license". |
| [Slippi Ishiiruka](https://github.com/project-slippi/Ishiiruka) | Ports: netplay, matchmaking and the user record in the engine's netplay object (`mu-engine/net/hle`, through Melee-NX-NetPlay); the GameFiles in `port/slippi_sys` (from `Data/Sys`) | GPL-2.0-or-later | Its Readme: "licensed under the terms of the GNU General Public License, version 2 or later (GPLv2+)". Source headers such as `EXI_DeviceSlippi.cpp` and `SlippiNetplay.cpp`: "Licensed under GPLv2+". |
| [slippi-rust-extensions](https://github.com/project-slippi/slippi-rust-extensions) | The Discord presence (`port/runtime/hle/discord_rpc.cpp`) follows [pull request 36](https://github.com/project-slippi/slippi-rust-extensions/pull/36), which is not merged | GPL-2.0, no version stated | The repository ships the GPLv2 text as `LICENSE`; neither its README nor any crate's `Cargo.toml` states a version. Section 9 of GPLv2: "If the Program does not specify a version number of this License, you may choose any version ever published by the Free Software Foundation." |

## Components under other licences

| Component | Where | Licence | In the app? |
|---|---|---|---|
| [SDL 3](https://github.com/libsdl-org/SDL) 3.4.10 | Fetched and built from source by `port/CMakeLists.txt` (pinned by SHA-256) | zlib | Yes |
| [ENet](https://github.com/lsalzman/enet) | `port/third_party/enet`; Melee-NX-NetPlay's changed `enet.h`, `peer.c` and `protocol.c` in `mu-engine/net/enet` | MIT, see its `LICENSE` | Yes |
| [nlohmann/json](https://github.com/nlohmann/json) 3.4.0 | `port/third_party/nlohmann/json.hpp` | MIT, notice in the header | Yes |
| [VirtualFriend](https://github.com/agg23/virtualfriend) | The on-screen controller layout and touch model in `window_sdl.cpp` are adapted from it | MIT | Yes (as an adaptation) |

## Not in this repository

- **doldecomp/melee.** melee-unlocked's game is the [doldecomp/melee](https://github.com/doldecomp/melee) decompilation's C
  (melee-unlocked's pinned submodule), compiled at build time in your own build folder; that repository publishes no licence,
  and none of it is committed here.
- **Super Smash Bros. Melee.** Nintendo and HAL Laboratory own the game. This repository contains none of its code or data;
  the game runs from a disc image the player supplies. The files in `port/slippi_sys` are Slippi's own menu files and
  patches, as distributed in Slippi Ishiiruka.
- **Trademarks.** Slippi is a trademark of its authors. melee-unlocked's name, icon and launcher artwork are melee-unlocked's,
  converted from the fetched tree when the app is packaged and never committed.

## AI-assisted development

Most of this project was written with AI coding assistants, and most commits carry an AI co-author trailer. On
copyright, the U.S. Copyright Office's
[Part 2 report on copyrightability](https://www.copyright.gov/ai/Copyright-and-Artificial-Intelligence-Part-2-Copyrightability-Report.pdf)
(January 2025) concludes that "Copyright does not extend to purely AI-generated material, or material where there is
insufficient human control over the expressive elements", and that "Copyright protects the original expression in a work
created by a human author, even if the work also includes AI-generated material." Its
[registration guidance](https://www.copyright.gov/ai/ai_policy_guidance.pdf) (88 Fed. Reg. 16190, March 16, 2023) adds
that a human may select, arrange or modify AI-generated material in a way that is protected.

What that means here:

- The licence covers every part of this project that is protected by copyright. That includes the upstream code above, which
  people wrote, and the human selection, arrangement and modification in this project.
- Parts that are purely AI-generated may not be protected at all. Nobody, this project included, can restrict their use
  with a licence.
- That does not make the project public domain. Code derived from the GPL sources above stays under the GPL, and anyone
  who distributes it or a modified version must follow the GPL for it.
- Anthropic's [commercial terms](https://www.anthropic.com/legal/commercial-terms) assign to the customer whatever rights
  Anthropic has in outputs; they do not create copyright where the law recognises none.

## Notice texts

The MIT and zlib licences require their notices to be kept. ENet and nlohmann/json keep theirs in `port/third_party`; the
app bundle carries the licences of what the engine compiles in from the fetched melee-unlocked tree (Source Sans 3, Dear
ImGui, zstd). The others, which are not otherwise present in this repository:

### VirtualFriend

```
The MIT License (MIT)

Copyright (c) 2024 Adam Gastineau

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
```

### SDL

```
Copyright (C) 1997-2026 Sam Lantinga <slouken@libsdl.org>
  
This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:
  
1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required. 
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
```
