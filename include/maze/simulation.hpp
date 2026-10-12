#pragma once

#include "maze/maze_generator.hpp"

#include <cstdint>
#include <vector>

namespace maze {

class HazardLayer {
public:
    explicit HazardLayer(const MazeGrid& terrain);
    [[nodiscard]] std::uint32_t width() const noexcept { return width_; }
    [[nodiscard]] std::uint32_t height() const noexcept { return height_; }
    [[nodiscard]] bool at(std::uint32_t x, std::uint32_t y) const;
    void set(std::uint32_t x, std::uint32_t y, bool active);

private:
    std::uint32_t width_;
    std::uint32_t height_;
    std::vector<std::uint8_t> cells_;
};

[[nodiscard]] HazardLayer generate_hazards(const MazeLayout& layout, std::uint64_t seed,
                                           std::uint32_t probability_percent);

struct Player {
    float x = 1.5f;
    float y = 1.5f;
    float health = 100.0f;
};

class Simulation {
public:
    Simulation(MazeLayout layout, HazardLayer hazards);

    [[nodiscard]] const MazeLayout& layout() const noexcept { return layout_; }
    [[nodiscard]] const HazardLayer& hazards() const noexcept { return hazards_; }
    [[nodiscard]] const Player& player() const noexcept { return player_; }
    [[nodiscard]] bool is_safe(std::uint32_t x, std::uint32_t y) const;
    [[nodiscard]] bool is_exposed() const;
    [[nodiscard]] bool paused() const noexcept { return paused_; }
    void set_paused(bool paused) noexcept { paused_ = paused; }
    void move_player(float dx, float dy);
    void update(float elapsed_seconds);

private:
    [[nodiscard]] bool can_occupy(float x, float y) const;

    MazeLayout layout_;
    HazardLayer hazards_;
    std::vector<std::uint8_t> safe_cells_;
    Player player_;
    bool paused_ = false;
};

} // namespace maze
