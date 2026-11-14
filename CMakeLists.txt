cmake_minimum_required(VERSION 3.28)

project(cpp_game_template
    VERSION 0.1.0
    DESCRIPTION "C++20 game starter: SDL3, Dear ImGui, EnTT, glm, spdlog"
    LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(GAME_BUILD_TESTS "Build unit tests" ON)
option(GAME_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
option(GAME_ENABLE_SANITIZERS "Build with AddressSanitizer + UBSan (GCC/Clang)" OFF)

include(cmake/CompilerWarnings.cmake)
include(cmake/Sanitizers.cmake)
include(cmake/Dependencies.cmake)

# ---------------------------------------------------------------------------
# game_core: simulation code. No SDL, no ImGui, no rendering, so it can be
# unit-tested headless and reused by tools or a server build.
# ---------------------------------------------------------------------------
add_library(game_core STATIC
    src/game/Systems.cpp)
target_include_directories(game_core PUBLIC src)
target_link_libraries(game_core PUBLIC EnTT::EnTT glm::glm)
game_set_warnings(game_core)
game_enable_sanitizers(game_core)

# ---------------------------------------------------------------------------
# game: the executable. Owns the platform layer (window, input, renderer, UI).
# ---------------------------------------------------------------------------
add_executable(game
    src/main.cpp
    src/app/Application.cpp)
target_link_libraries(game PRIVATE game_core imgui SDL3::SDL3 spdlog::spdlog)
game_set_warnings(game)
game_enable_sanitizers(game)

if(GAME_BUILD_TESTS)
    enable_testing()
    add_executable(game_tests
        tests/test_main.cpp
        tests/test_fixed_timestep.cpp
        tests/test_systems.cpp)
    target_link_libraries(game_tests PRIVATE game_core doctest::doctest)
    game_set_warnings(game_tests)
    game_enable_sanitizers(game_tests)
    add_test(NAME game_tests COMMAND game_tests)
endif()
