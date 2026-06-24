module;
#include <nlohmann/json.hpp>
#include <fstream>
#include <string>

module game.config;

import engine.log;

GameConfig load_config(const char* path) {
    std::ifstream f(path);
    if (!f) {
        log_warn(std::string{"config: could not open "} + path + ", using defaults");
        return {};
    }

    auto j = nlohmann::json::parse(f, nullptr, /*allow_exceptions=*/false);
    if (j.is_discarded()) {
        log_warn(std::string{"config: parse error in "} + path + ", using defaults");
        return {};
    }

    GameConfig cfg;
    cfg.pacman_speed = j.value("pacman_speed", cfg.pacman_speed);
    cfg.ghost_house_dots_pinky = j.value("ghost_house_dots_pinky", cfg.ghost_house_dots_pinky);
    cfg.ghost_house_dots_inky = j.value("ghost_house_dots_inky", cfg.ghost_house_dots_inky);
    cfg.ghost_house_dots_clyde = j.value("ghost_house_dots_clyde", cfg.ghost_house_dots_clyde);
    cfg.ghost_house_force_release_seconds = j.value("ghost_house_force_release_seconds", cfg.ghost_house_force_release_seconds);
    log_info(std::string{"config loaded from "} + path);
    return cfg;
}

void save_config(const GameConfig& cfg, const char* path) {
    nlohmann::json j;
    j["pacman_speed"] = cfg.pacman_speed;
    j["ghost_house_dots_pinky"] = cfg.ghost_house_dots_pinky;
    j["ghost_house_dots_inky"] = cfg.ghost_house_dots_inky;
    j["ghost_house_dots_clyde"] = cfg.ghost_house_dots_clyde;
    j["ghost_house_force_release_seconds"] = cfg.ghost_house_force_release_seconds;
    std::ofstream f(path);
    if (!f) {
        log_error(std::string{"config: could not open "} + path + " for write");
        return;
    }
    f << j.dump(4);
    log_info(std::string{"config saved to "} + path);
}
