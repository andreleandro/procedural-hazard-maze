#pragma once

#include "maze/maze_grid.hpp"

#include <vector>

namespace maze {

// Algorithm version 1: fixed (1,1) start, two-step randomized Prim,
// SplitMix64 random draws and rejection-sampled bounded selection.
[[nodiscard]] MazeGrid generate_maze(const MazeConfig& config);

struct MazeLayout {
    MazeGrid terrain;
    std::vector<SafeZone> safe_zones;
};

[[nodiscard]] MazeLayout generate_layout(const MazeConfig& config);
[[nodiscard]] bool is_walkable_connected(const MazeGrid& grid);

} // namespace maze
