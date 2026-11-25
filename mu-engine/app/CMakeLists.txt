# melee-unlocked engine player for macOS (the app's MU engine executable). Added by the top-level CMakeLists.txt when
# MELEE_BUILD_MU_ENGINE is on (setup.sh and tools/mac/rebuild.sh turn it on after mu-engine/build.sh game).
#
# One process: melee-unlocked's host services drive the game image (mu-engine/runner: loader and macOS back ends);
# the app's own GX decoder (port/runtime/gx/gx_core.cpp, a build-time copy with three changes below), Metal renderer,
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
string(REPLACE "#include \"host.h\"" "#include \"host.h\"\nnamespace host { extern const uint32_t mu_gx_ram_size; uint8_t mu_scene_major(); uint8_t mu_scene_minor(); }" _gx "${_gx}")
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
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${FE}/gen_fe.sh" "${FE}/patches/0112-phillip-ai-own-engine.patch"
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
add_library(mu_fe OBJECT "${FE}/settings.cpp" "${FE}/mods.cpp" ${_fe_features} "${MU_RUN}/features/snapshot_store.cpp" ${_fe_enet})
set_source_files_properties(${_fe_enet} PROPERTIES COMPILE_DEFINITIONS "HAS_FCNTL=1;HAS_POLL=1;HAS_INET_PTON=1;HAS_INET_NTOP=1;HAS_SOCKLEN_T=1;HAS_MSGHDR_FLAGS=1"
  COMPILE_OPTIONS "-w;-include;arpa/inet.h")
set_target_properties(mu_fe PROPERTIES CXX_STANDARD 17 CXX_EXTENSIONS ON)
target_include_directories(mu_fe BEFORE PRIVATE "${FE_GEN}/slpshim" "${FE_GEN}/zstdshim")
target_include_directories(mu_fe PRIVATE "${FE}" "${FE}/features" "${FE_GEN}" "${MU_RUN}" "${MU_RUN}/features" "${MU_TREE}/port/linux" "${MU_TREE}/port/runtime/abi"
  "${MU_TREE}/port/runtime/hle" "${MU_TREE}/port/runtime/ppc" "${MU_TREE}/port/runtime/gx" "${MU_TREE}/native" "${MU_REPO}/build/mu-engine/gen"
  "${_enet}/include" "${MU_REPO}/port/third_party" "${MU_TREE}/port/runtime/host" "${DD}/host")
target_compile_definitions(mu_fe PRIVATE MU_FOREIGN_ELF MU_SQUAD=1 MELEE_NO_SLIPPI MELEE_PORT_VERSION="mu-native" MU_HIGH_BASE=${MU_HIGH_BASE})
target_compile_options(mu_fe PRIVATE -O2 -fno-strict-aliasing -Wno-deprecated-declarations)
add_library(mu_proai OBJECT "${FE}/proai/onnx_lite.cpp" "${FE}/proai/ort_lite.cpp" "${FE_GEN}/proai/phillip_ai.cpp" "${FE_GEN}/proai_catalog.cpp"
  "${FE}/proai/proai_fetch_curl.cpp")
set_target_properties(mu_proai PROPERTIES CXX_STANDARD 17 CXX_EXTENSIONS ON)
target_include_directories(mu_proai PRIVATE "${MU_REPO}/port/third_party" "${FE}/proai/include" "${FE}/proai" "${FE_GEN}/proai" "${FE_GEN}/abi" "${FE}" "${MU_TREE}/port/runtime/hle" "${MU_TREE}/port/runtime/abi" "${DD}/host")
target_compile_definitions(mu_proai PRIVATE MU_PROAI_FRONTEND=1)
target_compile_options(mu_proai PRIVATE -O2 -fno-strict-aliasing)
set(_mu_fe_ui_sources "${FE}/app_menu.cpp" "${FE}/overlay_draw.cpp" "${FE_GEN}/proai/practice_menu.cpp" "${FE}/proai/practice_overlay.cpp")
set_source_files_properties("${FE}/overlay_draw.cpp" PROPERTIES INCLUDE_DIRECTORIES "${FE};${FE_GEN};${DD}/host")
set_source_files_properties("${FE}/controls.cpp" "${FE}/controls_files.cpp" PROPERTIES INCLUDE_DIRECTORIES "${MU_UI};${FE};${MU_TREE}/port/linux;${MU_TREE}/port/runtime/abi;${DD}/host;${MU_TREE}/port/runtime/host")
set_source_files_properties("${MU_UI}/chords_mac.cpp" PROPERTIES INCLUDE_DIRECTORIES "${MU_UI};${FE};${MU_TREE}/port/runtime/abi;${DD}/host;${MU_TREE}/port/runtime/host")
set_source_files_properties("${FE}/app_menu.cpp" PROPERTIES INCLUDE_DIRECTORIES "${MU_UI};${FE};${MU_RUN};${MU_TREE}/port/linux;${MU_TREE}/port/runtime/abi;${DD}/host")
set_source_files_properties("${FE_GEN}/proai/practice_menu.cpp" "${FE}/proai/practice_overlay.cpp" PROPERTIES INCLUDE_DIRECTORIES "${MU_UI}/mac_include;${FE};${FE}/proai/include;${FE}/proai;${FE_GEN}/proai;${FE_GEN};${MU_TREE}/port/runtime/hle;${MU_TREE}/port/runtime/abi"
  COMPILE_DEFINITIONS "MU_PROAI_FRONTEND=1")
set(_mu_ui_sources ${_mu_fe_ui_sources} "${FE}/controls.cpp" "${FE}/controls_files.cpp" "${MU_UI}/controls_mac.cpp" "${FE}/features/present/present.cpp" "${MU_UI}/chords_mac.cpp" "${MU_UI}/mu_panel.cpp" "${MU_UI}/mu_mods.cpp" "${MU_UI}/frontend_mac.cpp" "${MU_UI}/ui_metal.mm" "${_ui_embed}")
set_source_files_properties("${MU_UI}/ui_metal.mm" PROPERTIES COMPILE_OPTIONS "-fobjc-arc")
set_source_files_properties("${MU_UI}/mu_panel.cpp" PROPERTIES COMPILE_DEFINITIONS "app_menu_frame=menu_frame")
add_executable(melee_unlocked_mac
  "${MU_APPSRC}/mu_play_main.cpp" "${MU_APPSRC}/host_shim.cpp" "${MU_APPSRC}/slippi_menus.cpp" "${DD}/host/vcdiff.cpp"
  "${MU_RUN}/macos/frontend_hooks_none.cpp" "${_embed}"
  "${CMAKE_CURRENT_BINARY_DIR}/gen/gx_core.cpp" "${DD}/gx/gx_texture.cpp" "${DD}/gx/gx_shader.cpp" "${DD}/gx/gx_msl.cpp" "${DD}/gx/render_observer.cpp" "${DD}/gx/authored_pose.cpp" "${DD}/gx/subframe.cpp"
  "${DD}/host/window_sdl.cpp" "${DD}/host/input_config.cpp" "${DD}/host/input_script.cpp" ${_mu_ui_sources}
  "${DD}/host/audio_sdl.cpp" "${DD}/host/gc_adapter_iokit.cpp" ${_dd_objc})
set_source_files_properties(${_dd_objc} PROPERTIES COMPILE_OPTIONS "-fobjc-arc")
target_include_directories(melee_unlocked_mac PRIVATE "${MU_RUN}" "${MU_RUN}/macos" "${MU_TREE}/port/linux" "${MU_TREE}/port/runtime/abi"
  "${DD}/host" "${DD}/gx" "${DD}/ppc" "${DD}/hle" "${MU_REPO}/native" "${MU_REPO}/port/third_party" "${MU_TREE}/port/runtime/hle" "${MU_TREE}/port/third_party/slippilib" "${MU_REPO}/build/mu-engine/gen")
target_compile_definitions(melee_unlocked_mac PRIVATE MU_HIGH_BASE=${MU_HIGH_BASE} MELEE_PORT_VERSION="${MELEE_PORT_VERSION}"
  MELEE_SOURCE_ART_DIR="${MU_REPO}/port/app/art/controller")
target_compile_features(melee_unlocked_mac PRIVATE cxx_std_17)
target_include_directories(melee_unlocked_mac BEFORE PRIVATE "${MU_UI}" "${FE}" "${MU_UI_GEN}" "${MU_REPO}/mu-engine/modinstall" "${CMAKE_CURRENT_BINARY_DIR}/gen")
# The netplay object (mu-engine/build.sh net: this repository's Slippi netplay with only netplay_bridge::* global).
add_custom_command(OUTPUT "${MU_OUT}/netplay.o" COMMAND bash "${MU_REPO}/mu-engine/build.sh" net
  DEPENDS "${MU_RUN}/netplay_bridge_mac.cpp" "${DD}/hle/slippi_online.cpp" "${DD}/hle/slippi_net.cpp" "${DD}/hle/slippi_net.h" "${DD}/hle/slippi_online.h"
  COMMENT "melee-unlocked engine: the netplay object" VERBATIM)
add_custom_target(mu_netplay_object DEPENDS "${MU_OUT}/netplay.o")
add_dependencies(melee_unlocked_mac mu_netplay_object)
target_link_libraries(melee_unlocked_mac PRIVATE mu_host_mac mu_imgui mu_modinstall_objs mu_fe mu_proai mu_zstd curl native_animation "${MU_OUT}/netplay.o" iconv ${AURORA_SDL3_TARGET} z
  "-framework AppKit" "-framework Foundation" "-framework Metal" "-framework QuartzCore" "-framework CoreText"
  "-framework CoreGraphics" "-framework IOKit" "-framework GameController" "-framework MetalFX")
set_target_properties(melee_unlocked_mac PROPERTIES OUTPUT_NAME "MeleeUnlockedEngine")
# mu_panel.cpp asks for the Practice menu weakly (the Switch's features/proai.cpp): absent, its row is left out.
target_link_options(melee_unlocked_mac PRIVATE "LINKER:-U,__ZN2fe8features19practice_menu_chordEi")

# The mod installer (mu-engine/modinstall, from Melee-NX-NetPlay) for the launcher's Mods page: linked into the app
# (melee_port_mac) as objects so its definitions replace the launcher's weak "no installer" ones. melee-unlocked's
# catalog and the LZMA SDK decoders come from the fetched tree at build time.
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
if(TARGET melee_port_mac)
  target_sources(melee_port_mac PRIVATE $<TARGET_OBJECTS:mu_modinstall_objs>)
  target_link_libraries(melee_port_mac PRIVATE curl z)
endif()
