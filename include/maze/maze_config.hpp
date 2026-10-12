#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace maze {

struct SafeZone {
    std::uint32_t x;
    std::uint32_t y;
    std::uint32_t width;
    std::uint32_t height;
};

struct MazeConfig {
    static constexpr std::uint32_t min_dimension = 3;
    static constexpr std::uint32_t max_dimension = 1023;
    static constexpr std::size_t memory_limit_bytes = 64 * 1024 * 1024;
    static constexpr std::uint32_t max_safe_zones = 16;

    std::uint32_t width = 21;
    std::uint32_t height = 11;
    std::uint64_t seed = 1;
    std::vector<SafeZone> safe_zones;
    std::uint32_t automatic_safe_zone_count = 0;
    std::uint32_t automatic_safe_zone_width = 3;
    std::uint32_t automatic_safe_zone_height = 3;
    std::uint32_t additional_passage_percent = 0;
    std::uint32_t hazard_percent = 12;

    // Returns the checked cell count. Dimensions must be odd for the two-step lattice.
    [[nodiscard]] std::size_t cell_count() const;
};

} // namespace maze
