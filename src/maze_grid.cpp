#include "maze/maze_grid.hpp"

#include <stdexcept>

namespace maze {

MazeGrid::MazeGrid(const MazeConfig& config)
    : width_(config.width), height_(config.height), cells_(config.cell_count(), Terrain::Wall) {}

bool MazeGrid::contains(std::uint32_t x, std::uint32_t y) const noexcept {
    return x < width_ && y < height_;
}

std::size_t MazeGrid::index(std::uint32_t x, std::uint32_t y) const {
    if (!contains(x, y)) {
        throw std::out_of_range("Maze coordinate is outside the grid");
    }
    return static_cast<std::size_t>(y) * width_ + x;
}

Terrain MazeGrid::at(std::uint32_t x, std::uint32_t y) const {
    return cells_[index(x, y)];
}

void MazeGrid::set(std::uint32_t x, std::uint32_t y, Terrain terrain) {
    if (terrain != Terrain::Wall && terrain != Terrain::Floor) {
        throw std::invalid_argument("Unknown terrain value");
    }
    cells_[index(x, y)] = terrain;
}

} // namespace maze
