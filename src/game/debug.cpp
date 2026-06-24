module;
#include "imgui.h"
#include <span>

module game.debug;

import engine.types;

namespace {
const char* ghost_name(GhostId id) {
    const char* names[] = { "Blinky", "Pinky", "Inky", "Clyde" };
    return names[static_cast<int>(id)];
}
}

void DebugView::draw(const Map& map, const PacmanDebugState& pacman,
                     std::span<const GhostDebugState> ghosts, const GameState& game_state,
                     GameConfig& config) {
    if (!visible_) return;

    ImGui::SetNextWindowPos({ (float)LAYOUT_DEBUG_X, (float)LAYOUT_DEBUG_Y }, ImGuiCond_Once);
    ImGui::SetNextWindowSize({ (float)LAYOUT_DEBUG_W, (float)LAYOUT_DEBUG_H }, ImGuiCond_Once);

    ImGui::Begin("Debug");
    draw_pacman_section(pacman);
    ImGui::Separator();
    draw_ghosts_table(ghosts);
    ImGui::Separator();
    draw_gamestate_section(game_state);
    ImGui::Separator();
    draw_tweaks_section(config);
    ImGui::End();

    draw_minimap_window(map, pacman, ghosts);
}

void DebugView::draw_ghosts_table(std::span<const GhostDebugState> ghosts) {
    if (!ImGui::CollapsingHeader("Ghosts", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    const char* state_names[] = { "House", "Scatter", "Chase", "Frightened", "Dead" };

    if (!ImGui::BeginTable("ghosts", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
        return;

    ImGui::TableSetupColumn("Ghost");
    ImGui::TableSetupColumn("pos");
    ImGui::TableSetupColumn("dir");
    ImGui::TableSetupColumn("speed");
    ImGui::TableSetupColumn("state");
    ImGui::TableSetupColumn("target");
    ImGui::TableHeadersRow();

    for (const GhostDebugState& ghost : ghosts) {
        ImGui::TableNextRow();

        ImGui::TableNextColumn();
        ImGui::TextColored({ ghost.color.r / 255.f, ghost.color.g / 255.f, ghost.color.b / 255.f, 1.f },
            "%s", ghost_name(ghost.id));

        ImGui::TableNextColumn();
        ImGui::Text("%d, %d", ghost.coord.col, ghost.coord.row);

        ImGui::TableNextColumn();
        ImGui::Text("%d, %d", ghost.dir_x, ghost.dir_y);

        ImGui::TableNextColumn();
        ImGui::Text("%.1f", ghost.speed);

        ImGui::TableNextColumn();
        ImGui::Text("%s", state_names[static_cast<int>(ghost.state)]);

        ImGui::TableNextColumn();
        if (ghost.target.col >= 0)
            ImGui::Text("%d, %d", ghost.target.col, ghost.target.row);
        else
            ImGui::TextUnformatted("-");
    }

    ImGui::EndTable();
}

void DebugView::draw_gamestate_section(const GameState& game_state) {
    if (!ImGui::CollapsingHeader("Game State", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    ImGui::Text("dots_eaten               %d", game_state.dots_eaten);
    ImGui::Text("dot_timer                %.2f", game_state.dot_timer);
    if (game_state.next_force_release.has_value())
        ImGui::Text("next_force_release       %s", ghost_name(game_state.next_force_release.value()));
    else
        ImGui::Text("next_force_release       <none>");
    ImGui::Text("next_ghost_release_index %zu", game_state.next_ghost_release_index);
}

void DebugView::draw_tweaks_section(GameConfig& config) {
    if (!ImGui::CollapsingHeader("Tweaks", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    ImGui::SliderFloat("Pac-Man speed", &config.pacman_speed, 50.0f, 400.0f);
    ImGui::SliderFloat("Ghost speed", &config.ghost_speed, 50.0f, 400.0f);

    ImGui::Spacing();
    ImGui::TextDisabled("Ghost house (applies on next restart)");
    ImGui::SliderInt("Pinky dot threshold", &config.ghost_house_dots_pinky, 0, 60);
    ImGui::SliderInt("Inky dot threshold", &config.ghost_house_dots_inky, 0, 100);
    ImGui::SliderInt("Clyde dot threshold", &config.ghost_house_dots_clyde, 0, 150);
    ImGui::SliderFloat("Force-release seconds", &config.ghost_house_force_release_seconds, 1.0f, 15.0f);

    if (ImGui::Button("Save as defaults"))
        save_config(config, "config.json");
}

void DebugView::draw_pacman_section(const PacmanDebugState& pacman) {
    if (!ImGui::CollapsingHeader("Pac-Man", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    ImGui::Text("pos    %d, %d", pacman.coord.col, pacman.coord.row);
    ImGui::Text("vel    %d, %d", pacman.dir_x, pacman.dir_y);
    ImGui::Text("speed  %.1f", pacman.speed);
    ImGui::Spacing();
    ImGui::Text("AABB   x=%.1f y=%.1f w=%.1f h=%.1f",
        pacman.bounds.x, pacman.bounds.y,
        pacman.bounds.width, pacman.bounds.height);
}

void DebugView::draw_minimap_window(const Map& map, const PacmanDebugState& pacman, std::span<const GhostDebugState> ghosts) {
    ImGui::SetNextWindowPos({ (float)LAYOUT_MINIMAP_X, (float)LAYOUT_MINIMAP_Y }, ImGuiCond_Once);
    ImGui::SetNextWindowSize({ (float)LAYOUT_MINIMAP_W, (float)LAYOUT_MINIMAP_H }, ImGuiCond_Once);
    ImGui::Begin("Minimap");

    // Mini tile map — each tile rendered as a small colored square
    constexpr float MINI_TILE = 12.f;
    constexpr float PADDING = 2.f;
    constexpr float SCALE = (MINI_TILE + PADDING) / static_cast<float>(TILE_SIZE);

    ImVec2 origin = ImGui::GetCursorScreenPos();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    for (int r = 0; r < MAP_ROWS; ++r) {
        for (int c = 0; c < MAP_COLS; ++c) {
            Tile t = map.tile_at_index(r, c);

            ImU32 color;
            switch (t) {
            case Tile::Wall:       color = IM_COL32(33, 33, 222, 255); break;
            case Tile::Pellet:     color = IM_COL32(255, 255, 255, 255); break;
            case Tile::SuperPellet: color = IM_COL32(255, 255, 0, 255); break;
            case Tile::Door:       color = IM_COL32(255, 182, 255, 255); break;
            case Tile::GhostHouse: color = IM_COL32(60, 40, 40, 255); break;
            default:               color = IM_COL32(20, 20, 20, 255); break;
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
        ImVec2 tl{
            origin.x + pacman.bounds.x * SCALE - 1,
            origin.y + pacman.bounds.y * SCALE - 1
        };
        ImVec2 br{
            origin.x + (pacman.bounds.x + pacman.bounds.width)  * SCALE - 1,
            origin.y + (pacman.bounds.y + pacman.bounds.height) * SCALE - 1
        };
        draw_list->AddRect(tl, br, IM_COL32(255, 0, 0, 255));
    }

    {
        const float cell = MINI_TILE + PADDING;
        auto tile_center = [&](MapCoord c) {
            return ImVec2{ origin.x + c.col * cell + MINI_TILE * 0.5f,
                           origin.y + c.row * cell + MINI_TILE * 0.5f };
        };

        for (const GhostDebugState& ghost : ghosts) {
            auto color = IM_COL32(ghost.color.r, ghost.color.g, ghost.color.b, ghost.color.a);
            {
                ImVec2 center = tile_center(ghost.coord);
                float r = MINI_TILE * 0.45f;
                draw_list->AddCircleFilled(center, r, color);
                draw_list->AddCircle(center, r, IM_COL32(0, 0, 0, 180), 0, 1.0f);
            }

            if (ghost.target.col >= 0) {
                float x = ghost.target.col * cell;
                float y = ghost.target.row * cell;

                ImVec2 tl { origin.x + x,         origin.y + y };
                ImVec2 br { tl.x + MINI_TILE,     tl.y + MINI_TILE };
                ImVec2 tr { tl.x + MINI_TILE,     tl.y };
                ImVec2 bl { tl.x,                 tl.y + MINI_TILE };

                const ImU32 faded = IM_COL32(ghost.color.r, ghost.color.g, ghost.color.b, 180);
                draw_list->AddRect(tl, br, color, 0.0f, 0, 1.5f);
                draw_list->AddLine(tl, br, faded, 1.0f);
                draw_list->AddLine(tr, bl, faded, 1.0f);
            }

            {
                const ImU32 path_color = IM_COL32(ghost.color.r, ghost.color.g, ghost.color.b, 120);
                ImVec2 prev = tile_center(ghost.coord);
                for (const MapCoord& step : ghost.path) {
                    ImVec2 cur = tile_center(step);
                    draw_list->AddLine(prev, cur, path_color, 1.5f);
                    prev = cur;
                }
            }
        }
    }

    // Advance cursor past the mini-map so ImGui layout continues correctly
    ImGui::Dummy({
        MAP_COLS * (MINI_TILE + PADDING),
        MAP_ROWS * (MINI_TILE + PADDING)
        });

    ImGui::End();
}