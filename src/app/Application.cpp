#include "app/Application.hpp"

#include "game/Components.hpp"

#include <SDL3/SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <spdlog/spdlog.h>

#include <array>
#include <cstddef>
#include <string_view>
#include <utility>

namespace app {
namespace {

std::string_view orUnknown(const char* text) {
    return text != nullptr ? std::string_view{text} : std::string_view{"unknown"};
}

constexpr float kByteToUnit = 1.0f / 255.0f;

} // namespace

Application::Application(Config config) : m_config(std::move(config)) {
    m_world.size = {static_cast<float>(m_config.width), static_cast<float>(m_config.height)};
}

Application::~Application() {
    shutdown();
}

bool Application::init() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        spdlog::critical("SDL_Init failed: {}", SDL_GetError());
        return false;
    }
    m_sdlReady = true;

    m_window = SDL_CreateWindow("cpp-game-template", m_config.width, m_config.height, SDL_WINDOW_RESIZABLE);
    if (m_window == nullptr) {
        spdlog::critical("SDL_CreateWindow failed: {}", SDL_GetError());
        return false;
    }

    m_renderer = SDL_CreateRenderer(m_window, nullptr);
    if (m_renderer == nullptr) {
        spdlog::critical("SDL_CreateRenderer failed: {}", SDL_GetError());
        return false;
    }
    SDL_SetRenderVSync(m_renderer, 1);

    spdlog::info("SDL {}.{}.{} | video: {} | renderer: {}", SDL_MAJOR_VERSION, SDL_MINOR_VERSION,
                 SDL_MICRO_VERSION, orUnknown(SDL_GetCurrentVideoDriver()),
                 orUnknown(SDL_GetRendererName(m_renderer)));

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr; // don't write imgui.ini next to the executable
    ImGui::StyleColorsDark();
    ImGui_ImplSDL3_InitForSDLRenderer(m_window, m_renderer);
    ImGui_ImplSDLRenderer3_Init(m_renderer);
    m_imguiReady = true;

    game::spawnBodies(m_registry, m_world, m_config.initialBodies, m_seed);
    spdlog::info("Spawned {} bodies", m_config.initialBodies);
    return true;
}

void Application::shutdown() {
    if (m_imguiReady) {
        ImGui_ImplSDLRenderer3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
        m_imguiReady = false;
    }
    if (m_renderer != nullptr) {
        SDL_DestroyRenderer(m_renderer);
        m_renderer = nullptr;
    }
    if (m_window != nullptr) {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }
    if (m_sdlReady) {
        SDL_Quit();
        m_sdlReady = false;
    }
}

int Application::run() {
    if (!init()) {
        return 1;
    }

    const auto frequency = static_cast<double>(SDL_GetPerformanceFrequency());
    std::uint64_t last = SDL_GetPerformanceCounter();

    while (m_running) {
        const std::uint64_t now = SDL_GetPerformanceCounter();
        const double frameSeconds = static_cast<double>(now - last) / frequency;
        last = now;
        m_frameMs = frameSeconds * 1000.0;

        const bool lastFrame = m_config.maxFrames && m_frame + 1 >= *m_config.maxFrames;

        processEvents();
        update(frameSeconds);
        render(lastFrame && !m_config.screenshotPath.empty());

        ++m_frame;
        if (lastFrame) {
            m_running = false;
        }
    }

    spdlog::info("Exiting after {} frames", m_frame);
    return m_exitCode;
}

bool Application::saveScreenshot() {
    SDL_Surface* surface = SDL_RenderReadPixels(m_renderer, nullptr);
    if (surface == nullptr) {
        spdlog::error("SDL_RenderReadPixels failed: {}", SDL_GetError());
        return false;
    }
    const bool saved = SDL_SavePNG(surface, m_config.screenshotPath.c_str());
    SDL_DestroySurface(surface);
    if (!saved) {
        spdlog::error("Could not save {}: {}", m_config.screenshotPath, SDL_GetError());
        return false;
    }
    spdlog::info("Saved screenshot to {}", m_config.screenshotPath);
    return true;
}

void Application::processEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL3_ProcessEvent(&event);
        switch (event.type) {
        case SDL_EVENT_QUIT:
            m_running = false;
            break;
        case SDL_EVENT_KEY_DOWN:
            if (ImGui::GetIO().WantCaptureKeyboard) {
                break;
            }
            if (event.key.key == SDLK_ESCAPE) {
                m_running = false;
            } else if (event.key.key == SDLK_SPACE) {
                m_paused = !m_paused;
            }
            break;
        default:
            break;
        }
    }
}

void Application::update(double frameSeconds) {
    int width = 0;
    int height = 0;
    if (SDL_GetCurrentRenderOutputSize(m_renderer, &width, &height)) {
        m_world.size = {static_cast<float>(width), static_cast<float>(height)};
    }

    if (m_paused) {
        return;
    }

    const int steps = m_timestep.advance(frameSeconds);
    const auto dt = static_cast<float>(m_timestep.step());
    for (int i = 0; i < steps; ++i) {
        game::integrate(m_registry, m_world, dt);
        game::resolveBounds(m_registry, m_world);
    }
}

void Application::drawDebugUi() {
    ImGui::SetNextWindowPos({12.0f, 12.0f}, ImGuiCond_FirstUseEver);
    ImGui::Begin("Debug", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    const std::size_t bodies = m_registry.view<game::Transform>().size();
    ImGui::Text("Frame: %.2f ms (%.0f FPS)", m_frameMs, static_cast<double>(ImGui::GetIO().Framerate));
    ImGui::Text("Simulation: fixed %.2f ms step", m_timestep.step() * 1000.0);
    ImGui::Text("Bodies: %zu (1 draw call)", bodies);
    ImGui::Separator();

    ImGui::Checkbox("Paused (Space)", &m_paused);
    ImGui::SliderFloat("Time scale", &m_world.timeScale, 0.0f, 3.0f);
    ImGui::SliderFloat2("Gravity", &m_world.gravity.x, -2000.0f, 2000.0f);
    ImGui::SliderInt("Spawn count", &m_spawnCount, 1, 10000);
    if (ImGui::Button("Spawn")) {
        game::spawnBodies(m_registry, m_world, m_spawnCount, ++m_seed);
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear")) {
        m_registry.clear();
    }

    ImGui::End();
}

void Application::render(bool capture) {
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    drawDebugUi();
    ImGui::Render();

    // Batch every body into one vertex/index buffer and submit a single
    // SDL_RenderGeometry call instead of one draw call per body.
    m_vertices.clear();
    m_indices.clear();
    m_registry.view<const game::Transform, const game::Body, const game::Tint>().each(
        [this](const game::Transform& transform, const game::Body& body, const game::Tint& tint) {
            const SDL_FColor color{static_cast<float>(tint.r) * kByteToUnit,
                                   static_cast<float>(tint.g) * kByteToUnit,
                                   static_cast<float>(tint.b) * kByteToUnit, 1.0f};
            const float x0 = transform.position.x - body.radius;
            const float y0 = transform.position.y - body.radius;
            const float x1 = transform.position.x + body.radius;
            const float y1 = transform.position.y + body.radius;

            const auto base = static_cast<int>(m_vertices.size());
            m_vertices.push_back({{x0, y0}, color, {0.0f, 0.0f}});
            m_vertices.push_back({{x1, y0}, color, {0.0f, 0.0f}});
            m_vertices.push_back({{x1, y1}, color, {0.0f, 0.0f}});
            m_vertices.push_back({{x0, y1}, color, {0.0f, 0.0f}});

            constexpr std::array<int, 6> quad{0, 1, 2, 0, 2, 3};
            for (const int offset : quad) {
                m_indices.push_back(base + offset);
            }
        });

    SDL_SetRenderDrawColor(m_renderer, 16, 18, 24, 255);
    SDL_RenderClear(m_renderer);
    if (!m_indices.empty()) {
        SDL_RenderGeometry(m_renderer, nullptr, m_vertices.data(), static_cast<int>(m_vertices.size()),
                           m_indices.data(), static_cast<int>(m_indices.size()));
    }
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), m_renderer);
    if (capture && !saveScreenshot()) {
        m_exitCode = 1;
    }
    SDL_RenderPresent(m_renderer);
}

} // namespace app
