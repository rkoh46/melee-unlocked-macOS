# melee-unlocked-macOS: the launcher and the app's renderer and host code (port/) and melee-unlocked's engine
# (mu-engine/app). mu-engine/fetch.sh and mu-engine/build.sh game come first; setup.sh and tools/mac/rebuild.sh run them.
# SPDX-License-Identifier: GPL-3.0-or-later
cmake_minimum_required(VERSION 3.25)
project(MeleeUnlocked LANGUAGES C CXX OBJC OBJCXX ASM)
if(NOT APPLE OR NOT CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  message(FATAL_ERROR "melee-unlocked-macOS builds on macOS (Apple silicon)")
endif()
file(STRINGS "${PROJECT_SOURCE_DIR}/VERSION" MELEE_PORT_VERSION LIMIT_COUNT 1)
add_subdirectory(port)
add_subdirectory(mu-engine/app)
