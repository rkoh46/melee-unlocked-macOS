# melee-unlocked engine player for macOS (the app's engine executable). Added by the top-level CMakeLists.txt; needs
# mu-engine/fetch.sh and mu-engine/build.sh game first (setup.sh and tools/mac/rebuild.sh run them).
#
# One process: melee-unlocked's host services drive the game image (mu-engine/runner: loader and macOS back ends);
# the app's own GX decoder (port/runtime/gx/gx_core.cpp, a build-time copy with the changes below), Metal renderer,
# SDL3 window, controllers (keyboard, SDL gamepads, the GameCube adapter over IOKit), in-game menu and sound output
# take the picture, the pads and the audio through the host's frontend hooks (patches/0011). Nothing in port/ changes.
# SPDX-License-Identifier: GPL-3.0-or-later
set(MU_REPO "${PROJECT_SOURCE_DIR}")
set(MU_TREE "${MU_REPO}/build/mu-engine/mu")
set(MU_OUT "${MU_REPO}/build/mu-engine/out")
set(MU_RUN "${MU_REPO}/mu-engine/runner")
set(MU_APPSRC "${MU_REPO}/mu-engine/app")
set(DD "${MU_REPO}/port/runtime")
set(MU_HIGH_BASE 0x100000000000ull)
foreach(_needed "${MU_TREE}/port/linux/host_services.cpp" "${MU_OUT}/melee_game.so")
  if(NOT EXISTS "${_needed}")
    message(FATAL_ERROR "missing ${_needed}: run mu-engine/fetch.sh and mu-engine/build.sh game first")
  endif()
endforeach()

# The decoder, from port/: console addresses reach 64 MiB (main memory is 40 MiB here and the game image follows it
# at 0x82800000), the RAM size check covers both, and the scene comes from the host's state snapshot (the scene
# controller lives in the game image, not at its console address).
file(READ "${DD}/gx/gx_core.cpp" _gx)
string(REPLACE "& 0x01FFFFFFu" "& 0x03FFFFFFu" _gx "${_gx}")
string(REPLACE "(g_bp.reg[BP_LOADTLUT0] << 5) & 0x01FFFFFF" "(g_bp.reg[BP_LOADTLUT0] & 0x1FFFFFu) << 5" _gx "${_gx}")
string(REPLACE "+ size > 0x01800000u" "+ size > host::mu_gx_ram_size" _gx "${_gx}")
string(REPLACE "ppc::RAM_SIZE" "host::mu_gx_ram_size" _gx "${_gx}")
string(REPLACE "host::rd8(0x80479D30)" "host::mu_scene_major()" _gx "${_gx}")
string(REPLACE "host::rd8(0x80479D33)" "host::mu_scene_minor()" _gx "${_gx}")
string(REPLACE "#include \"host.h\"" "#include \"host.h\"\nnamespace host { extern const uint32_t mu_gx_ram_size; uint8_t mu_scene_major(); uint8_t mu_scene_minor(); }\nnamespace gx { namespace { uint8_t g_native_owner = 0xFF, g_native_stage_map = 0xFF; bool g_native_skinned = false; } }" _gx "${_gx}")
# melee-unlocked's draw owner tokens (sent only while the host's native_owner_tracking answers 1, which the looks module
# does when a look needs them; Melee-NX-NetPlay frontend/patches/0116): 0xF1 the fighter slot of the draws that follow
# (0xFE particles, 0xFD items and projectiles, 0xFC an effect object's model, 0xFF none), 0xF2 whether the PObj is
# skinned, 0xF7 the stage map tag (0x80 scenery, 0x40 props close behind the floor; kept in DrawCall::viewport_x, unused
# otherwise). 0xF0 is its draw-scope audit (21 bytes), 0xF8 the online character select's hands (1 byte). Display only.
string(REPLACE "dc.owner_player = observed_owner();" "dc.owner_player = g_native_owner; (void)observed_owner;" _gx "${_gx}")
string(REPLACE "dc.skinned = observed_skinned();" "dc.skinned = g_native_skinned; dc.viewport_x = g_native_stage_map;" _gx "${_gx}")
string(REPLACE "g_dl_calls.clear(); g_immediate_draws.clear(); finish_observed_frame();" "g_dl_calls.clear(); g_immediate_draws.clear(); finish_observed_frame(); g_native_owner = 0xFF; g_native_stage_map = 0xFF; g_native_skinned = false;" _gx "${_gx}")
string(REPLACE "  if (op == 0x61) { if (len < 5) return incomplete(5); bp_write(be32(d + 1)); return 5; }" "  if (op == 0xF0) { if (len < 21) return incomplete(21); return 21; }
  if (op == 0xF1) { if (len < 2) return incomplete(2); g_native_owner = d[1]; return 2; }
  if (op == 0xF2) { if (len < 2) return incomplete(2); g_native_skinned = d[1] != 0; return 2; }
  if (op == 0xF7) { if (len < 2) return incomplete(2); g_native_stage_map = d[1]; return 2; }
  if (op == 0xF8) return 1;
  if (op == 0x61) { if (len < 5) return incomplete(5); bp_write(be32(d + 1)); return 5; }" _gx "${_gx}")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/gen/gx_core.cpp.new" "${_gx}")
configure_file("${CMAKE_CURRENT_BINARY_DIR}/gen/gx_core.cpp.new" "${CMAKE_CURRENT_BINARY_DIR}/gen/gx_core.cpp" COPYONLY)

# The game image and its sound-driver snapshot list, embedded so the executable is one file.
set(_embed "${CMAKE_CURRENT_BINARY_DIR}/gen/embedded_game.S")
file(WRITE "${_embed}.new" ".section __TEXT,__const\n.p2align 4\n.globl _mu_embedded_game\n_mu_embedded_game: .incbin \"${MU_OUT}/melee_game.so\"\n.globl _mu_embedded_game_end\n_mu_embedded_game_end:\n.globl _mu_embedded_snapexcl\n_mu_embedded_snapexcl: .incbin \"${MU_OUT}/melee_game.snapexcl\"\n.globl _mu_embedded_snapexcl_end\n_mu_embedded_snapexcl_end:\n")
configure_file("${_embed}.new" "${_embed}" COPYONLY)
set_source_files_properties("${_embed}" PROPERTIES OBJECT_DEPENDS "${MU_OUT}/melee_game.so;${MU_OUT}/melee_game.snapexcl")

add_library(mu_host_mac STATIC
  "${MU_TREE}/port/linux/host_services.cpp" "${MU_TREE}/port/linux/input_script.cpp"
  "${MU_TREE}/port/runtime/hle/ax_ucode.cpp" "${MU_TREE}/port/runtime/hle/native_savestate.cpp"
  "${MU_RUN}/mu_elf.cpp" "${MU_RUN}/macos/fixedmap_macos.c" "${MU_RUN}/macos/mu_imports_macos.c"
  "${MU_RUN}/macos/mu_linux_varargs.S" "${MU_RUN}/macos/abi_macos.cpp"
  "${MU_RUN}/net_host.cpp" "${MU_RUN}/rollback.cpp" "${MU_RUN}/macos/dirtytrack_macos.c")
target_include_directories(mu_host_mac PRIVATE "${MU_RUN}" "${MU_RUN}/macos" "${MU_TREE}/port/linux" "${MU_TREE}/port/runtime/abi"
  "${MU_TREE}/port/runtime/hle" "${MU_TREE}/port/runtime/host" "${MU_TREE}/port/runtime/ppc" "${MU_TREE}/port/runtime/gx" "${MU_TREE}/native")
target_compile_definitions(mu_host_mac PRIVATE MU_FOREIGN_ELF MU_SQUAD=1 MELEE_NO_SLIPPI MELEE_PORT_VERSION="mu-native"
  MELEE_LINUX_CPU_BASELINE="AArch64" MU_CARD_SIMPLE_COMMIT MU_HIGH_BASE=${MU_HIGH_BASE})
set_target_properties(mu_host_mac PROPERTIES CXX_STANDARD 17 CXX_EXTENSIONS ON)
target_compile_options(mu_host_mac PRIVATE -O2 -fno-strict-aliasing)

set(_dd_objc "${DD}/gx/gx_metal.mm" "${DD}/host/controller_rate.mm")

# melee-unlocked's in-game settings panel (ui/mu_panel.cpp, from Melee-NX-NetPlay's frontend) in place of the app's
# game_menu.cpp: its drawing code taken from the fetched tree at build time (ui/mu_ui_extract.py -> gen/mu_ui), Dear
# ImGui from the fetched tree with imgui_impl_metal (fetch.sh), its fonts and launcher art embedded below.
set(MU_UI "${MU_APPSRC}/ui")
set(MU_UI_GEN "${CMAKE_CURRENT_BINARY_DIR}/gen/mu_ui")
set(IMGUI "${MU_TREE}/port/third_party/imgui")
set(IMGUI_METAL "${MU_REPO}/build/mu-engine/imgui-metal")
execute_process(COMMAND python3 -I "${MU_UI}/mu_ui_extract.py" "${MU_TREE}" "${MU_UI_GEN}" OUTPUT_QUIET RESULT_VARIABLE _mu_ui_rc)
if(NOT _mu_ui_rc EQUAL 0)
  message(FATAL_ERROR "melee-unlocked engine: ui/mu_ui_extract.py could not take the panel's code from ${MU_TREE}")
endif()
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${MU_UI}/mu_ui_extract.py")
set(_ui_embed "${CMAKE_CURRENT_BINARY_DIR}/gen/embedded_ui.S")
set(_ui_files "")
set(_ui_asm ".section __TEXT,__const\n")
foreach(_pair "font_black|port/runtime/gx/ui_sources/gd_melee/assets/kit/SourceSans3-Black.otf"
              "font_bold|port/runtime/gx/ui_sources/gd_melee/assets/kit/SourceSans3-Bold.otf"
              "bailey|port/app/bailey_cut.bmp" "mark|port/app/melee_mark.bmp")
  string(REPLACE "|" ";" _p "${_pair}")
  list(GET _p 0 _n)
  list(GET _p 1 _f)
  if(NOT EXISTS "${MU_TREE}/${_f}")
    message(FATAL_ERROR "melee-unlocked engine: ${_f} is missing from the fetched tree")
  endif()
  string(APPEND _ui_asm ".p2align 4\n.globl _mu_ui_${_n}\n_mu_ui_${_n}: .incbin \"${MU_TREE}/${_f}\"\n.globl _mu_ui_${_n}_end\n_mu_ui_${_n}_end:\n")
  list(APPEND _ui_files "${MU_TREE}/${_f}")
endforeach()
# The GD Melee menu style's kit pictures and font pages (ui_sources/gd_melee/assets/kit, GPL-2.0-or-later, its ORIGIN.md) and
# the style gallery's screenshots (ui_sources/menu_previews/assets), embedded as PNG and decoded on first use (ui/mu_panel.cpp);
# gd_assets.inc names them (MU_GD_ASSET(name); a menu preview as mp_<name>).
set(_gd_inc "")
file(GLOB _gd_pngs "${MU_TREE}/port/runtime/gx/ui_sources/gd_melee/assets/kit/*.png")
file(GLOB _mp_pngs "${MU_TREE}/port/runtime/gx/ui_sources/menu_previews/assets/*.png")
foreach(_png ${_gd_pngs} ${_mp_pngs})
  get_filename_component(_n "${_png}" NAME_WE)
  string(MAKE_C_IDENTIFIER "${_n}" _n)
  if(_png IN_LIST _mp_pngs)
    set(_n "mp_${_n}")
  endif()
  string(APPEND _ui_asm ".p2align 3\n.globl _mu_gd_${_n}\n_mu_gd_${_n}: .incbin \"${_png}\"\n.globl _mu_gd_${_n}_end\n_mu_gd_${_n}_end:\n")
  string(APPEND _gd_inc "MU_GD_ASSET(${_n})\n")
  list(APPEND _ui_files "${_png}")
endforeach()
file(WRITE "${MU_UI_GEN}/gd_assets.inc.new" "${_gd_inc}")
configure_file("${MU_UI_GEN}/gd_assets.inc.new" "${MU_UI_GEN}/gd_assets.inc" COPYONLY)
file(WRITE "${_ui_embed}.new" "${_ui_asm}")
configure_file("${_ui_embed}.new" "${_ui_embed}" COPYONLY)
set_source_files_properties("${_ui_embed}" PROPERTIES OBJECT_DEPENDS "${_ui_files}")
add_library(mu_imgui STATIC "${IMGUI}/imgui.cpp" "${IMGUI}/imgui_draw.cpp" "${IMGUI}/imgui_tables.cpp" "${IMGUI}/imgui_widgets.cpp"
  "${IMGUI_METAL}/imgui_impl_metal.mm")
target_include_directories(mu_imgui PUBLIC "${IMGUI}" "${IMGUI_METAL}")
set_source_files_properties("${IMGUI_METAL}/imgui_impl_metal.mm" PROPERTIES COMPILE_OPTIONS "-fobjc-arc")
target_compile_features(mu_imgui PRIVATE cxx_std_17)
# The frontend layer of Melee-NX-NetPlay (mu-engine/app/fe): settings.ini and the option words (settings.cpp), mod
# profiles on the disc (mods.cpp), the feature modules (fe/features: options, Training, replays and their library,
# overlays, search, spectator, network overlay, Fight a pro) and Fight a pro's own inference engine (fe/proai).
set(FE "${MU_APPSRC}/fe")
set(FE_GEN "${CMAKE_CURRENT_BINARY_DIR}/gen/fe")
execute_process(COMMAND bash "${FE}/gen_fe.sh" "${MU_TREE}" "${FE_GEN}" RESULT_VARIABLE _fe_rc)
if(NOT _fe_rc EQUAL 0)
  message(FATAL_ERROR "melee-unlocked engine: fe/gen_fe.sh could not take its files from ${MU_TREE}")
endif()
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${FE}/gen_fe.sh" "${FE}/patches/0118-jukebox-on-the-mac.patch" "${FE}/patches/0122-user-gecko-on-the-mac.patch" "${FE}/patches/0112-phillip-ai-own-engine.patch"
  "${FE}/patches/0113-practice-menu-download-question.patch" "${FE}/proai/catalog.json")
file(READ "${FE}/proai/catalog.json" _proai_catalog)
file(WRITE "${FE_GEN}/proai_catalog.cpp.new"
  "// Generated from fe/proai/catalog.json.\nextern const char fe_proai_catalog_json[];\nconst char fe_proai_catalog_json[] = R\"PROAICATALOG(${_proai_catalog})PROAICATALOG\";\n")
configure_file("${FE_GEN}/proai_catalog.cpp.new" "${FE_GEN}/proai_catalog.cpp" COPYONLY)
file(WRITE "${FE_GEN}/zstdshim/zstd/zstd.h" "#include \"${MU_TREE}/port/third_party/zstd/zstd.h\"\n")
file(WRITE "${FE_GEN}/slpshim/slippilib/SlippiGame.h" "#include \"${MU_TREE}/port/third_party/slippilib/SlippiGame.h\"\n")
file(GLOB _fe_features CONFIGURE_DEPENDS "${FE}/features/*.cpp")
set(_enet "${MU_REPO}/port/third_party/enet")
set(_fe_enet "${_enet}/callbacks.c" "${_enet}/compress.c" "${_enet}/host.c" "${_enet}/list.c" "${_enet}/packet.c" "${_enet}/peer.c"
  "${_enet}/protocol.c" "${_enet}/unix.c")
file(GLOB _zstd_sources "${MU_TREE}/port/third_party/zstd/common/*.c" "${MU_TREE}/port/third_party/zstd/compress/*.c" "${MU_TREE}/port/third_party/zstd/decompress/*.c")
add_library(mu_zstd STATIC ${_zstd_sources})   # melee-unlocked's vendored zstd 1.5.7 (BSD-3-Clause): .slpz replays
target_compile_definitions(mu_zstd PRIVATE ZSTD_DISABLE_ASM=1)
target_compile_options(mu_zstd PRIVATE -O2 -w)
add_library(mu_fe OBJECT "${FE}/settings.cpp" "${FE}/mods.cpp" ${_fe_features} "${FE}/phob_adapter.cpp" "${MU_TREE}/port/runtime/host/phob.cpp" "${MU_RUN}/features/snapshot_store.cpp" ${_fe_enet})
set_source_files_properties(${_fe_enet} PROPERTIES COMPILE_DEFINITIONS "HAS_FCNTL=1;HAS_POLL=1;HAS_INET_PTON=1;HAS_INET_NTOP=1;HAS_SOCKLEN_T=1;HAS_MSGHDR_FLAGS=1"
  COMPILE_OPTIONS "-w;-include;arpa/inet.h")
set_target_properties(mu_fe PROPERTIES CXX_STANDARD 17 CXX_EXTENSIONS ON)
target_include_directories(mu_fe BEFORE PRIVATE "${FE_GEN}/slpshim" "${FE_GEN}/zstdshim")
target_include_directories(mu_fe PRIVATE "${FE}" "${FE}/features" "${FE_GEN}" "${MU_RUN}" "${MU_RUN}/features" "${MU_TREE}/port/linux" "${MU_TREE}/port/runtime/abi"
  "${MU_TREE}/port/runtime/hle" "${MU_TREE}/port/runtime/ppc" "${MU_TREE}/port/runtime/gx" "${MU_TREE}/native" "${MU_REPO}/build/mu-engine/gen"
  "${_enet}/include" "${MU_REPO}/port/third_party" "${MU_TREE}/port/runtime/host" "${DD}/host")
target_compile_definitions(mu_fe PRIVATE MU_FOREIGN_ELF MU_SQUAD=1 MELEE_NO_SLIPPI MELEE_PORT_VERSION="mu-native" MU_HIGH_BASE=${MU_HIGH_BASE})
target_compile_options(mu_fe PRIVATE -O2 -fno-strict-aliasing -Wno-deprecated-declarations)
# The free DSP coefficient table (Dolphin's free DSP ROM table, GPL-2.0-or-later, melee-unlocked's Sys/GC/dsp_coef.bin) for
# features/options.cpp (settings.ini dsp_lowpass), embedded from the fetched tree.
set(_dsp_bytes "0")
if(EXISTS "${MU_TREE}/port/slippi_sys/GC/dsp_coef.bin")
  file(READ "${MU_TREE}/port/slippi_sys/GC/dsp_coef.bin" _dsp_hex HEX)
  string(REGEX REPLACE "(..)" "0x\\1," _dsp_bytes "${_dsp_hex}")
endif()
file(WRITE "${FE_GEN}/dsp_coef_free.cpp.new" "// Generated from the fetched tree's port/slippi_sys/GC/dsp_coef.bin.\nextern const unsigned char fe_dsp_coef_free[];\nextern const unsigned fe_dsp_coef_free_size;\nconst unsigned char fe_dsp_coef_free[] = {${_dsp_bytes}};\nconst unsigned fe_dsp_coef_free_size = sizeof fe_dsp_coef_free;\n")
configure_file("${FE_GEN}/dsp_coef_free.cpp.new" "${FE_GEN}/dsp_coef_free.cpp" COPYONLY)
add_library(mu_proai OBJECT "${FE_GEN}/dsp_coef_free.cpp" "${MU_TREE}/port/runtime/hle/native_practice_model.cpp" "${FE}/proai/onnx_lite.cpp" "${FE}/proai/ort_lite.cpp" "${FE_GEN}/proai/phillip_ai.cpp" "${FE_GEN}/proai_catalog.cpp"
  "${FE}/proai/proai_fetch_curl.cpp")
set_target_properties(mu_proai PROPERTIES CXX_STANDARD 17 CXX_EXTENSIONS ON)
target_include_directories(mu_proai PRIVATE "${MU_REPO}/port/third_party" "${FE}/proai/include" "${FE}/proai" "${FE_GEN}/proai" "${FE_GEN}/abi" "${FE}" "${MU_TREE}/port/runtime/hle" "${MU_TREE}/port/runtime/abi" "${DD}/host")
target_compile_definitions(mu_proai PRIVATE MU_PROAI_FRONTEND=1)
target_compile_options(mu_proai PRIVATE -O2 -fno-strict-aliasing)
# Use custom music (fe/features/audio.cpp): melee-unlocked's jukebox (gen/fe/jukebox/jukebox.cpp: fe/gen_fe.sh's copy with
# fe/patches/0118, on the few host services fe/jukebox_shim/ declares) and the player-file decoders (fe/media_decode.cpp over
# minimp3, stb_vorbis, dr_wav and dr_flac, fetched by mu-engine/fetch.sh into build/mu-engine/audio-decoders, and AudioToolbox).
set(MU_AUDIO_DECODERS "${MU_REPO}/build/mu-engine/audio-decoders")
if(NOT EXISTS "${MU_AUDIO_DECODERS}/minimp3_ex.h")
  message(FATAL_ERROR "melee-unlocked engine: the audio decoders are missing: run mu-engine/fetch.sh")
endif()
add_library(mu_jukebox OBJECT "${FE_GEN}/jukebox/jukebox.cpp" "${FE}/media_decode.cpp")
set_target_properties(mu_jukebox PROPERTIES CXX_STANDARD 17 CXX_EXTENSIONS ON)   # upstream builds it as C++17
target_include_directories(mu_jukebox BEFORE PRIVATE "${FE}/jukebox_shim")
target_include_directories(mu_jukebox PRIVATE "${MU_TREE}/port/runtime/hle" "${MU_TREE}/port/linux" "${MU_AUDIO_DECODERS}" "${MU_TREE}/port/runtime/host")
target_compile_options(mu_jukebox PRIVATE -O2 -g -fno-strict-aliasing)
set_source_files_properties("${FE}/media_decode.cpp" PROPERTIES COMPILE_OPTIONS "-w")   # the decoders' own warnings
# The player's Gecko codes (fe/features/gecko.cpp): melee-unlocked's user_gecko.cpp (gen/fe/gecko: fe/gen_fe.sh's copy with
# fe/patches/0122) on fe/gecko_shim/gecko_host_shim.h; its variable table (user_gecko_targets.h) and function tables from the fetched tree.
add_library(mu_gecko OBJECT "${FE_GEN}/gecko/user_gecko.cpp")
set_target_properties(mu_gecko PROPERTIES CXX_STANDARD 17 CXX_EXTENSIONS ON)
target_include_directories(mu_gecko BEFORE PRIVATE "${FE}/gecko_shim")
target_include_directories(mu_gecko PRIVATE "${MU_TREE}/port/runtime/host" "${MU_TREE}/sourceport/game/shim")
target_compile_definitions(mu_gecko PRIVATE MELEE_NO_SLIPPI MNX_NO_FUNCTION_PATCHES)
target_compile_options(mu_gecko PRIVATE -O2 -g -fno-strict-aliasing)
# RVZ and WIA discs (disc_convert.cpp): melee-unlocked's disc_rvz.cpp and the bzip2 it vendors, from the fetched tree; LZMA from
# the mod installer's LZMA SDK objects, Zstandard from mu_zstd.
set(_mu_tp "${MU_TREE}/port/third_party")
add_library(mu_rvz OBJECT "${MU_APPSRC}/disc_convert.cpp" "${MU_TREE}/port/app/disc_rvz.cpp" "${_mu_tp}/bzip2/blocksort.c" "${_mu_tp}/bzip2/huffman.c"
  "${_mu_tp}/bzip2/crctable.c" "${_mu_tp}/bzip2/randtable.c" "${_mu_tp}/bzip2/compress.c" "${_mu_tp}/bzip2/decompress.c" "${_mu_tp}/bzip2/bzlib.c")
set_target_properties(mu_rvz PROPERTIES CXX_STANDARD 17)
target_include_directories(mu_rvz PRIVATE "${_mu_tp}" "${MU_TREE}/port/app")
target_compile_definitions(mu_rvz PRIVATE BZ_EXPORT)
target_compile_options(mu_rvz PRIVATE -O2 $<$<COMPILE_LANGUAGE:C>:-w>)
# melee-unlocked's character select easter egg (fe/egg.cpp): walter_egg.cpp, its header and its picture from the fetched tree
# (GPL-2.0-or-later), their includes of upstream's test hooks, renderer options and texture helper pointed at fe/egg_shim.
set(_egg_gen "${MU_UI_GEN}/egg")
file(MAKE_DIRECTORY "${_egg_gen}")
foreach(_f walter_egg.cpp walter_egg.h walter_egg_picture.h)
  file(READ "${MU_TREE}/port/runtime/gx/${_f}" _egg_text)
  string(REPLACE "#include \"test_env.h\"" "#include \"egg_test_env.h\"" _egg_text "${_egg_text}")
  string(REPLACE "#include \"render_options.h\"" "#include \"egg_render_options.h\"" _egg_text "${_egg_text}")
  string(REPLACE "#include \"user_texture.h\"" "#include \"egg_user_texture.h\"" _egg_text "${_egg_text}")
  file(WRITE "${_egg_gen}/${_f}.new" "${_egg_text}")
  configure_file("${_egg_gen}/${_f}.new" "${_egg_gen}/${_f}" COPYONLY)
endforeach()
set_source_files_properties("${FE}/egg.cpp" PROPERTIES INCLUDE_DIRECTORIES "${_egg_gen};${FE}/egg_shim;${MU_UI}/mac_include;${DD}/host")
set(_mu_fe_ui_sources "${FE}/egg.cpp" "${FE}/app_menu.cpp" "${FE}/overlay_draw.cpp" "${FE_GEN}/proai/practice_menu.cpp" "${FE}/proai/practice_overlay.cpp")
set_source_files_properties("${FE}/overlay_draw.cpp" PROPERTIES INCLUDE_DIRECTORIES "${FE};${FE_GEN};${DD}/host")
set_source_files_properties("${FE}/controls.cpp" "${FE}/controls_files.cpp" PROPERTIES INCLUDE_DIRECTORIES "${MU_UI};${FE};${MU_TREE}/port/linux;${MU_TREE}/port/runtime/abi;${DD}/host;${MU_TREE}/port/runtime/host")
set_source_files_properties("${MU_UI}/chords_mac.cpp" PROPERTIES INCLUDE_DIRECTORIES "${MU_UI};${FE};${MU_TREE}/port/runtime/abi;${DD}/host;${MU_TREE}/port/runtime/host")
set_source_files_properties("${FE}/app_menu.cpp" PROPERTIES INCLUDE_DIRECTORIES "${MU_UI};${FE};${MU_RUN};${MU_TREE}/port/linux;${MU_TREE}/port/runtime/abi;${DD}/host")
set_source_files_properties("${FE_GEN}/proai/practice_menu.cpp" "${FE}/proai/practice_overlay.cpp" PROPERTIES INCLUDE_DIRECTORIES "${MU_UI}/mac_include;${FE};${FE}/proai/include;${FE}/proai;${FE_GEN}/proai;${FE_GEN};${MU_TREE}/port/runtime/hle;${MU_TREE}/port/runtime/abi"
  COMPILE_DEFINITIONS "MU_PROAI_FRONTEND=1")
set(_mu_ui_sources ${_mu_fe_ui_sources} "${FE}/controls.cpp" "${FE}/controls_files.cpp" "${MU_UI}/controls_mac.cpp" "${FE}/features/present/present.cpp" "${MU_UI}/chords_mac.cpp" "${MU_UI}/mu_panel.cpp" "${MU_UI}/mu_mods.cpp" "${MU_UI}/frontend_mac.cpp" "${MU_UI}/software_keyboard_mac.mm" "${MU_UI}/ui_metal.mm" "${_ui_embed}")
set_source_files_properties("${MU_UI}/ui_metal.mm" "${MU_UI}/software_keyboard_mac.mm" PROPERTIES COMPILE_OPTIONS "-fobjc-arc")
set_source_files_properties("${MU_UI}/mu_panel.cpp" PROPERTIES COMPILE_DEFINITIONS "app_menu_frame=menu_frame")
add_executable(melee_unlocked_mac
  "${MU_APPSRC}/mu_play_main.cpp" "${MU_APPSRC}/host_shim.cpp" "${MU_APPSRC}/gx_looks_mac.cpp" "${MU_APPSRC}/slippi_menus.cpp" "${DD}/host/vcdiff.cpp"
  "${MU_RUN}/macos/frontend_hooks_none.cpp" "${_embed}"
  "${CMAKE_CURRENT_BINARY_DIR}/gen/gx_core.cpp" "${DD}/gx/gx_texture.cpp" "${DD}/gx/gx_shader.cpp" "${DD}/gx/gx_msl.cpp" "${DD}/gx/render_observer.cpp" "${DD}/gx/authored_pose.cpp" "${DD}/gx/subframe.cpp"
  "${DD}/host/window_sdl.cpp" "${DD}/host/input_config.cpp" "${DD}/host/input_script.cpp" ${_mu_ui_sources}
  "${DD}/host/audio_sdl.cpp" "${DD}/host/gc_adapter_iokit.cpp" ${_dd_objc})
set_source_files_properties(${_dd_objc} PROPERTIES COMPILE_OPTIONS "-fobjc-arc")
target_include_directories(melee_unlocked_mac PRIVATE "${MU_RUN}" "${MU_RUN}/macos" "${MU_TREE}/port/linux" "${MU_TREE}/port/runtime/abi"
  "${DD}/host" "${DD}/gx" "${DD}/ppc" "${DD}/hle" "${MU_REPO}/native" "${MU_REPO}/port/third_party" "${MU_TREE}/port/runtime/hle" "${MU_TREE}/port/third_party/slippilib" "${MU_REPO}/build/mu-engine/gen")
target_compile_definitions(melee_unlocked_mac PRIVATE MU_HIGH_BASE=${MU_HIGH_BASE} MELEE_PORT_VERSION="${MELEE_PORT_VERSION}")
target_compile_features(melee_unlocked_mac PRIVATE cxx_std_17)
target_include_directories(melee_unlocked_mac BEFORE PRIVATE "${MU_UI}" "${FE}" "${MU_UI_GEN}" "${MU_REPO}/mu-engine/modinstall" "${CMAKE_CURRENT_BINARY_DIR}/gen")
file(GLOB _net_patches "${MU_REPO}/mu-engine/net/patches/*.patch" "${MU_REPO}/mu-engine/net/hle/*" "${MU_REPO}/mu-engine/net/enet/*")
# The netplay object (mu-engine/build.sh net: Melee-NX-NetPlay's Slippi 3.7.0 netplay, mu-engine/net, with only netplay_bridge::* global).
add_custom_command(OUTPUT "${MU_OUT}/netplay.o" COMMAND bash "${MU_REPO}/mu-engine/build.sh" net
  DEPENDS "${MU_RUN}/netplay_bridge_mac.cpp" "${MU_RUN}/netplay_bridge.h" "${MU_REPO}/mu-engine/net_sources.sh"
    "${DD}/hle/discord_rpc.cpp" "${DD}/hle/slippi_login.cpp" ${_net_patches}
  COMMENT "melee-unlocked engine: the netplay object" VERBATIM)
add_custom_target(mu_netplay_object DEPENDS "${MU_OUT}/netplay.o")
add_dependencies(melee_unlocked_mac mu_netplay_object)
target_link_libraries(melee_unlocked_mac PRIVATE mu_host_mac mu_imgui mu_modinstall_objs mu_fe mu_proai mu_jukebox mu_gecko mu_rvz mu_zstd curl "${MU_OUT}/netplay.o" iconv ${MELEE_SDL3_TARGET} z
  "-framework AppKit" "-framework Foundation" "-framework Metal" "-framework QuartzCore" "-framework CoreText"
  "-framework CoreGraphics" "-framework IOKit" "-framework GameController" "-framework MetalFX" "-framework AudioToolbox" "-framework CoreAudio")
set_target_properties(melee_unlocked_mac PROPERTIES OUTPUT_NAME "MeleeUnlockedEngine")
# mu_panel.cpp asks for the Practice menu weakly (the Switch's features/proai.cpp): absent, its row is left out.
target_link_options(melee_unlocked_mac PRIVATE "LINKER:-U,__ZN2fe8features19practice_menu_chordEi")

# The engine's picture effects (mu-engine/app/gfx, gx::PostHooks on the Metal renderer): melee-unlocked's texture packs,
# menu video backgrounds, one-frame glitch detector and Lab view compiled from the fetched tree (gfx/gen_gfx.sh, its patches
# 013x), the Mac's AVFoundation/ImageIO media decoding for them (gfx/media_apple.mm), shader presets through librashader's
# Metal runtime loaded at run time (its MPL-2.0 headers from the fetched tree), capture bursts. Settings:
# fe/features/graphics.cpp.
set(GFX "${MU_APPSRC}/gfx")
set(GFX_GEN "${CMAKE_CURRENT_BINARY_DIR}/gen/gfx")
execute_process(COMMAND bash "${GFX}/gen_gfx.sh" "${MU_TREE}" "${GFX_GEN}" RESULT_VARIABLE _gfx_rc)
if(NOT _gfx_rc EQUAL 0)
  message(FATAL_ERROR "melee-unlocked engine: gfx/gen_gfx.sh could not take its files from ${MU_TREE}")
endif()
file(GLOB _gfx_patches "${GFX}/patches/*.patch")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${GFX}/gen_gfx.sh" ${_gfx_patches})
set(_gfx_mu_sources "${GFX_GEN}/texture_pack.cpp" "${GFX_GEN}/video_background.cpp" "${GFX_GEN}/flicker_scan.cpp" "${GFX_GEN}/lab_view.cpp")
set(_gfx_includes "${GFX}/shim" "${GFX_GEN}" "${FE}" "${DD}/gx" "${DD}/host" "${DD}/ppc" "${MU_TREE}/port/third_party" "${MU_TREE}/port/linux"
  "${MU_TREE}/port/runtime/abi" "${IMGUI}" "${MU_TREE}/port/runtime/gx")
set_source_files_properties(${_gfx_mu_sources} PROPERTIES INCLUDE_DIRECTORIES "${_gfx_includes}"
  COMPILE_OPTIONS "-include;${GFX}/shim/gfx_host_shim.h;-Wno-deprecated-declarations;-Wno-unused-function;-Wno-unused-variable")
set_source_files_properties("${GFX}/lab_mac.cpp" PROPERTIES INCLUDE_DIRECTORIES "${_gfx_includes}")
set_source_files_properties("${GFX}/effects_mac.mm" "${GFX}/media_apple.mm" PROPERTIES INCLUDE_DIRECTORIES "${_gfx_includes};${MU_TREE}/port/third_party/librashader"
  COMPILE_OPTIONS "-fobjc-arc")
target_sources(melee_unlocked_mac PRIVATE ${_gfx_mu_sources} "${GFX}/effects_mac.mm" "${GFX}/media_apple.mm" "${GFX}/lab_mac.cpp")
target_link_libraries(melee_unlocked_mac PRIVATE "-framework AVFoundation" "-framework CoreMedia" "-framework CoreVideo" "-framework ImageIO")

# melee-unlocked's PhobGCC menu (PhobVision's screens, PhobScope, Phob Check, named setups, the remap editor: its phob_menu.cpp,
# phobscope.cpp and phob_store.cpp, copied from the fetched tree by phob/gen_phob.sh) on fe/phob_adapter.cpp, compiled against
# melee-unlocked's own host.h in a library of its own (phob/phob_host_mac.cpp answers the host calls); phob_feed_mac.cpp is the
# panel's side, against the app's host.h.
set(PHOB "${MU_APPSRC}/phob")
set(PHOB_GEN "${CMAKE_CURRENT_BINARY_DIR}/gen/phob")
execute_process(COMMAND bash "${PHOB}/gen_phob.sh" "${MU_TREE}" "${PHOB_GEN}" RESULT_VARIABLE _phob_rc)
if(NOT _phob_rc EQUAL 0)
  message(FATAL_ERROR "melee-unlocked engine: phob/gen_phob.sh could not take its files from ${MU_TREE}")
endif()
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${PHOB}/gen_phob.sh")
add_library(mu_phob OBJECT "${PHOB_GEN}/phob_menu.cpp" "${PHOB_GEN}/phobscope.cpp" "${PHOB_GEN}/phob_store.cpp" "${PHOB}/phob_host_mac.cpp")
set_target_properties(mu_phob PROPERTIES CXX_STANDARD 17 CXX_EXTENSIONS ON)
target_include_directories(mu_phob BEFORE PRIVATE "${PHOB}/shim" "${PHOB_GEN}" "${PHOB}")
target_include_directories(mu_phob PRIVATE "${FE}" "${MU_TREE}/port/runtime/host" "${MU_TREE}/port/runtime/gx" "${MU_TREE}/port/runtime/ppc"
  "${MU_TREE}/port/runtime/abi" "${MU_TREE}/port/runtime/hle" "${IMGUI}" "${FE_GEN}/proai" "${MU_TREE}/port/third_party")
target_compile_definitions(mu_phob PRIVATE MU_FOREIGN_ELF MELEE_NO_SLIPPI MU_HIGH_BASE=${MU_HIGH_BASE})
target_compile_options(mu_phob PRIVATE -O2 -fno-strict-aliasing -Wno-deprecated-declarations)
set_source_files_properties("${PHOB}/phob_feed_mac.cpp" PROPERTIES INCLUDE_DIRECTORIES "${PHOB};${DD}/host")
target_sources(melee_unlocked_mac PRIVATE $<TARGET_OBJECTS:mu_phob> "${PHOB}/phob_feed_mac.cpp")

# The mod installer (mu-engine/modinstall, from Melee-NX-NetPlay) for the launcher's Mods page, linked into the launcher
# (melee_launcher_mac) as objects. melee-unlocked's catalog and the LZMA SDK decoders come from the fetched tree at build time.
file(READ "${MU_TREE}/mods-catalog-2.json" _catalog)
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/gen/mods_catalog_json.h.new" "// generated from the fetched tree's mods-catalog-2.json\n#pragma once\nstatic const char kModsCatalogJson[] = R\"MUCATALOG(${_catalog})MUCATALOG\";\n")
configure_file("${CMAKE_CURRENT_BINARY_DIR}/gen/mods_catalog_json.h.new" "${CMAKE_CURRENT_BINARY_DIR}/gen/mods_catalog_json.h" COPYONLY)
set(MI "${MU_REPO}/mu-engine/modinstall")
add_library(mu_modinstall_objs OBJECT "${MU_APPSRC}/mods_facade.cpp"
  "${MI}/mi_hash.cpp" "${MI}/mi_io.cpp" "${MI}/mi_archive.cpp" "${MI}/mi_lzma.cpp" "${MI}/mi_vcdiff.cpp" "${MI}/mi_catalog.cpp"
  "${MI}/mod_install.cpp" "${MI}/mi_fetch_curl.cpp"
  "${MU_TREE}/port/third_party/lzma/LzmaDec.c" "${MU_TREE}/port/third_party/lzma/Lzma2Dec.c")
target_include_directories(mu_modinstall_objs PRIVATE "${MI}" "${MU_TREE}/port/third_party/lzma" "${MU_REPO}/port/third_party" "${CMAKE_CURRENT_BINARY_DIR}/gen")
target_compile_features(mu_modinstall_objs PRIVATE cxx_std_17)
# melee-unlocked's crash report (crash_report.cpp, its port/app/launcher_crash_*.h from the fetched tree) for the launcher's
# "Save crash report": linked into the launcher the same way.
file(STRINGS "${MU_REPO}/VERSION" _mu_app_version LIMIT_COUNT 1)
add_library(mu_crash_objs OBJECT "${MU_APPSRC}/crash_report.cpp")
target_include_directories(mu_crash_objs PRIVATE "${MU_TREE}/port/app")
target_compile_definitions(mu_crash_objs PRIVATE MELEE_PORT_VERSION="${_mu_app_version}")
target_compile_features(mu_crash_objs PRIVATE cxx_std_17)
target_sources(melee_launcher_mac PRIVATE $<TARGET_OBJECTS:mu_modinstall_objs> $<TARGET_OBJECTS:mu_crash_objs>)
target_link_libraries(melee_launcher_mac PRIVATE curl z)
