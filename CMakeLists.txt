cmake_minimum_required(VERSION 3.24)
project(ProceduralHazardMaze VERSION 0.1.0 LANGUAGES CXX)

option(MAZE_BUILD_TESTS "Build maze_core tests" ON)
option(MAZE_BUILD_VIEWER "Build the raylib viewer" ON)
option(MAZE_BUILD_BENCHMARKS "Build standalone benchmarks" ON)

add_library(maze_core
    src/maze_config.cpp
    src/maze_grid.cpp
    src/maze_generator.cpp
    src/simulation.cpp
)
target_include_directories(maze_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_compile_features(maze_core PUBLIC cxx_std_20)
set_target_properties(maze_core PROPERTIES CXX_EXTENSIONS OFF)

if(MSVC)
    target_compile_options(maze_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(maze_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(maze_demo app/main.cpp)
target_link_libraries(maze_demo PRIVATE maze_core)

if(MAZE_BUILD_BENCHMARKS)
    add_executable(maze_benchmarks app/benchmarks.cpp)
    target_link_libraries(maze_benchmarks PRIVATE maze_core)
endif()

if(MAZE_BUILD_VIEWER)
    include(FetchContent)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(raylib
        URL https://github.com/raysan5/raylib/archive/refs/tags/5.5.zip
        URL_HASH SHA256=00e8a7c85f7a5221e56ee8e4e1c6642b773eae73d29379a91c8ac5149e803f42
        DOWNLOAD_EXTRACT_TIMESTAMP FALSE
    )
    FetchContent_MakeAvailable(raylib)
    add_executable(maze_viewer app/viewer.cpp)
    target_link_libraries(maze_viewer PRIVATE maze_core raylib)
endif()

if(MAZE_BUILD_TESTS)
    include(CTest)
    include(FetchContent)
    FetchContent_Declare(Catch2
        URL https://github.com/catchorg/Catch2/archive/refs/tags/v3.8.1.zip
        URL_HASH SHA256=11e422a9d5a0a1732b3845ebb374c2d17e9d04337f3e717b21210be4ec2ec45b
        DOWNLOAD_EXTRACT_TIMESTAMP FALSE
    )
    FetchContent_MakeAvailable(Catch2)

    add_executable(maze_tests tests/maze_tests.cpp)
    target_link_libraries(maze_tests PRIVATE maze_core Catch2::Catch2WithMain)
    list(APPEND CMAKE_MODULE_PATH "${catch2_SOURCE_DIR}/extras")
    include(Catch)
    catch_discover_tests(maze_tests)
endif()
