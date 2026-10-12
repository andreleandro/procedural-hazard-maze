#include "maze/maze_generator.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <deque>
#include <limits>
#include <vector>

namespace maze {
namespace {

class SplitMix64 {
public:
    explicit SplitMix64(std::uint64_t seed) : state_(seed) {}

    std::uint64_t next() noexcept {
        state_ += UINT64_C(0x9E3779B97F4A7C15);
        auto z = state_;
        z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
        z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
        return z ^ (z >> 31);
    }

    std::size_t bounded(std::size_t bound) noexcept {
        assert(bound > 0);
        const auto n = static_cast<std::uint64_t>(bound);
        const auto threshold = (std::uint64_t{0} - n) % n;
        for (;;) {
            const auto value = next();
            if (value >= threshold) {
                return static_cast<std::size_t>(value % n);
            }
        }
    }

private:
    std::uint64_t state_;
};

struct Point { std::uint32_t x; std::uint32_t y; };
struct Offset { std::int32_t x; std::int32_t y; };
constexpr std::array<Offset, 4> offsets{{{0, 2}, {2, 0}, {0, -2}, {-2, 0}}};

std::vector<std::uint8_t> connected_floors(const MazeGrid& grid) {
    std::vector<std::uint8_t> visited(grid.cell_count(), 0);
    std::vector<std::size_t> pending;
    pending.reserve(grid.cell_count());
    if (grid.at(1, 1) != Terrain::Floor) return visited;
    const auto start = grid.index(1, 1);
    visited[start] = 1;
    pending.push_back(start);
    for (std::size_t head = 0; head < pending.size(); ++head) {
        const auto index = pending[head];
        const auto x = static_cast<std::uint32_t>(index % grid.width());
        const auto y = static_cast<std::uint32_t>(index / grid.width());
        auto visit = [&](std::uint32_t nx, std::uint32_t ny) {
            const auto next = grid.index(nx, ny);
            if (!visited[next] && grid.at(nx, ny) == Terrain::Floor) {
                visited[next] = 1;
                pending.push_back(next);
            }
        };
        if (x > 0) visit(x - 1, y);
        if (y > 0) visit(x, y - 1);
        if (x + 1 < grid.width()) visit(x + 1, y);
        if (y + 1 < grid.height()) visit(x, y + 1);
    }
    return visited;
}

void repair_disconnected_floors(MazeGrid& grid) {
    const auto count = grid.cell_count();
    for (;;) {
        const auto connected = connected_floors(grid);
        bool disconnected = false;
        for (std::uint32_t y = 1; y + 1 < grid.height() && !disconnected; ++y) {
            for (std::uint32_t x = 1; x + 1 < grid.width(); ++x) {
                if (grid.at(x, y) == Terrain::Floor && !connected[grid.index(x, y)]) {
                    disconnected = true;
                    break;
                }
            }
        }
        if (!disconnected) return;

        // A wall costs one; an existing floor costs zero.
        std::vector<std::uint32_t> distance(count, std::numeric_limits<std::uint32_t>::max());
        std::vector<std::size_t> previous(count, count);
        std::deque<std::size_t> pending;
        for (std::size_t index = 0; index < count; ++index) {
            if (connected[index]) {
                distance[index] = 0;
                pending.push_back(index);
            }
        }
        std::size_t target = count;
        while (!pending.empty() && target == count) {
            const auto index = pending.front();
            pending.pop_front();
            const auto x = static_cast<std::uint32_t>(index % grid.width());
            const auto y = static_cast<std::uint32_t>(index / grid.width());
            if (grid.at(x, y) == Terrain::Floor && !connected[index]) {
                target = index;
                break;
            }
            auto visit = [&](std::uint32_t nx, std::uint32_t ny) {
                if (nx == 0 || ny == 0 || nx + 1 == grid.width() || ny + 1 == grid.height()) return;
                const auto next = grid.index(nx, ny);
                const auto cost = grid.at(nx, ny) == Terrain::Wall ? 1u : 0u;
                if (distance[index] + cost < distance[next]) {
                    distance[next] = distance[index] + cost;
                    previous[next] = index;
                    if (cost == 0) pending.push_front(next);
                    else pending.push_back(next);
                }
            };
            if (x > 0) visit(x - 1, y);
            if (y > 0) visit(x, y - 1);
            if (x + 1 < grid.width()) visit(x + 1, y);
            if (y + 1 < grid.height()) visit(x, y + 1);
        }
        assert(target < count);
        for (auto index = target; !connected[index]; index = previous[index]) {
            grid.set(static_cast<std::uint32_t>(index % grid.width()),
                     static_cast<std::uint32_t>(index / grid.width()), Terrain::Floor);
            assert(previous[index] < count);
        }
    }
}

} // namespace

MazeGrid generate_maze(const MazeConfig& config) {
    const auto count = config.cell_count();
    MazeGrid grid(config);
    SplitMix64 rng(config.seed);
    std::vector<std::uint8_t> marked(count, 0);
    std::vector<Point> frontier;
    frontier.reserve(count / 4 + 1);

    auto add_neighbors = [&](Point center) {
        for (const auto& delta : offsets) {
            const auto x = static_cast<std::int64_t>(center.x) + delta.x;
            const auto y = static_cast<std::int64_t>(center.y) + delta.y;
            if (x > 0 && y > 0 && x < static_cast<std::int64_t>(config.width - 1) &&
                y < static_cast<std::int64_t>(config.height - 1)) {
                const auto neighbor_x = static_cast<std::uint32_t>(x);
                const auto neighbor_y = static_cast<std::uint32_t>(y);
                const auto neighbor_index = grid.index(neighbor_x, neighbor_y);
                if (grid.at(neighbor_x, neighbor_y) == Terrain::Wall && !marked[neighbor_index]) {
                    frontier.push_back({neighbor_x, neighbor_y});
                    marked[neighbor_index] = 1;
                }
            }
        }
    };

    grid.set(1, 1, Terrain::Floor);
    add_neighbors({1, 1});
    while (!frontier.empty()) {
        const auto chosen = rng.bounded(frontier.size());
        const auto cell = frontier[chosen];
        frontier[chosen] = frontier.back();
        frontier.pop_back();

        std::array<Point, 4> connections{};
        std::size_t connection_count = 0;
        for (const auto& delta : offsets) {
            const auto x = static_cast<std::int64_t>(cell.x) + delta.x;
            const auto y = static_cast<std::int64_t>(cell.y) + delta.y;
            if (x > 0 && y > 0 && x < static_cast<std::int64_t>(config.width - 1) &&
                y < static_cast<std::int64_t>(config.height - 1)) {
                const auto neighbor_x = static_cast<std::uint32_t>(x);
                const auto neighbor_y = static_cast<std::uint32_t>(y);
                if (grid.at(neighbor_x, neighbor_y) == Terrain::Floor) {
                    connections[connection_count++] = {neighbor_x, neighbor_y};
                }
            }
        }
        // Every frontier cell was added by a carved neighbor.
        assert(connection_count > 0 && connection_count <= connections.size());
        const auto parent = connections[rng.bounded(connection_count)];
        grid.set((cell.x + parent.x) / 2, (cell.y + parent.y) / 2, Terrain::Floor);
        grid.set(cell.x, cell.y, Terrain::Floor);
        add_neighbors(cell);
    }
    return grid;
}

bool is_walkable_connected(const MazeGrid& grid) {
    if (grid.at(1, 1) != Terrain::Floor) return false;
    const auto visited = connected_floors(grid);
    for (std::uint32_t y = 0; y < grid.height(); ++y) {
        for (std::uint32_t x = 0; x < grid.width(); ++x) {
            if (grid.at(x, y) == Terrain::Floor && !visited[grid.index(x, y)]) return false;
        }
    }
    return true;
}

MazeLayout generate_layout(const MazeConfig& config) {
    static_cast<void>(config.cell_count());
    MazeLayout layout{generate_maze(config), config.safe_zones};
    SplitMix64 rng(config.seed ^ UINT64_C(0xD1B54A32D192ED03));
    for (std::uint32_t i = 0; i < config.automatic_safe_zone_count; ++i) {
        const auto x = 1u + static_cast<std::uint32_t>(rng.bounded(config.width - 1 - config.automatic_safe_zone_width));
        const auto y = 1u + static_cast<std::uint32_t>(rng.bounded(config.height - 1 - config.automatic_safe_zone_height));
        layout.safe_zones.push_back({x, y, config.automatic_safe_zone_width, config.automatic_safe_zone_height});
    }
    for (const auto& zone : layout.safe_zones) {
        for (std::uint32_t y = zone.y; y < zone.y + zone.height; ++y) {
            for (std::uint32_t x = zone.x; x < zone.x + zone.width; ++x) {
                layout.terrain.set(x, y, Terrain::Floor);
            }
        }
    }
    repair_disconnected_floors(layout.terrain);
    if (config.additional_passage_percent > 0) {
        for (std::uint32_t y = 1; y + 1 < layout.terrain.height(); ++y) {
            for (std::uint32_t x = 1; x + 1 < layout.terrain.width(); ++x) {
                if (layout.terrain.at(x, y) != Terrain::Wall) continue;
                const auto horizontal = layout.terrain.at(x - 1, y) == Terrain::Floor &&
                                        layout.terrain.at(x + 1, y) == Terrain::Floor;
                const auto vertical = layout.terrain.at(x, y - 1) == Terrain::Floor &&
                                      layout.terrain.at(x, y + 1) == Terrain::Floor;
                if ((horizontal || vertical) && rng.bounded(100) < config.additional_passage_percent) {
                    layout.terrain.set(x, y, Terrain::Floor);
                }
            }
        }
    }
    return layout;
}

} // namespace maze
