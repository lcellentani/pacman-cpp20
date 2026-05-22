module;
#include <nlohmann/json.hpp>
#include <fstream>

module game.config;

GameConfig load_config(const char* path) {
    std::ifstream f(path);
    if (!f) return {};

    auto j = nlohmann::json::parse(f, nullptr, /*allow_exceptions=*/false);
    if (j.is_discarded()) return {};

    GameConfig cfg;
    cfg.pacman_speed = j.value("pacman_speed", cfg.pacman_speed);
    return cfg;
}

void save_config(const GameConfig& cfg, const char* path) {
    nlohmann::json j;
    j["pacman_speed"] = cfg.pacman_speed;
    std::ofstream f(path);
    f << j.dump(4);
}
