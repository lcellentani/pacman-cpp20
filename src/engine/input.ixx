export module engine.input;

// Minimal input snapshot — polled once per frame
export struct InputState {
    bool quit = false;
    bool up = false;
    bool down = false;
    bool left = false;
    bool right = false;
};

export InputState poll_input();