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

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.91.9b
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(imgui)

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer2.cpp
#    ${imgui_SOURCE_DIR}/imgui_demo.cpp
)

target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

target_link_libraries(imgui PUBLIC SDL2::SDL2)
set_target_properties(imgui PROPERTIES CXX_SCAN_FOR_MODULES OFF)

# Pacman executable
add_executable(pacman)

target_sources(pacman
    PRIVATE
        src/main.cpp
        src/engine/input.cpp
        src/engine/renderer.cpp
        src/game/debug.cpp
        src/game/map.cpp
        src/game/pacman.cpp
        src/game/stage.cpp
    PRIVATE FILE_SET CXX_MODULES FILES
        src/engine/renderer.ixx
        src/engine/input.ixx
        src/engine/types.ixx
        src/game/concepts.ixx
        src/game/debug.ixx
        src/game/map.ixx
        src/game/pacman.ixx
        src/game/stage.ixx
        src/game/types.ixx
 )

target_link_libraries(pacman PRIVATE SDL2::SDL2 SDL2::SDL2main imgui)

# MSVC: disable legacy header units, enable standard conformance
target_compile_options(pacman PRIVATE /W4 /permissive- /EHsc)

add_custom_command(TARGET pacman POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:SDL2::SDL2>
        $<TARGET_FILE_DIR:pacman>
    COMMENT "Copying SDL2 DLL to output directory"
)