module;
#include "imgui.h"

module game.debug;

void DebugView::draw(const Map& map, const PacmanDebugState& pacman) {
    if (!visible_) return;

    ImGui::SetNextWindowPos({ WINDOW_W + 10.f, 10.f }, ImGuiCond_Once);
    ImGui::SetNextWindowSize({ DEBUG_PANEL_W - 20.f, WINDOW_H - 20.f }, ImGuiCond_Once);

    ImGui::Begin("Debug");
    draw_pacman_section(pacman);
    ImGui::Separator();
    draw_map_section(map, pacman);
    ImGui::End();
}

void DebugView::draw_pacman_section(const PacmanDebugState& pacman) {
    if (!ImGui::CollapsingHeader("Pac-Man", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    ImGui::Text("pos    %.d, %.d", pacman.col, pacman.row);
    ImGui::Text("vel    %.d, %.d", pacman.dir_x, pacman.dir_y);
    ImGui::Text("speed  %.1f", pacman.speed);
    ImGui::Spacing();
    ImGui::Text("AABB   x=%.1f y=%.1f w=%.1f h=%.1f",
        pacman.bounds.x, pacman.bounds.y,
        pacman.bounds.width, pacman.bounds.height);
}

void DebugView::draw_map_section(const Map& map, const PacmanDebugState& pacman) {
    if (!ImGui::CollapsingHeader("Map", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    // Mini tile map — each tile rendered as a small colored square
    constexpr float MINI_TILE = 12.f;
    constexpr float PADDING = 2.f;

    ImVec2 origin = ImGui::GetCursorScreenPos();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    for (int r = 0; r < MAP_ROWS; ++r) {
        for (int c = 0; c < MAP_COLS; ++c) {
            Tile t = map.tile_at_index(r, c);

            ImU32 color;
            switch (t) {
            case Tile::Wall:   color = IM_COL32(33, 33, 222, 255); break;
            case Tile::Pellet: color = IM_COL32(255, 255, 255, 255); break;
            default:           color = IM_COL32(20, 20, 20, 255); break;
            }

            ImVec2 tl{
                origin.x + c * (MINI_TILE + PADDING),
                origin.y + r * (MINI_TILE + PADDING)
            };
            ImVec2 br{ tl.x + MINI_TILE, tl.y + MINI_TILE };
            draw_list->AddRectFilled(tl, br, color);
        }
    }

    {
        constexpr float SCALE = (MINI_TILE + PADDING) / static_cast<float>(TILE_SIZE);
        ImVec2 tl{
            origin.x + pacman.bounds.x * SCALE,
            origin.y + pacman.bounds.y * SCALE
        };
        ImVec2 br{
            origin.x + (pacman.bounds.x + pacman.bounds.width)  * SCALE,
            origin.y + (pacman.bounds.y + pacman.bounds.height) * SCALE
        };
        draw_list->AddRect(tl, br, IM_COL32(255, 0, 0, 255));
    }

    // Advance cursor past the mini-map so ImGui layout continues correctly
    ImGui::Dummy({
        MAP_COLS * (MINI_TILE + PADDING),
        MAP_ROWS * (MINI_TILE + PADDING)
        });
}