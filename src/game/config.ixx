export module game.config;

export struct GameConfig {
    float pacman_speed = 150.0f;
    float ghost_speed = 150.0f;
    int ghost_house_dots_pinky = 0;
    int ghost_house_dots_inky = 30;
    int ghost_house_dots_clyde = 60;
    float ghost_house_force_release_seconds = 5.0f;
};

export GameConfig load_config(const char* path);
export void save_config(const GameConfig& cfg, const char* path);
