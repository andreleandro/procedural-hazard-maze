#pragma once

#include "maze/maze_config.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace maze {

enum class Terrain : std::uint8_t { Wall, Floor };

// Terrain is stored independently of any future hazard layer.
class MazeGrid {
public:
    explicit MazeGrid(const MazeConfig& config);

    [[nodiscard]] std::uint32_t width() const noexcept { return width_; }
    [[nodiscard]] std::uint32_t height() const noexcept { return height_; }
    [[nodiscard]] std::size_t cell_count() const noexcept { return cells_.size(); }
    [[nodiscard]] bool contains(std::uint32_t x, std::uint32_t y) const noexcept;
    [[nodiscard]] std::size_t index(std::uint32_t x, std::uint32_t y) const;
    [[nodiscard]] Terrain at(std::uint32_t x, std::uint32_t y) const;
    void set(std::uint32_t x, std::uint32_t y, Terrain terrain);

private:
    std::uint32_t width_;
    std::uint32_t height_;
    std::vector<Terrain> cells_;
};

} // namespace maze
