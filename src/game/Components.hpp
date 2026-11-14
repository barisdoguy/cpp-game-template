#pragma once

#include <glm/vec2.hpp>

#include <cstdint>

// Components are plain data. Behaviour lives in systems (Systems.hpp).
namespace game {

struct Transform {
    glm::vec2 position{0.0f};
};

struct Velocity {
    glm::vec2 value{0.0f};
};

struct Body {
    float radius = 4.0f;
    float restitution = 0.9f; // 1 = perfectly elastic bounce, 0 = no bounce
};

struct Tint {
    std::uint8_t r = 255;
    std::uint8_t g = 255;
    std::uint8_t b = 255;
};

} // namespace game
