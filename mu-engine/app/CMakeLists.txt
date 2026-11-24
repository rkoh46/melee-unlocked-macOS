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
  "${MU_RUN}/macos/mu_linux_varargs.S" "${MU_RUN}/macos/abi_macos.cpp")
target_include_directories(mu_host_mac PRIVATE "${MU_RUN}" "${MU_RUN}/macos" "${MU_TREE}/port/linux" "${MU_TREE}/port/runtime/abi"
  "${MU_TREE}/port/runtime/hle" "${MU_TREE}/port/runtime/host" "${MU_TREE}/port/runtime/ppc" "${MU_TREE}/port/runtime/gx" "${MU_TREE}/native")
target_compile_definitions(mu_host_mac PRIVATE MU_FOREIGN_ELF MU_SQUAD=1 MELEE_NO_SLIPPI MELEE_PORT_VERSION="mu-native"
  MELEE_LINUX_CPU_BASELINE="AArch64" MU_CARD_SIMPLE_COMMIT MU_HIGH_BASE=${MU_HIGH_BASE})
set_target_properties(mu_host_mac PROPERTIES CXX_STANDARD 17 CXX_EXTENSIONS ON)
target_compile_options(mu_host_mac PRIVATE -O2 -fno-strict-aliasing)

set(_dd_objc "${DD}/gx/gx_metal.mm" "${DD}/host/controller_rate.mm")
add_executable(melee_unlocked_mac
  "${MU_APPSRC}/mu_play_main.cpp" "${MU_APPSRC}/host_shim.cpp" "${MU_RUN}/macos/frontend_hooks_none.cpp" "${_embed}"
  "${CMAKE_CURRENT_BINARY_DIR}/gen/gx_core.cpp" "${DD}/gx/gx_texture.cpp" "${DD}/gx/gx_shader.cpp" "${DD}/gx/gx_msl.cpp" "${DD}/gx/render_observer.cpp" "${DD}/gx/authored_pose.cpp" "${DD}/gx/subframe.cpp"
  "${DD}/host/window_sdl.cpp" "${DD}/host/input_config.cpp" "${DD}/host/input_script.cpp" "${DD}/host/game_menu.cpp"
  "${DD}/host/audio_sdl.cpp" "${DD}/host/gc_adapter_iokit.cpp" ${_dd_objc})
set_source_files_properties(${_dd_objc} PROPERTIES COMPILE_OPTIONS "-fobjc-arc")
target_include_directories(melee_unlocked_mac PRIVATE "${MU_RUN}" "${MU_RUN}/macos" "${MU_TREE}/port/linux" "${MU_TREE}/port/runtime/abi"
  "${DD}/host" "${DD}/gx" "${DD}/ppc" "${DD}/hle" "${MU_REPO}/native" "${MU_REPO}/port/third_party")
target_compile_definitions(melee_unlocked_mac PRIVATE MU_HIGH_BASE=${MU_HIGH_BASE} MELEE_PORT_VERSION="${MELEE_PORT_VERSION}"
  MELEE_SOURCE_ART_DIR="${MU_REPO}/port/app/art/controller")
target_compile_features(melee_unlocked_mac PRIVATE cxx_std_17)
target_link_libraries(melee_unlocked_mac PRIVATE mu_host_mac native_animation ${AURORA_SDL3_TARGET} z
  "-framework AppKit" "-framework Foundation" "-framework Metal" "-framework QuartzCore" "-framework CoreText"
  "-framework CoreGraphics" "-framework IOKit" "-framework GameController" "-framework MetalFX")
set_target_properties(melee_unlocked_mac PROPERTIES OUTPUT_NAME "MeleeUnlockedEngine")
