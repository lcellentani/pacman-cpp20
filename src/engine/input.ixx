export module engine.input;

// Minimal input snapshot — polled once per frame
export struct InputState {
    bool quit = false;
    bool up = false;
    bool down = false;
    bool left = false;
    bool right = false;
    bool debug_toggle = false;   // D key
    bool console_toggle = false; // grave/backtick key
};

export InputState poll_input();