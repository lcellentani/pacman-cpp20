module;
#include <SDL.h>

export module engine.input;

// Minimal input snapshot — polled once per frame
export struct InputState {
    bool quit = false;
    bool up = false;
    bool down = false;
    bool left = false;
    bool right = false;
};

export InputState poll_input() {
    InputState state{};
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT)
            state.quit = true;
    }

    const uint8_t* keys = SDL_GetKeyboardState(nullptr);
    state.up = keys[SDL_SCANCODE_UP];
    state.down = keys[SDL_SCANCODE_DOWN];
    state.left = keys[SDL_SCANCODE_LEFT];
    state.right = keys[SDL_SCANCODE_RIGHT];

    return state;
}