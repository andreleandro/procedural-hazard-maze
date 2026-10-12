#include "maze/maze_generator.hpp"
#include "maze/simulation.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <limits>
#include <queue>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

std::string snapshot(const maze::MazeGrid& grid) {
    std::string result;
    result.reserve(grid.cell_count());
    for (std::uint32_t y = 0; y < grid.height(); ++y) {
        for (std::uint32_t x = 0; x < grid.width(); ++x) {
            result += grid.at(x, y) == maze::Terrain::Floor ? '.' : '#';
        }
    }
    return result;
}

void check_spanning_tree(const maze::MazeGrid& grid) {
    std::size_t floors = 0;
    std::size_t edges = 0;
    for (std::uint32_t y = 0; y < grid.height(); ++y) {
        for (std::uint32_t x = 0; x < grid.width(); ++x) {
            const auto terrain = grid.at(x, y);
            REQUIRE((terrain == maze::Terrain::Wall || terrain == maze::Terrain::Floor));
            if (x == 0 || y == 0 || x == grid.width() - 1 || y == grid.height() - 1) {
                REQUIRE(terrain == maze::Terrain::Wall);
            }
            if (terrain == maze::Terrain::Floor) {
                ++floors;
                if (x + 1 < grid.width() && grid.at(x + 1, y) == maze::Terrain::Floor) ++edges;
                if (y + 1 < grid.height() && grid.at(x, y + 1) == maze::Terrain::Floor) ++edges;
            }
        }
    }

    // Each logical room is carved exactly once; corridors connect those rooms.
    const auto rooms = static_cast<std::size_t>((grid.width() - 1) / 2) * ((grid.height() - 1) / 2);
    REQUIRE(floors == 2 * rooms - 1);
    REQUIRE(edges == floors - 1);

    std::vector<std::uint8_t> seen(grid.cell_count(), 0);
    std::queue<std::size_t> pending;
    pending.push(grid.index(1, 1));
    seen[grid.index(1, 1)] = 1;
    std::size_t visited = 0;
    while (!pending.empty()) {
        const auto index = pending.front();
        pending.pop();
        ++visited;
        const auto x = static_cast<std::uint32_t>(index % grid.width());
        const auto y = static_cast<std::uint32_t>(index / grid.width());
        auto visit = [&](std::uint32_t nx, std::uint32_t ny) {
            const auto next = grid.index(nx, ny);
            if (!seen[next] && grid.at(nx, ny) == maze::Terrain::Floor) {
                seen[next] = 1;
                pending.push(next);
            }
        };
        if (x > 0) visit(x - 1, y);
        if (y > 0) visit(x, y - 1);
        if (x + 1 < grid.width()) visit(x + 1, y);
        if (y + 1 < grid.height()) visit(x, y + 1);
    }
    REQUIRE(visited == floors);
}

} // namespace

TEST_CASE("Grid coordinates and checked cell access") {
    maze::MazeGrid grid({3, 5, 1});
    REQUIRE(grid.cell_count() == 15);
    REQUIRE(grid.index(2, 4) == 14);
    REQUIRE(grid.contains(2, 4));
    REQUIRE_FALSE(grid.contains(3, 4));
    REQUIRE(grid.at(1, 2) == maze::Terrain::Wall);
    grid.set(1, 2, maze::Terrain::Floor);
    REQUIRE(grid.at(1, 2) == maze::Terrain::Floor);
    REQUIRE_THROWS_AS(grid.index(3, 0), std::out_of_range);
    REQUIRE_THROWS_AS(grid.at(0, 5), std::out_of_range);
    REQUIRE_THROWS_AS(grid.set(3, 0, maze::Terrain::Floor), std::out_of_range);
    REQUIRE_THROWS_AS(grid.set(0, 0, static_cast<maze::Terrain>(2)), std::invalid_argument);
}

TEST_CASE("Configuration rejects invalid and extreme dimensions") {
    REQUIRE_THROWS_AS((maze::MazeConfig{0, 3, 0}.cell_count()), std::invalid_argument);
    REQUIRE_THROWS_AS((maze::MazeConfig{2, 3, 0}.cell_count()), std::invalid_argument);
    REQUIRE_THROWS_AS((maze::MazeConfig{3, 4, 0}.cell_count()), std::invalid_argument);
    REQUIRE_THROWS_AS((maze::MazeConfig{1025, 3, 0}.cell_count()), std::invalid_argument);
    REQUIRE_THROWS_AS((maze::MazeConfig{std::numeric_limits<std::uint32_t>::max(),
                                            std::numeric_limits<std::uint32_t>::max(), 0}.cell_count()),
                      std::invalid_argument);
    REQUIRE((maze::MazeConfig{3, 3, 0}.cell_count() == 9));
    REQUIRE((maze::MazeConfig{1023, 1023, 0}.cell_count() == 1046529));
}

TEST_CASE("Same configuration produces the same layout") {
    const maze::MazeConfig config{21, 15, 123456789};
    REQUIRE(snapshot(maze::generate_maze(config)) == snapshot(maze::generate_maze(config)));
    REQUIRE(snapshot(maze::generate_maze({21, 15, 1})) !=
            snapshot(maze::generate_maze({21, 15, 2})));
    REQUIRE(snapshot(maze::generate_maze({9, 7, 42})) ==
            "#########"
            "#...#...#"
            "#.#.#.###"
            "#.#.....#"
            "#.###.#.#"
            "#.#...#.#"
            "#########");
}

TEST_CASE("Small and narrow grids terminate with a valid maze") {
    for (const auto config : {maze::MazeConfig{3, 3, 0}, maze::MazeConfig{3, 101, 42},
                              maze::MazeConfig{101, 3, 42}, maze::MazeConfig{51, 51, 999}}) {
        check_spanning_tree(maze::generate_maze(config));
    }
}

TEST_CASE("Largest supported grid terminates and remains a spanning tree") {
    check_spanning_tree(maze::generate_maze({1023, 1023, std::numeric_limits<std::uint64_t>::max()}));
}

TEST_CASE("Legacy layout keeps the version 1 terrain") {
    const maze::MazeConfig config{9, 7, 42};
    const auto layout = maze::generate_layout(config);
    REQUIRE(layout.safe_zones.empty());
    REQUIRE(snapshot(layout.terrain) == snapshot(maze::generate_maze(config)));
}

TEST_CASE("Manual and automatic safe zones are walkable and deterministic") {
    maze::MazeConfig config{21, 21, 42};
    config.safe_zones.push_back({2, 2, 4, 3});
    config.automatic_safe_zone_count = 3;
    config.automatic_safe_zone_width = 3;
    config.automatic_safe_zone_height = 2;
    const auto first = maze::generate_layout(config);
    const auto second = maze::generate_layout(config);
    REQUIRE(first.safe_zones.size() == 4);
    REQUIRE(snapshot(first.terrain) == snapshot(second.terrain));
    for (std::size_t i = 0; i < first.safe_zones.size(); ++i) {
        const auto zone = first.safe_zones[i];
        const auto repeated = second.safe_zones[i];
        REQUIRE(zone.x == repeated.x);
        REQUIRE(zone.y == repeated.y);
        for (auto y = zone.y; y < zone.y + zone.height; ++y) {
            for (auto x = zone.x; x < zone.x + zone.width; ++x) {
                REQUIRE(first.terrain.at(x, y) == maze::Terrain::Floor);
            }
        }
    }
    REQUIRE(maze::is_walkable_connected(first.terrain));
}

TEST_CASE("An isolated safe-zone floor is repaired through walls") {
    maze::MazeConfig config{31, 31, 7};
    const auto base = maze::generate_maze(config);
    bool found = false;
    for (std::uint32_t y = 2; y + 2 < base.height() && !found; ++y) {
        for (std::uint32_t x = 2; x + 2 < base.width(); ++x) {
            if (base.at(x, y) == maze::Terrain::Wall &&
                base.at(x - 1, y) == maze::Terrain::Wall &&
                base.at(x + 1, y) == maze::Terrain::Wall &&
                base.at(x, y - 1) == maze::Terrain::Wall &&
                base.at(x, y + 1) == maze::Terrain::Wall) {
                config.safe_zones.push_back({x, y, 1, 1});
                found = true;
                break;
            }
        }
    }
    REQUIRE(found);
    const auto repaired = maze::generate_layout(config);
    REQUIRE(repaired.terrain.at(config.safe_zones[0].x, config.safe_zones[0].y) == maze::Terrain::Floor);
    REQUIRE(maze::is_walkable_connected(repaired.terrain));
    std::size_t opened = 0;
    for (std::uint32_t y = 0; y < base.height(); ++y) {
        for (std::uint32_t x = 0; x < base.width(); ++x) {
            if (base.at(x, y) == maze::Terrain::Wall && repaired.terrain.at(x, y) == maze::Terrain::Floor) {
                ++opened;
            }
        }
    }
    REQUIRE(opened == 2); // The zone cell and one connecting wall.
}

TEST_CASE("Additional passages preserve connectivity") {
    maze::MazeConfig config{31, 31, 99};
    const auto base = maze::generate_layout(config);
    config.additional_passage_percent = 100;
    const auto opened = maze::generate_layout(config);
    auto count_floors = [](const maze::MazeGrid& grid) {
        std::size_t count = 0;
        for (auto cell : snapshot(grid)) count += cell == '.';
        return count;
    };
    REQUIRE(count_floors(opened.terrain) > count_floors(base.terrain));
    REQUIRE(maze::is_walkable_connected(opened.terrain));
    REQUIRE(snapshot(opened.terrain) == snapshot(maze::generate_layout(config).terrain));
}

TEST_CASE("Invalid safe zones and passage settings are rejected") {
    maze::MazeConfig config{9, 9, 1};
    config.safe_zones.push_back({0, 1, 2, 2});
    REQUIRE_THROWS_AS(maze::generate_layout(config), std::invalid_argument);
    config.safe_zones = {{7, 7, 2, 2}};
    REQUIRE_THROWS_AS(maze::generate_layout(config), std::invalid_argument);
    config.safe_zones.clear();
    config.automatic_safe_zone_count = 1;
    config.automatic_safe_zone_width = 8;
    REQUIRE_THROWS_AS(maze::generate_layout(config), std::invalid_argument);
    config.automatic_safe_zone_count = 0;
    config.additional_passage_percent = 101;
    REQUIRE_THROWS_AS(maze::generate_layout(config), std::invalid_argument);
    config.additional_passage_percent = 0;
    config.hazard_percent = 101;
    REQUIRE_THROWS_AS(maze::generate_layout(config), std::invalid_argument);
}

TEST_CASE("Player stops at walls and maze boundaries") {
    maze::MazeGrid grid({5, 5, 1});
    grid.set(1, 1, maze::Terrain::Floor);
    grid.set(2, 1, maze::Terrain::Floor);
    maze::HazardLayer hazards(grid);
    maze::Simulation simulation({std::move(grid), {}}, std::move(hazards));
    simulation.move_player(-100.0f, 0.0f);
    REQUIRE(simulation.player().x >= 1.0f);
    simulation.move_player(100.0f, 0.0f);
    REQUIRE(simulation.player().x >= 2.0f);
    REQUIRE(simulation.player().x < 3.0f);
    const auto y = simulation.player().y;
    simulation.move_player(0.0f, 10.0f);
    REQUIRE(simulation.player().y >= y);
    REQUIRE(simulation.player().y < 2.0f);
}

TEST_CASE("Hazards damage the player over elapsed time and pause stops damage") {
    maze::MazeGrid grid({3, 3, 1});
    grid.set(1, 1, maze::Terrain::Floor);
    maze::HazardLayer hazards(grid);
    hazards.set(1, 1, true);
    maze::Simulation simulation({std::move(grid), {}}, std::move(hazards));
    REQUIRE(simulation.is_exposed());
    simulation.update(0.5f);
    simulation.update(1.5f);
    REQUIRE(simulation.player().health == 80.0f);
    simulation.set_paused(true);
    simulation.update(10.0f);
    REQUIRE(simulation.player().health == 80.0f);
    REQUIRE_THROWS_AS(simulation.update(-1.0f), std::invalid_argument);
}

TEST_CASE("Safe zones prevent damage and generated hazards avoid them") {
    maze::MazeConfig config{9, 9, 8};
    config.safe_zones.push_back({1, 1, 2, 2});
    auto layout = maze::generate_layout(config);
    auto hazards = maze::generate_hazards(layout, config.seed, 100);
    REQUIRE_FALSE(hazards.at(1, 1));
    hazards.set(1, 1, true);
    maze::Simulation simulation(std::move(layout), std::move(hazards));
    REQUIRE(simulation.is_safe(1, 1));
    REQUIRE_FALSE(simulation.is_exposed());
    simulation.update(10.0f);
    REQUIRE(simulation.player().health == 100.0f);
}

TEST_CASE("Hazard and safe cell queries match a simple reference") {
    maze::MazeConfig config{21, 21, 44};
    config.safe_zones = {{2, 2, 3, 4}, {10, 10, 2, 2}};
    auto layout = maze::generate_layout(config);
    auto hazards = maze::generate_hazards(layout, config.seed, 35);
    auto repeat = maze::generate_hazards(layout, config.seed, 35);
    for (std::uint32_t y = 0; y < layout.terrain.height(); ++y) {
        for (std::uint32_t x = 0; x < layout.terrain.width(); ++x) {
            REQUIRE(hazards.at(x, y) == repeat.at(x, y));
        }
    }
    maze::Simulation simulation(std::move(layout), std::move(hazards));
    for (std::uint32_t y = 0; y < simulation.layout().terrain.height(); ++y) {
        for (std::uint32_t x = 0; x < simulation.layout().terrain.width(); ++x) {
            bool reference_safe = false;
            for (const auto& zone : simulation.layout().safe_zones) {
                if (x >= zone.x && y >= zone.y && x - zone.x < zone.width && y - zone.y < zone.height) {
                    reference_safe = true;
                }
            }
            REQUIRE(simulation.is_safe(x, y) == reference_safe);
            if (simulation.layout().terrain.at(x, y) == maze::Terrain::Wall || reference_safe) {
                REQUIRE_FALSE(simulation.hazards().at(x, y));
            }
        }
    }
    REQUIRE_THROWS_AS(simulation.hazards().at(21, 0), std::out_of_range);
    REQUIRE_THROWS_AS(maze::generate_hazards(simulation.layout(), 1, 101), std::invalid_argument);
}

TEST_CASE("Connectivity rejects a grid without a walkable start") {
    maze::MazeGrid grid({3, 3, 1});
    REQUIRE_FALSE(maze::is_walkable_connected(grid));
}
