#pragma once

#include <entt/entity/fwd.hpp>
#include <glm/vec2.hpp>

#include <cstdint>

namespace game {

struct WorldSettings {
    glm::vec2 size{1280.0f, 720.0f};
    glm::vec2 gravity{0.0f, 600.0f}; // pixels / s^2, +Y points down
    float timeScale = 1.0f;
};

// Semi-implicit Euler: velocity first, then position. Stable enough for games
// and cheaper than RK4.
void integrate(entt::registry& registry, const WorldSettings& world, float dt);

// Keeps every body inside the world rectangle and reflects its velocity,
// scaled by the body's restitution.
void resolveBounds(entt::registry& registry, const WorldSettings& world);

// Spawns `count` bodies. The same seed gives the same bodies on a given
// standard library (distribution output is implementation-defined).
void spawnBodies(entt::registry& registry, const WorldSettings& world, int count, std::uint32_t seed);

} // namespace game
