module;
#include <SDL.h>
#include <cstdint>
#include <string_view>

export module engine.renderer;

export struct Color {
    uint8_t r, g, b, a = 255;
};

export struct Rect {
    int x, y, w, h;
};

export class Renderer {
public:
    Renderer(std::string_view title, int width, int height);
    ~Renderer();

    // Non-copyable — owns SDL resources
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void clear(Color c);
    void draw_rect(Rect r, Color c);
    void present();

    [[nodiscard]] bool is_valid() const { return window_ && renderer_; }

    void imgui_new_frame();
    void imgui_render();

private:
    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
};