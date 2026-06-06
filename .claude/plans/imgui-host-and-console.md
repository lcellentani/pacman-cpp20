# ImGui-Hosted Game Viewport + Console Log Panel

> Per `CLAUDE.md`, on approval move this file to `.claude/plans/imgui-host-and-console.md` inside the repo so it is version-controlled.

## Context

The existing debug surface is a single ImGui window pinned to the right of the SDL-rendered game (`debug.cpp:11-12`, window sized as `WINDOW_W + DEBUG_PANEL_W` in `main.cpp:11`). To grow into a multi-panel debug/editor workbench (console log, future inspectors, ghost AI tooling), the game itself must become *a panel among panels* rather than the privileged left half of the window. There is also no logging infrastructure today — every observation goes through bespoke fields in `PacmanDebugState` / `GhostDebugState`, which doesn't scale once coroutine ghost AI starts producing transient events.

Outcome:
- Game renders into an off-screen `SDL_Texture` and is shown via `ImGui::Image()` inside a fixed-layout `"Game"` window.
- A new `"Console"` window shows a ring-buffered log history with level filters, text search, and a command input line.
- Existing `"Debug"` panel keeps its current contents, just repositioned in the new layout.
- No docking branch switch; layout is fixed via `SetNextWindowPos/Size` like the current debug panel.

## Architecture changes

### 1. Renderer — add a game render target

`src/engine/renderer.ixx` / `renderer.cpp`:

- Add `SDL_Texture* game_target_ = nullptr` plus its `int target_w_, target_h_`.
- Constructor: create the target with `SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, w, h)` after `SDL_CreateRenderer`. Destroy in the destructor before `SDL_DestroyRenderer`.
- New API:
  - `void begin_game_target()` → `SDL_SetRenderTarget(renderer_, game_target_)` then `clear({0,0,0})`.
  - `void end_game_target()` → `SDL_SetRenderTarget(renderer_, nullptr)`.
  - `[[nodiscard]] void* game_texture_id() const` → returns `static_cast<void*>(game_target_)` so callers can pass it to `ImGui::Image((ImTextureID)id, ...)` without exposing the SDL type to `game.*` modules.
  - `[[nodiscard]] int game_target_width() / _height() const`.
- The window itself shrinks: `main.cpp` constructs `Renderer{ "Pac-Man", LAYOUT_WINDOW_W, LAYOUT_WINDOW_H }`. The texture stays at `WINDOW_W × WINDOW_H` so `Map` / `Pacman` / `Ghost` keep drawing in tile-pixel space unchanged.

`imgui_impl_sdlrenderer2` already accepts `SDL_Texture*` as `ImTextureID` — verified by reading the backend in `imgui_SOURCE_DIR/backends/imgui_impl_sdlrenderer2.cpp` (uses `(SDL_Texture*)tex_id`).

### 2. New module `engine.log`

New files:
- `src/engine/log.ixx`
- `src/engine/log.cpp`

Exports:
```cpp
export enum class LogLevel { Trace, Info, Warn, Error };

export struct LogEntry {
    LogLevel    level;
    float       time_s;       // seconds since program start (steady_clock)
    std::string message;
};

export class Log {
public:
    static Log& instance();
    void push(LogLevel level, std::string message);
    [[nodiscard]] std::span<const LogEntry> entries() const;
    void clear();
    static constexpr std::size_t CAPACITY = 2048;
private:
    std::vector<LogEntry> ring_;   // sized to CAPACITY, wraps via head_
    std::size_t head_ = 0;
    // exposed entries() returns a flat copy view; see below
};
```

Free helpers for the call sites:
```cpp
export void log_info(std::string msg);
export void log_warn(std::string msg);
export void log_error(std::string msg);
export void log_trace(std::string msg);
```

Implementation notes:
- Singleton via Meyers' `static Log instance; return instance;`. Single-threaded (game loop), so no mutex.
- Ring buffer: pre-reserved `std::vector<LogEntry>` of fixed `CAPACITY`. To avoid handing a wrapped view to ImGui, keep a second `std::vector<LogEntry> flat_view_` regenerated on demand inside `entries()` — or, simpler given CAPACITY=2048, just shift entries with `erase(begin())` when full. Pick the ring; document the trade-off inline.
- `time_s` from a `std::chrono::steady_clock::time_point` captured at first `instance()` call.
- No file output (deferred — user did not select).

Add `src/engine/log.cpp` to `target_sources` and `src/engine/log.ixx` to the `CXX_MODULES` file set in `CMakeLists.txt`.

### 3. New module `game.console`

New files:
- `src/game/console.ixx`
- `src/game/console.cpp`

```cpp
export class ConsoleView {
public:
    void toggle() { visible_ = !visible_; }
    [[nodiscard]] bool is_visible() const { return visible_; }
    void draw();   // reads engine.log, renders a fixed-position window
private:
    bool visible_ = true;
    bool show_[4] { true, true, true, true };   // Trace/Info/Warn/Error filter
    char filter_[128] {};                       // ImGuiTextFilter-backed search
    char input_[256] {};                        // command line buffer
    bool auto_scroll_ = true;

    void dispatch_command(std::string_view cmd);
};
```

Layout (fixed, matching the new window dimensions in §4):
- `ImGui::SetNextWindowPos({CONSOLE_X, CONSOLE_Y}, ImGuiCond_Once);`
- `ImGui::SetNextWindowSize({CONSOLE_W, CONSOLE_H}, ImGuiCond_Once);`

Body:
- Top row: four checkboxes for levels, an `ImGui::InputTextWithHint` for the filter, a `Clear` button.
- Middle: `ImGui::BeginChild("scroll", ..., ImGuiChildFlags_Border)`, iterate `Log::instance().entries()`, skip entries not matching the filter or level, push a per-level color, draw with `ImGui::TextUnformatted`. Auto-scroll if `auto_scroll_` and `ImGui::GetScrollY() >= ImGui::GetScrollMaxY()` before submitting new lines.
- Bottom: `ImGui::InputText("##cmd", input_, ImGuiInputTextFlags_EnterReturnsTrue)`. On Enter, call `dispatch_command(input_)`, then `log_info("> " + cmd)` echo, then clear `input_`, then re-grab keyboard focus with `ImGui::SetKeyboardFocusHere(-1)`.
- `dispatch_command` initial body: parse first whitespace-delimited token; `"clear"` clears the log; anything else calls `log_warn("unknown command: ...")`. Designed as the seam for future commands; do not pre-build a registry.

### 4. Fixed-window layout

New constants in `src/game/types.ixx`:

```cpp
export constexpr int LAYOUT_GAME_X = 10;
export constexpr int LAYOUT_GAME_Y = 10;
// Game panel inner size = WINDOW_W × WINDOW_H plus ImGui title-bar/padding.
export constexpr int LAYOUT_DEBUG_X = LAYOUT_GAME_X + WINDOW_W + 40;
export constexpr int LAYOUT_DEBUG_Y = 10;
export constexpr int LAYOUT_DEBUG_W = DEBUG_PANEL_W;
export constexpr int LAYOUT_DEBUG_H = WINDOW_H;
export constexpr int LAYOUT_CONSOLE_X = LAYOUT_GAME_X;
export constexpr int LAYOUT_CONSOLE_Y = LAYOUT_GAME_Y + WINDOW_H + 40;
export constexpr int LAYOUT_CONSOLE_W = WINDOW_W + DEBUG_PANEL_W + 30;
export constexpr int LAYOUT_CONSOLE_H = 240;
export constexpr int LAYOUT_WINDOW_W = LAYOUT_CONSOLE_X + LAYOUT_CONSOLE_W + 20;
export constexpr int LAYOUT_WINDOW_H = LAYOUT_CONSOLE_Y + LAYOUT_CONSOLE_H + 20;
```

Game panel:
- `ImGui::SetNextWindowPos({LAYOUT_GAME_X, LAYOUT_GAME_Y}, ImGuiCond_Once);`
- `ImGui::Begin("Game", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar);`
- `ImGui::Image((ImTextureID)renderer.game_texture_id(), ImVec2{(float)WINDOW_W, (float)WINDOW_H});`
- `ImGui::End();`

Debug panel: update `debug.cpp:11-12` to use `LAYOUT_DEBUG_*` constants instead of `WINDOW_W + 10`.

### 5. Stage render flow

`src/game/stage.cpp` (current sequence, `stage.cpp:46-59` per scout report) becomes:

```cpp
void Stage::render(Renderer& renderer) {
    // Game world into off-screen texture
    renderer.begin_game_target();
    renderer.clear({0, 0, 0});
    map_.draw(renderer);
    pacman_entity_.draw(renderer);
    Clyde.draw(renderer);
    renderer.end_game_target();

    // ImGui frame for the whole window
    renderer.imgui_new_frame();
    renderer.clear({20, 20, 20});       // window background behind panels

    draw_game_panel(renderer);          // ImGui::Image of game_target_
    debug_.draw(map_, pacman_entity_.debug_state(),
                std::span<const GhostDebugState>{...}, config_);
    console_.draw();

    renderer.imgui_render();
    renderer.present();
}
```

Add `ConsoleView console_` member to `Stage` (next to `DebugView debug_`) and `import game.console;` in `stage.ixx`. `draw_game_panel` is a small private member that issues the `Begin/Image/End` block from §4.

Input: extend `InputState` and the `D` toggle pattern at `stage.cpp` so a second key (suggest `` ` `` / backtick, mirroring most game consoles) toggles `console_`. Symmetric `prev_console_key_` edge tracking.

### 6. Wire up logging at key sites

Once `engine.log` exists, sprinkle a *minimal* set of initial calls so the console is non-empty out of the box and to validate the pipeline. Do not retrofit everything:

- `Stage::reset()` → `log_info("stage reset")`.
- Pellet collection in `Stage::update` → `log_trace(std::format("pellet eaten, score={}", score_))`.
- `load_config` / `save_config` (`game.config`) → info on success, warn on failure paths.

Further call sites should be added organically as Phase 3 ghost AI lands.

## Critical files to modify

- `src/engine/renderer.ixx`, `src/engine/renderer.cpp` — render-target API.
- `src/engine/log.ixx`, `src/engine/log.cpp` — **new**.
- `src/game/console.ixx`, `src/game/console.cpp` — **new**.
- `src/game/types.ixx` — `LAYOUT_*` constants.
- `src/game/debug.cpp` — switch hard-coded `WINDOW_W + 10` to `LAYOUT_DEBUG_*`.
- `src/game/stage.ixx`, `src/game/stage.cpp` — render flow, console member, input toggle.
- `src/engine/input.ixx`, `src/engine/input.cpp` — add `console_toggle` to `InputState`.
- `src/main.cpp` — window size becomes `LAYOUT_WINDOW_W × LAYOUT_WINDOW_H`.
- `CMakeLists.txt` — add `engine/log.{ixx,cpp}` and `game/console.{ixx,cpp}` to `target_sources` / module file-set. Order matters: `engine.log` must precede `game.console` in the file set.

## Verification

1. `/build` (clang-ninja-debug) — module graph compiles, no `Wreorder`/import-cycle regressions.
2. Launch: window is `LAYOUT_WINDOW_W × LAYOUT_WINDOW_H`. Three panels visible — `Game` (top-left), `Debug` (top-right), `Console` (bottom, spanning width).
3. Game panel shows the maze + Pac-Man + ghost; arrow keys still steer; tile/pixel coordinates inside the panel match what they were before (texture is 1:1, not stretched).
4. `D` toggles Debug; `` ` `` toggles Console.
5. Console shows `stage reset` on launch. Eating a pellet appends `pellet eaten, score=…` trace entries; toggling the Trace checkbox hides them; filter text narrows visible lines.
6. Type `clear` + Enter — log empties; type `foo` — see `unknown command: foo` warn line.
7. Ring buffer: drive >2048 log lines (hold a movement key for a while with Trace pellet logs); confirm the panel stays responsive and the oldest entries are dropped, not duplicated.
8. Run from both MSVC and Clang/Ninja presets per CLAUDE.md build matrix.
