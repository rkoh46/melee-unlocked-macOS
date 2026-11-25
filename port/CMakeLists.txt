# The app's own code, next to melee-unlocked's engine (mu-engine/app):
#   - the launcher executable (app/main_mac.cpp and runtime/host/mu_launcher.mm, melee-unlocked's launcher rebuilt for
#     the Mac), the bundle's main program: it shows the launcher and starts MeleeUnlockedEngine;
#   - SDL3 for the engine's window, controllers and sound (SDL 3.4.10, built from source as a static library);
#   - runtime/: the GX decoder, Metal renderer and host services the engine compiles in (mu-engine/app/CMakeLists.txt
#     lists them), Discord presence and the Slippi sign-in (mu-engine/build.sh net), enet and nlohmann/json.
# SPDX-License-Identifier: GPL-2.0-or-later
include(FetchContent)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
FetchContent_Declare(SDL
  URL "https://github.com/libsdl-org/SDL/archive/refs/tags/release-3.4.10.tar.gz"
  URL_HASH SHA256=0dc11d980ba17250200718fa4e28011da293f27ed92f92203afffe396811f307
  DOWNLOAD_EXTRACT_TIMESTAMP FALSE
  EXCLUDE_FROM_ALL)
FetchContent_MakeAvailable(SDL)
set(MELEE_SDL3_TARGET SDL3::SDL3-static CACHE INTERNAL "SDL3 for the engine")

add_executable(melee_launcher_mac app/main_mac.cpp runtime/host/mu_launcher.mm runtime/host/dashboard.cpp
  runtime/host/input_config.cpp runtime/hle/slippi_history.cpp runtime/hle/slippi_login.cpp runtime/hle/slippi_http_apple.mm)
set_source_files_properties(runtime/host/mu_launcher.mm runtime/hle/slippi_http_apple.mm PROPERTIES COMPILE_OPTIONS "-fobjc-arc")
target_include_directories(melee_launcher_mac PRIVATE runtime/host runtime/hle runtime/ppc runtime/gx third_party "${PROJECT_SOURCE_DIR}/mu-engine/app")
target_compile_features(melee_launcher_mac PRIVATE cxx_std_17)
target_compile_definitions(melee_launcher_mac PRIVATE MELEE_PORT_VERSION="${MELEE_PORT_VERSION}")
target_link_libraries(melee_launcher_mac PRIVATE ${MELEE_SDL3_TARGET} "-framework AppKit" "-framework Foundation")
