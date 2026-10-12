#include "maze/simulation.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace maze {
namespace {

std::uint64_t next_random(std::uint64_t& state) noexcept {
    state += UINT64_C(0x9E3779B97F4A7C15);
    auto value = state;
    value = (value ^ (value >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
    value = (value ^ (value >> 27)) * UINT64_C(0x94D049BB133111EB);
    return value ^ (value >> 31);
}

} // namespace

HazardLayer::HazardLayer(const MazeGrid& terrain)
    : width_(terrain.width()), height_(terrain.height()), cells_(terrain.cell_count(), 0) {}

bool HazardLayer::at(std::uint32_t x, std::uint32_t y) const {
    if (x >= width_ || y >= height_) throw std::out_of_range("Hazard coordinate is outside the grid");
    return cells_[static_cast<std::size_t>(y) * width_ + x] != 0;
}

void HazardLayer::set(std::uint32_t x, std::uint32_t y, bool active) {
    if (x >= width_ || y >= height_) throw std::out_of_range("Hazard coordinate is outside the grid");
    cells_[static_cast<std::size_t>(y) * width_ + x] = active ? 1 : 0;
}

HazardLayer generate_hazards(const MazeLayout& layout, std::uint64_t seed,
                             std::uint32_t probability_percent) {
    if (probability_percent > 100) throw std::invalid_argument("Hazard probability exceeds 100");
    HazardLayer hazards(layout.terrain);
    auto state = seed ^ UINT64_C(0x94D049BB133111EB);
    for (std::uint32_t y = 0; y < layout.terrain.height(); ++y) {
        for (std::uint32_t x = 0; x < layout.terrain.width(); ++x) {
            const auto random = next_random(state);
            if (layout.terrain.at(x, y) != Terrain::Floor) continue;
            bool safe = false;
            for (const auto& zone : layout.safe_zones) {
                if (x >= zone.x && y >= zone.y && x - zone.x < zone.width && y - zone.y < zone.height) {
                    safe = true;
                    break;
                }
            }
            if (!safe && random % 100 < probability_percent) hazards.set(x, y, true);
        }
    }
    return hazards;
}

Simulation::Simulation(MazeLayout layout, HazardLayer hazards)
    : layout_(std::move(layout)), hazards_(std::move(hazards)),
      safe_cells_(layout_.terrain.cell_count(), 0) {
    if (hazards_.width() != layout_.terrain.width() ||
        hazards_.height() != layout_.terrain.height()) {
        throw std::invalid_argument("Hazard layer dimensions must match terrain");
    }
    for (const auto& zone : layout_.safe_zones) {
        for (auto y = zone.y; y < zone.y + zone.height; ++y) {
            for (auto x = zone.x; x < zone.x + zone.width; ++x) {
                safe_cells_[layout_.terrain.index(x, y)] = 1;
            }
        }
    }
    if (layout_.terrain.at(1, 1) != Terrain::Floor) {
        throw std::invalid_argument("Player start must be walkable");
    }
}

bool Simulation::is_safe(std::uint32_t x, std::uint32_t y) const {
    return safe_cells_[layout_.terrain.index(x, y)] != 0;
}

bool Simulation::is_exposed() const {
    const auto x = static_cast<std::uint32_t>(player_.x);
    const auto y = static_cast<std::uint32_t>(player_.y);
    return hazards_.at(x, y) && !is_safe(x, y);
}

bool Simulation::can_occupy(float x, float y) const {
    if (!std::isfinite(x) || !std::isfinite(y) || x < 0 || y < 0 ||
        x >= layout_.terrain.width() || y >= layout_.terrain.height()) return false;
    return layout_.terrain.at(static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y)) == Terrain::Floor;
}

void Simulation::move_player(float dx, float dy) {
    if (paused_ || player_.health <= 0 || !std::isfinite(dx) || !std::isfinite(dy)) return;
    dx = std::clamp(dx, -static_cast<float>(layout_.terrain.width()),
                    static_cast<float>(layout_.terrain.width()));
    dy = std::clamp(dy, -static_cast<float>(layout_.terrain.height()),
                    static_cast<float>(layout_.terrain.height()));
    const auto steps = static_cast<int>(std::ceil(std::max(std::abs(dx), std::abs(dy)) * 4));
    if (steps == 0) return;
    const auto step_x = dx / steps;
    const auto step_y = dy / steps;
    for (int i = 0; i < steps; ++i) {
        if (can_occupy(player_.x + step_x, player_.y)) player_.x += step_x;
        if (can_occupy(player_.x, player_.y + step_y)) player_.y += step_y;
    }
}

void Simulation::update(float elapsed_seconds) {
    if (!std::isfinite(elapsed_seconds) || elapsed_seconds < 0) {
        throw std::invalid_argument("Elapsed time must be finite and nonnegative");
    }
    if (!paused_ && is_exposed()) {
        player_.health = std::max(0.0f, player_.health - 10.0f * elapsed_seconds);
    }
}

} // namespace maze
