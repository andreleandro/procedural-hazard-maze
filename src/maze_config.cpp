#include "maze/maze_config.hpp"

#include <limits>
#include <stdexcept>

namespace maze {

std::size_t MazeConfig::cell_count() const {
    if (width < min_dimension || height < min_dimension ||
        width > max_dimension || height > max_dimension ||
        width % 2 == 0 || height % 2 == 0) {
        throw std::invalid_argument("Maze dimensions must be odd and between 3 and 1023");
    }

    const auto w = static_cast<std::size_t>(width);
    const auto h = static_cast<std::size_t>(height);
    if (w > std::numeric_limits<std::size_t>::max() / h) {
        throw std::length_error("Maze cell count overflows size_t");
    }
    const auto count = w * h;
    // Terrain, frontier marks and a conservative one-point-per-cell frontier estimate.
    constexpr std::size_t bytes_per_cell = sizeof(std::uint8_t) * 2 + sizeof(std::uint32_t) * 2;
    if (count > memory_limit_bytes / bytes_per_cell) {
        throw std::length_error("Maze exceeds the memory budget");
    }
    if (additional_passage_percent > 100 || hazard_percent > 100 ||
        safe_zones.size() + automatic_safe_zone_count > max_safe_zones) {
        throw std::invalid_argument("Invalid generation parameters");
    }
    if (automatic_safe_zone_count > 0 &&
        (automatic_safe_zone_width == 0 || automatic_safe_zone_height == 0 ||
         automatic_safe_zone_width > width - 2 || automatic_safe_zone_height > height - 2)) {
        throw std::invalid_argument("Automatic safe zone does not fit inside the maze");
    }
    for (const auto& zone : safe_zones) {
        if (zone.x == 0 || zone.y == 0 || zone.width == 0 || zone.height == 0 ||
            zone.x >= width - 1 || zone.y >= height - 1 ||
            zone.width > width - 1 - zone.x || zone.height > height - 1 - zone.y) {
            throw std::invalid_argument("Safe zone must fit inside the maze border");
        }
    }
    return count;
}

} // namespace maze
