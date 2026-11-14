#include "game/Systems.hpp"

#include "game/Components.hpp"

#include <entt/entity/registry.hpp>

#include <algorithm>
#include <cmath>
#include <random>

namespace game {

void integrate(entt::registry& registry, const WorldSettings& world, float dt) {
    const float scaledDt = dt * world.timeScale;
    registry.view<Transform, Velocity>().each([&](Transform& transform, Velocity& velocity) {
        velocity.value += world.gravity * scaledDt;
        transform.position += velocity.value * scaledDt;
    });
}

void resolveBounds(entt::registry& registry, const WorldSettings& world) {
    registry.view<Transform, Velocity, const Body>().each(
        [&](Transform& transform, Velocity& velocity, const Body& body) {
            for (glm::length_t axis = 0; axis < 2; ++axis) {
                const float lo = body.radius;
                const float hi = std::max(lo, world.size[axis] - body.radius);
                float& pos = transform.position[axis];
                float& vel = velocity.value[axis];
                if (pos < lo) {
                    pos = lo;
                    vel = std::abs(vel) * body.restitution;
                } else if (pos > hi) {
                    pos = hi;
                    vel = -std::abs(vel) * body.restitution;
                }
            }
        });
}

void spawnBodies(entt::registry& registry, const WorldSettings& world, int count, std::uint32_t seed) {
    std::mt19937 rng{seed};
    std::uniform_real_distribution<float> radius{2.0f, 6.0f};
    std::uniform_real_distribution<float> unit{0.0f, 1.0f};
    std::uniform_real_distribution<float> speed{-300.0f, 300.0f};
    std::uniform_int_distribution<int> channel{80, 255};

    const auto byte = [&] { return static_cast<std::uint8_t>(channel(rng)); };

    // Braced initializers evaluate left to right, so the draw order from the
    // RNG is fixed. (Function-call arguments would be unspecified order.)
    for (int i = 0; i < count; ++i) {
        const auto entity = registry.create();
        const float r = radius(rng);
        const glm::vec2 position{
            std::clamp(unit(rng) * world.size.x, r, std::max(r, world.size.x - r)),
            std::clamp(unit(rng) * world.size.y * 0.5f, r, std::max(r, world.size.y - r)),
        };
        const glm::vec2 velocity{speed(rng), speed(rng)};
        const Body body{r, 0.75f + 0.2f * unit(rng)};
        const Tint tint{byte(), byte(), byte()};

        registry.emplace<Transform>(entity, position);
        registry.emplace<Velocity>(entity, velocity);
        registry.emplace<Body>(entity, body);
        registry.emplace<Tint>(entity, tint);
    }
}

} // namespace game
