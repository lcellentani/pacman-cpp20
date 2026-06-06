module;
#include <SDL.h>
#include <stdexcept>
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"

module engine.renderer;

Renderer::Renderer(std::string_view title, int width, int height,
                   int game_target_w, int game_target_h)
    : target_w_(game_target_w), target_h_(game_target_h) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
        throw std::runtime_error(SDL_GetError());

    window_ = SDL_CreateWindow(
        title.data(),
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        width, height,
        SDL_WINDOW_SHOWN
    );
    if (!window_) throw std::runtime_error(SDL_GetError());

    renderer_ = SDL_CreateRenderer(window_, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer_) throw std::runtime_error(SDL_GetError());

    game_target_ = SDL_CreateTexture(renderer_,
        SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET,
        target_w_, target_h_);
    if (!game_target_) throw std::runtime_error(SDL_GetError());

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplSDL2_InitForSDLRenderer(window_, renderer_);
    ImGui_ImplSDLRenderer2_Init(renderer_);
}

Renderer::~Renderer() {
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    if (game_target_) SDL_DestroyTexture(game_target_);
    if (renderer_) SDL_DestroyRenderer(renderer_);
    if (window_)   SDL_DestroyWindow(window_);
    SDL_Quit();
}

void Renderer::begin_game_target() {
    SDL_SetRenderTarget(renderer_, game_target_);
}

void Renderer::end_game_target() {
    SDL_SetRenderTarget(renderer_, nullptr);
}

void Renderer::clear(Color c) {
    SDL_SetRenderDrawColor(renderer_, c.r, c.g, c.b, c.a);
    SDL_RenderClear(renderer_);
}

void Renderer::draw_rect(Rect r, Color c) {
    SDL_SetRenderDrawColor(renderer_, c.r, c.g, c.b, c.a);
    SDL_Rect sdl_rect{ r.x, r.y, r.w, r.h };
    SDL_RenderFillRect(renderer_, &sdl_rect);
}

void Renderer::draw_circle(int cx, int cy, int radius, Color c) {
    SDL_SetRenderDrawColor(renderer_, c.r, c.g, c.b, c.a);
    for (int w = 0; w < radius * 2; w++) {
        for (int h = 0; h < radius * 2; h++) {
            int dx = radius - w; // horizontal offset
            int dy = radius - h; // vertical offset
            if ((dx*dx + dy*dy) <= (radius * radius)) {
                SDL_RenderDrawPoint(renderer_, cx + dx, cy + dy);
            }
        }
	}
}

void Renderer::present() {
    SDL_RenderPresent(renderer_);
}

void Renderer::imgui_new_frame() {
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
}

void Renderer::imgui_render() {
    ImGui::Render();
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer_);
}