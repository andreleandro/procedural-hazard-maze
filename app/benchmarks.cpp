#include "maze/simulation.hpp"

#include <chrono>
#include <cstdint>
#include <iostream>

namespace {

using Clock = std::chrono::steady_clock;

template <typename Action>
double average_ms(int repetitions, Action action) {
    const auto start = Clock::now();
    for (int i = 0; i < repetitions; ++i) action(i);
    const auto elapsed = std::chrono::duration<double, std::milli>(Clock::now() - start);
    return elapsed.count() / repetitions;
}

} // namespace

int main() {
    maze::MazeConfig config{101, 101, 42};
    config.automatic_safe_zone_count = 2;
    config.additional_passage_percent = 10;
    volatile std::size_t result = 0;

    const auto generation_ms = average_ms(30, [&](int iteration) {
        config.seed = 42 + static_cast<std::uint64_t>(iteration);
        const auto layout = maze::generate_layout(config);
        result = result + layout.terrain.cell_count();
    });
    config.seed = 42;
    const auto layout = maze::generate_layout(config);
    const auto hazards = maze::generate_hazards(layout, config.seed, config.hazard_percent);
    const auto connectivity_ms = average_ms(100, [&](int) {
        result = result + maze::is_walkable_connected(layout.terrain);
    });
    const auto query_ms = average_ms(100, [&](int) {
        for (std::uint32_t y = 0; y < layout.terrain.height(); ++y) {
            for (std::uint32_t x = 0; x < layout.terrain.width(); ++x) {
                result = result + hazards.at(x, y);
            }
        }
    });
    std::cout << "101x101 maze, MSVC release recommended\n"
              << "Generation: " << generation_ms << " ms per maze (30 runs)\n"
              << "Connectivity BFS: " << connectivity_ms << " ms per run (100 runs)\n"
              << "Hazard queries: " << query_ms << " ms per 10,201 queries (100 runs)\n"
              << "Checksum: " << result << '\n';
}
