cmake_minimum_required(VERSION 3.28)
project(pacman LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Required for MSVC module scanning
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

# SDL2 2.30.x defines _m_prefetch inside #ifndef __PRFCHWINTRIN_H as a Clang
# workaround. Clang 22 added _m_prefetch as a builtin, so SDL2's definition now
# collides (hard error). Predefining the guard lets Clang's builtin win and
# skips SDL2's conflicting definition. C-mode SDL2 sources need it too.
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND NOT MSVC)
    add_compile_options(-D__PRFCHWINTRIN_H)
endif()

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

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(nlohmann_json)

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
add_executable(pacman WIN32)

target_sources(pacman
    PRIVATE
        src/main.cpp
        src/engine/input.cpp
        src/engine/log.cpp
        src/engine/random.cpp
        src/engine/renderer.cpp
        src/game/config.cpp
        src/game/console.cpp
        src/game/debug.cpp
        src/game/ghost.cpp
        src/game/map.cpp
        src/game/pacman.cpp
        src/game/scheduler.cpp
        src/game/stage.cpp
    PRIVATE FILE_SET CXX_MODULES FILES
        src/engine/log.ixx
        src/engine/random.ixx
        src/engine/renderer.ixx
        src/engine/input.ixx
        src/engine/types.ixx
        src/game/concepts.ixx
        src/game/config.ixx
        src/game/console.ixx
        src/game/debug.ixx
        src/game/ghost.ixx
        src/game/map.ixx
        src/game/pacman.ixx
        src/game/scheduler.ixx
        src/game/stage.ixx
        src/game/types.ixx
 )

target_link_libraries(pacman PRIVATE SDL2::SDL2 imgui nlohmann_json::nlohmann_json)

# WIN32 subsystem requires WinMain, but we use int main(). Explicitly set the
# CRT entry point so lld-link/MSVC calls main() directly instead of WinMain.
target_link_options(pacman PRIVATE -Xlinker /ENTRY:mainCRTStartup)

if(MSVC)
    # cl.exe and clang-cl: strict conformance, high warnings
    target_compile_options(pacman PRIVATE /W4 /permissive- /EHsc)
else()
    # clang++/g++ GNU frontend
    target_compile_options(pacman PRIVATE -Wall -Wextra -Wpedantic -fexceptions)
endif()

add_custom_command(TARGET pacman POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:SDL2::SDL2>
        $<TARGET_FILE_DIR:pacman>
    COMMENT "Copying SDL2 DLL to output directory"
)