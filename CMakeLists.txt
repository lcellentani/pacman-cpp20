cmake_minimum_required(VERSION 3.28)
project(pacman LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Required for MSVC module scanning
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

# Fetch SDL2
include(FetchContent)
FetchContent_Declare(
    SDL2
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG        release-2.30.2
)
FetchContent_MakeAvailable(SDL2)

add_executable(pacman)

target_sources(pacman
    PRIVATE
        src/main.cpp
        src/engine/renderer.cpp
        src/engine/game.cpp
        src/game/pacman.cpp
    PRIVATE FILE_SET CXX_MODULES FILES
        src/engine/renderer.ixx
        src/engine/input.ixx
        src/engine/game.ixx
        src/engine/types.ixx
        src/game/concepts.ixx
        src/game/types.ixx
        src/game/pacman.ixx
   "src/engine/types.ixx")

target_link_libraries(pacman PRIVATE SDL2::SDL2 SDL2::SDL2main)

# MSVC: disable legacy header units, enable standard conformance
target_compile_options(pacman PRIVATE /W4 /permissive- /EHsc)

add_custom_command(TARGET pacman POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:SDL2::SDL2>
        $<TARGET_FILE_DIR:pacman>
    COMMENT "Copying SDL2 DLL to output directory"
)