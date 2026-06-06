module;
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <cstdint>
#include <string_view>

export module engine.renderer;

import engine.types;

export class Renderer {
public:
    Renderer(std::string_view title, int width, int height,
             int game_target_w, int game_target_h);
    ~Renderer();

    // Non-copyable — owns SDL resources
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void clear(Color c);
    void draw_rect(Rect r, Color c);
	void draw_circle(int cx, int cy, int radius, Color c);
    void present();

    [[nodiscard]] bool is_valid() const { return window_ && renderer_; }

    void imgui_new_frame();
    void imgui_render();

    // Off-screen game render target. Game code draws between begin/end;
    // the resulting texture is shown inside an ImGui panel.
    void begin_game_target();
    void end_game_target();
    [[nodiscard]] void* game_texture_id() const { return static_cast<void*>(game_target_); }
    [[nodiscard]] int game_target_width()  const { return target_w_; }
    [[nodiscard]] int game_target_height() const { return target_h_; }

private:
    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    SDL_Texture* game_target_ = nullptr;
    int target_w_ = 0;
    int target_h_ = 0;
};