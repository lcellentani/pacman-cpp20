export module game.config;

export struct GameConfig {
    float pacman_speed = 150.0f;
};

export GameConfig load_config(const char* path);
export void save_config(const GameConfig& cfg, const char* path);
