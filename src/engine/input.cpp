module;
#include <SDL.h>
#include "imgui.h"
#include "imgui_impl_sdl2.h"

module engine.input;

InputState poll_input() {
    InputState state{};
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL2_ProcessEvent(&event);

        if (event.type == SDL_QUIT)
            state.quit = true;
    }

    const uint8_t* keys = SDL_GetKeyboardState(nullptr);
    state.up = keys[SDL_SCANCODE_UP];
    state.down = keys[SDL_SCANCODE_DOWN];
    state.left = keys[SDL_SCANCODE_LEFT];
    state.right = keys[SDL_SCANCODE_RIGHT];
    state.debug_toggle = keys[SDL_SCANCODE_D];

    return state;
}