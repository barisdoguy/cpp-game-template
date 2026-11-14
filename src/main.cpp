#include "app/Application.hpp"

#include <SDL3/SDL_main.h> // maps main() to the platform entry point (e.g. WinMain)
#include <spdlog/spdlog.h>

#include <charconv>
#include <cstdint>
#include <cstdio>
#include <string_view>
#include <system_error>
#include <utility>

namespace {

template <typename T>
bool parseNumber(std::string_view text, T& out) {
    const auto* first = text.data();
    const auto* last = first + text.size();
    const auto [ptr, ec] = std::from_chars(first, last, out);
    return ec == std::errc{} && ptr == last;
}

void printUsage() {
    std::puts("Usage: game [--bodies N] [--frames N [--screenshot FILE.png]]\n"
              "  --bodies N           bodies spawned at start (default 2000)\n"
              "  --frames N           quit after N frames (N >= 1, headless smoke tests)\n"
              "  --screenshot FILE    save the last frame as PNG (requires --frames)");
}

} // namespace

int main(int argc, char* argv[]) {
    app::Config config;

    for (int i = 1; i < argc; ++i) {
        const std::string_view arg{argv[i]};
        const bool hasValue = i + 1 < argc;

        if (arg == "--help" || arg == "-h") {
            printUsage();
            return 0;
        }
        if (arg == "--frames" && hasValue) {
            std::uint64_t frames = 0;
            if (!parseNumber(argv[++i], frames) || frames == 0) {
                spdlog::error("--frames expects a positive integer");
                return 2;
            }
            config.maxFrames = frames;
        } else if (arg == "--bodies" && hasValue) {
            if (!parseNumber(argv[++i], config.initialBodies) || config.initialBodies < 0) {
                spdlog::error("--bodies expects a non-negative integer");
                return 2;
            }
        } else if (arg == "--screenshot" && hasValue) {
            config.screenshotPath = argv[++i];
        } else {
            spdlog::error("Unknown argument: {}", arg);
            printUsage();
            return 2;
        }
    }

    if (!config.screenshotPath.empty() && !config.maxFrames) {
        spdlog::error("--screenshot requires --frames");
        return 2;
    }

    app::Application application{std::move(config)};
    return application.run();
}
