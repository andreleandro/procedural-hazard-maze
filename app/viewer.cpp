#include "maze/simulation.hpp"

#include <raylib.h>

#include <algorithm>
#include <cstdint>
#include <exception>
#include <iostream>
#include <utility>

namespace {

maze::Simulation make_simulation(const maze::MazeConfig& config) {
    auto layout = maze::generate_layout(config);
    auto hazards = maze::generate_hazards(layout, config.seed, config.hazard_percent);
    return {std::move(layout), std::move(hazards)};
}

void draw_viewer(const maze::Simulation& simulation, const maze::MazeConfig& config, bool connected) {
    constexpr int grid_left = 24;
    constexpr int grid_top = 70;
    constexpr int grid_width = 900;
    constexpr int grid_height = 680;
    const auto& grid = simulation.layout().terrain;
    const int tile = std::max(1, std::min(grid_width / static_cast<int>(grid.width()),
                                          grid_height / static_cast<int>(grid.height())));
    DrawText("PROCEDURAL HAZARD MAZE", 24, 20, 28, RAYWHITE);
    for (std::uint32_t y = 0; y < grid.height(); ++y) {
        for (std::uint32_t x = 0; x < grid.width(); ++x) {
            Color color = {35, 46, 69, 255};
            if (grid.at(x, y) == maze::Terrain::Floor) color = {200, 208, 207, 255};
            if (simulation.hazards().at(x, y)) color = {211, 82, 75, 255};
            if (simulation.is_safe(x, y)) color = {77, 174, 130, 255};
            DrawRectangle(grid_left + static_cast<int>(x) * tile,
                          grid_top + static_cast<int>(y) * tile, tile, tile, color);
        }
    }
    DrawCircle(grid_left + simulation.player().x * tile,
               grid_top + simulation.player().y * tile,
               std::max(3.0f, tile * 0.32f), YELLOW);

    constexpr int panel_x = 950;
    DrawText("GENERATION", panel_x, 75, 22, RAYWHITE);
    DrawText(TextFormat("Seed: %llu", static_cast<unsigned long long>(config.seed)), panel_x, 112, 18, LIGHTGRAY);
    DrawText(TextFormat("Size: %u x %u", config.width, config.height), panel_x, 139, 18, LIGHTGRAY);
    DrawText(TextFormat("Extra passages: %u%%", config.additional_passage_percent), panel_x, 166, 18, LIGHTGRAY);
    DrawText(TextFormat("Safe zones: %u", static_cast<unsigned>(simulation.layout().safe_zones.size())),
             panel_x, 193, 18, LIGHTGRAY);
    DrawText(TextFormat("Connected: %s", connected ? "YES" : "NO"),
             panel_x, 220, 18, LIGHTGRAY);
    DrawText(TextFormat("Health: %.1f", simulation.player().health), panel_x, 260, 22, YELLOW);
    DrawText(simulation.paused() ? "PAUSED" : "RUNNING", panel_x, 290, 20, RAYWHITE);
    DrawText("CONTROLS", panel_x, 345, 22, RAYWHITE);
    DrawText("WASD / arrows  Move", panel_x, 382, 16, LIGHTGRAY);
    DrawText("P  Pause / resume", panel_x, 407, 16, LIGHTGRAY);
    DrawText("N  New seed", panel_x, 432, 16, LIGHTGRAY);
    DrawText("R  Regenerate", panel_x, 457, 16, LIGHTGRAY);
    DrawText("J / K  Seed - / +", panel_x, 482, 16, LIGHTGRAY);
    DrawText("Q / E  Width - / +", panel_x, 507, 16, LIGHTGRAY);
    DrawText("Z / X  Height - / +", panel_x, 532, 16, LIGHTGRAY);
    DrawText("B / M  Passages - / +", panel_x, 557, 16, LIGHTGRAY);
    DrawText("LEGEND", panel_x, 610, 22, RAYWHITE);
    DrawText("Blue wall  Gray floor", panel_x, 647, 16, LIGHTGRAY);
    DrawText("Red hazard  Green safe", panel_x, 672, 16, LIGHTGRAY);
    DrawText("Yellow player", panel_x, 697, 16, LIGHTGRAY);
}

} // namespace

int main() {
    maze::MazeConfig config{41, 31, 1};
    config.automatic_safe_zone_count = 2;
    config.additional_passage_percent = 10;
    try {
        auto simulation = make_simulation(config);
        bool connected = maze::is_walkable_connected(simulation.layout().terrain);
        InitWindow(1230, 780, "Procedural Hazard Maze");
        SetTargetFPS(60);
        while (!WindowShouldClose()) {
            bool regenerate = IsKeyPressed(KEY_R) || IsKeyPressed(KEY_N);
            if (IsKeyPressed(KEY_N) || IsKeyPressed(KEY_K)) { ++config.seed; regenerate = true; }
            if (IsKeyPressed(KEY_J)) { --config.seed; regenerate = true; }
            if (IsKeyPressed(KEY_Q) && config.width > 11) { config.width -= 2; regenerate = true; }
            if (IsKeyPressed(KEY_E) && config.width < 101) { config.width += 2; regenerate = true; }
            if (IsKeyPressed(KEY_Z) && config.height > 11) { config.height -= 2; regenerate = true; }
            if (IsKeyPressed(KEY_X) && config.height < 101) { config.height += 2; regenerate = true; }
            if (IsKeyPressed(KEY_B) && config.additional_passage_percent >= 10) {
                config.additional_passage_percent -= 10;
                regenerate = true;
            }
            if (IsKeyPressed(KEY_M) && config.additional_passage_percent <= 90) {
                config.additional_passage_percent += 10;
                regenerate = true;
            }
            if (regenerate) {
                simulation = make_simulation(config);
                connected = maze::is_walkable_connected(simulation.layout().terrain);
            }
            if (IsKeyPressed(KEY_P)) simulation.set_paused(!simulation.paused());
            const float elapsed = GetFrameTime();
            const float movement_time = std::min(elapsed, 0.05f);
            float dx = 0;
            float dy = 0;
            if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) dx -= 1;
            if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) dx += 1;
            if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) dy -= 1;
            if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) dy += 1;
            simulation.move_player(dx * movement_time * 4.0f, dy * movement_time * 4.0f);
            simulation.update(elapsed);

            BeginDrawing();
            ClearBackground({20, 27, 43, 255});
            draw_viewer(simulation, config, connected);
            EndDrawing();
        }
        CloseWindow();
    } catch (const std::exception& error) {
        std::cerr << "Viewer error: " << error.what() << '\n';
        return 1;
    }
}
