#include "maze/maze_generator.hpp"

#include <charconv>
#include <cstdint>
#include <exception>
#include <iostream>
#include <string_view>
#include <system_error>

namespace {

template <typename T>
bool parse_number(std::string_view text, T& result) {
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), result);
    return error == std::errc{} && end == text.data() + text.size();
}

} // namespace

int main(int argc, char* argv[]) {
    maze::MazeConfig config;
    if (argc != 1 && argc != 4) {
        std::cerr << "Usage: maze_demo [odd_width odd_height seed]\n";
        return 2;
    }
    if (argc == 4 && (!parse_number(argv[1], config.width) ||
                      !parse_number(argv[2], config.height) ||
                      !parse_number(argv[3], config.seed))) {
        std::cerr << "Width, height and seed must be nonnegative integers.\n";
        return 2;
    }

    try {
        const auto grid = maze::generate_maze(config);
        for (std::uint32_t y = 0; y < grid.height(); ++y) {
            for (std::uint32_t x = 0; x < grid.width(); ++x) {
                std::cout << (grid.at(x, y) == maze::Terrain::Floor ? ' ' : '#');
            }
            std::cout << '\n';
        }
    } catch (const std::exception& error) {
        std::cerr << "Cannot generate maze: " << error.what() << '\n';
        return 1;
    }
}
