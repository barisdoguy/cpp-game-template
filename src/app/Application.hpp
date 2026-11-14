#pragma once

#include "core/FixedTimestep.hpp"
#include "game/Systems.hpp"

#include <SDL3/SDL_render.h>
#include <entt/entity/registry.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace app {

struct Config {
    int width = 1280;
    int height = 720;
    int initialBodies = 2000;
    // Quit after this many frames. Used for headless smoke tests in CI.
    std::optional<std::uint64_t> maxFrames;
    // With maxFrames: save the last rendered frame to this PNG file.
    std::string screenshotPath;
};

// Platform layer: owns the window, renderer, input and debug UI, and drives
// the simulation in game_core with a fixed timestep.
class Application {
public:
    explicit Application(Config config);
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&) = delete;
    Application& operator=(Application&&) = delete;

    // Returns the process exit code.
    int run();

private:
    bool init();
    void shutdown();
    void processEvents();
    void update(double frameSeconds);
    void drawDebugUi();
    void render(bool capture);
    bool saveScreenshot();

    Config m_config;
    SDL_Window* m_window = nullptr;
    SDL_Renderer* m_renderer = nullptr;
    bool m_sdlReady = false;
    bool m_imguiReady = false;

    entt::registry m_registry;
    game::WorldSettings m_world;
    core::FixedTimestep m_timestep{1.0 / 120.0};

    // Reused every frame so drawing allocates nothing in steady state.
    std::vector<SDL_Vertex> m_vertices;
    std::vector<int> m_indices;

    bool m_running = true;
    int m_exitCode = 0;
    bool m_paused = false;
    int m_spawnCount = 500;
    std::uint32_t m_seed = 1337;
    std::uint64_t m_frame = 0;
    double m_frameMs = 0.0;
};

} // namespace app
