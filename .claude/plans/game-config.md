# Plan: `GameConfig` — Live-Tweakable, Serializable Gameplay Parameters

## Context

Gameplay values like `speed_` are hardcoded inside `Pacman::reset()`. There is no central place to discover, reason about, modify at runtime, or persist them across sessions. As Phase 3 (ghost AI) approaches, the number of tunable values will grow significantly (ghost speeds, scatter/chase durations, frightened timer, etc.). This plan introduces a `GameConfig` struct, wires it into the ImGui debug panel for live editing, and supports JSON serialization so tweaks survive restarts.

---

## Design Rationale

### Serialization (nlohmann/json, exception-free)

`GameConfig` is a flat struct of named fields — maps naturally to a JSON object. We use **nlohmann/json** (header-only, FetchContent-friendly). Critically, we use the exception-free API throughout:

- `nlohmann::json::parse(f, nullptr, false)` — returns a "discarded" sentinel on malformed JSON instead of throwing `parse_error`.
- `j.value("key", default)` per field — returns the field's own default if the key is absent; no macro, no hidden code generation.
- `save_config` builds the JSON object field-by-field manually.

No `try/catch` anywhere. No `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE` macro.

**Adding a new field** = one line in `load_config` + one line in `save_config`.

### Preserving ImGui edits as new defaults

1. **Launch** → `load_config("config.json")` — returns hardcoded defaults if file absent or malformed.
2. **Runtime** → ImGui sliders modify the live `config_` in `Stage`. Changes take effect next frame (Stage syncs `config_.pacman_speed` into Pacman before every `update`).
3. **"Save as defaults"** button → calls `save_config(config_, "config.json")`. Next launch reads the saved values.
4. **`Stage::reset()`** does *not* re-initialize `GameConfig`. Config is not game state — mid-session resets preserve all tweaks.

---

## Architecture

```
game.config      — new module: GameConfig struct + load_config / save_config
     ↑
game.stage       — owns GameConfig; passes to DebugView for editing
     ↑
game.debug       — receives GameConfig& in draw(); renders sliders + Save button
```

`Pacman` stays independent of `GameConfig`. `Stage` bridges them via `set_speed()`.

---

## Changes

### 1. `CMakeLists.txt`

Add nlohmann/json via FetchContent (after the imgui block):
```cmake
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(nlohmann_json)
```

In `target_link_libraries`: add `nlohmann_json::nlohmann_json`

In `FILE_SET CXX_MODULES FILES`: add `src/game/config.ixx`

In `PRIVATE` sources: add `src/game/config.cpp`

### 2. NEW `src/game/config.ixx`

```cpp
export module game.config;

export struct GameConfig {
    float pacman_speed = 150.0f;
};

export GameConfig load_config(const char* path);
export void save_config(const GameConfig& cfg, const char* path);
```

### 3. NEW `src/game/config.cpp`

```cpp
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
```

### 4. `src/game/stage.ixx`

- Add `import game.config;`
- Change constructor: `Stage(GameConfig config = {});`
- Add member: `GameConfig config_;`

### 5. `src/game/stage.cpp`

Constructor:
```cpp
Stage::Stage(GameConfig config) : config_(config), map_(), pacman_entity_() {}
```

In `update()`, before `pacman_entity_.update(dt)`:
```cpp
pacman_entity_.set_speed(config_.pacman_speed);
```

In `render()`:
```cpp
debug_.draw(map_, pacman_entity_.debug_state(), config_);
```

### 6. `src/main.cpp`

```cpp
import game.config;
// ...
Stage stage{ load_config("config.json") };
```

### 7. `src/game/pacman.ixx`

Add setter (inline, one line):
```cpp
void set_speed(float s) { speed_ = s; }
```

### 8. `src/game/debug.ixx`

- Add `import game.config;`
- Update `draw()`: `void draw(const Map&, const PacmanDebugState&, GameConfig&);`
- Add private: `void draw_tweaks_section(GameConfig&);`

### 9. `src/game/debug.cpp`

New section called from `draw()`:
```cpp
void DebugView::draw_tweaks_section(GameConfig& config) {
    if (!ImGui::CollapsingHeader("Tweaks", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    ImGui::SliderFloat("Pac-Man speed", &config.pacman_speed, 50.0f, 400.0f);

    if (ImGui::Button("Save as defaults"))
        save_config(config, "config.json");
}
```

---

## What Does NOT Change

- `game.types` — map/window constants are compile-time fixed, separate concern.
- `game.concepts` — `Updatable` contract (`update(float dt)`) is unaffected.
- `pacman.cpp` — accumulator logic unchanged; `set_speed` is a trivial inline setter.
- `Stage::reset()` — does not reinitialize `GameConfig`.

---

## Verification

1. Build both presets — zero new warnings.
2. Run the game, press `D` — a "Tweaks" section appears with Pac-Man speed slider at 150.
3. Drag the slider to 250 — Pac-Man accelerates immediately.
4. Click "Save as defaults" — `config.json` appears next to the executable.
5. Close and relaunch — slider starts at 250 (loaded from file).
6. Delete `config.json` — game falls back to 150 (hardcoded struct default).
