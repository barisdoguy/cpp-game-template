#include "game/Components.hpp"
#include "game/Systems.hpp"

#include <doctest/doctest.h>
#include <entt/entity/registry.hpp>

namespace {

// doctest::Approx compares in double. Widen explicitly so -Wdouble-promotion
// stays quiet and the intent is visible.
double wide(float value) {
    return static_cast<double>(value);
}

doctest::Approx approx(float expected) {
    return doctest::Approx(static_cast<double>(expected));
}

entt::entity makeBody(entt::registry& registry, glm::vec2 position, glm::vec2 velocity, float radius = 4.0f,
                      float restitution = 1.0f) {
    const auto entity = registry.create();
    registry.emplace<game::Transform>(entity, position);
    registry.emplace<game::Velocity>(entity, velocity);
    registry.emplace<game::Body>(entity, radius, restitution);
    return entity;
}

} // namespace

TEST_CASE("integrate applies gravity to velocity, then velocity to position") {
    entt::registry registry;
    game::WorldSettings world;
    world.gravity = {0.0f, 10.0f};
    const auto entity = makeBody(registry, {0.0f, 0.0f}, {2.0f, 0.0f});

    game::integrate(registry, world, 0.5f);

    const auto& velocity = registry.get<game::Velocity>(entity).value;
    const auto& position = registry.get<game::Transform>(entity).position;
    CHECK(wide(velocity.x) == approx(2.0f));
    CHECK(wide(velocity.y) == approx(5.0f));
    CHECK(wide(position.x) == approx(1.0f));
    CHECK(wide(position.y) == approx(2.5f));
}

TEST_CASE("integrate respects the world time scale") {
    entt::registry registry;
    game::WorldSettings world;
    world.gravity = {0.0f, 0.0f};
    world.timeScale = 0.0f;
    const auto entity = makeBody(registry, {10.0f, 10.0f}, {100.0f, 100.0f});

    game::integrate(registry, world, 1.0f);

    const auto& position = registry.get<game::Transform>(entity).position;
    CHECK(wide(position.x) == approx(10.0f));
    CHECK(wide(position.y) == approx(10.0f));
}

TEST_CASE("resolveBounds clamps to the floor and bounces upward with restitution") {
    entt::registry registry;
    game::WorldSettings world;
    world.size = {100.0f, 100.0f};
    const auto entity = makeBody(registry, {50.0f, 120.0f}, {0.0f, 40.0f}, 5.0f, 0.5f);

    game::resolveBounds(registry, world);

    const auto& position = registry.get<game::Transform>(entity).position;
    const auto& velocity = registry.get<game::Velocity>(entity).value;
    CHECK(wide(position.y) == approx(95.0f));
    CHECK(wide(velocity.y) == approx(-20.0f));
}

TEST_CASE("resolveBounds bounces off the left wall") {
    entt::registry registry;
    game::WorldSettings world;
    world.size = {100.0f, 100.0f};
    const auto entity = makeBody(registry, {-3.0f, 50.0f}, {-10.0f, 0.0f}, 2.0f, 1.0f);

    game::resolveBounds(registry, world);

    CHECK(wide(registry.get<game::Transform>(entity).position.x) == approx(2.0f));
    CHECK(wide(registry.get<game::Velocity>(entity).value.x) == approx(10.0f));
}

TEST_CASE("resolveBounds leaves bodies inside the world untouched") {
    entt::registry registry;
    game::WorldSettings world;
    world.size = {100.0f, 100.0f};
    const auto entity = makeBody(registry, {50.0f, 50.0f}, {3.0f, -4.0f});

    game::resolveBounds(registry, world);

    CHECK(wide(registry.get<game::Transform>(entity).position.x) == approx(50.0f));
    CHECK(wide(registry.get<game::Velocity>(entity).value.y) == approx(-4.0f));
}

TEST_CASE("spawnBodies creates bodies inside the world") {
    entt::registry registry;
    game::WorldSettings world;
    world.size = {320.0f, 240.0f};

    game::spawnBodies(registry, world, 256, 42u);

    CHECK(registry.view<game::Transform>().size() == 256u);
    registry.view<const game::Transform, const game::Body>().each(
        [&](const game::Transform& transform, const game::Body& body) {
            CHECK(transform.position.x >= body.radius);
            CHECK(transform.position.x <= world.size.x - body.radius);
            CHECK(transform.position.y >= body.radius);
            CHECK(transform.position.y <= world.size.y - body.radius);
        });
}

TEST_CASE("spawnBodies is reproducible for the same seed") {
    entt::registry a;
    entt::registry b;
    const game::WorldSettings world;

    game::spawnBodies(a, world, 32, 7u);
    game::spawnBodies(b, world, 32, 7u);

    const auto viewA = a.view<const game::Transform>();
    const auto viewB = b.view<const game::Transform>();
    REQUIRE(viewA.size() == viewB.size());
    for (const auto entity : viewA) {
        REQUIRE(b.valid(entity));
        CHECK(viewA.get<const game::Transform>(entity).position ==
              viewB.get<const game::Transform>(entity).position);
    }
}

TEST_CASE("a bouncing body stays inside the world over many fixed steps") {
    entt::registry registry;
    game::WorldSettings world;
    world.size = {200.0f, 100.0f};
    const auto entity = makeBody(registry, {100.0f, 20.0f}, {350.0f, -50.0f}, 4.0f, 0.9f);

    for (int i = 0; i < 10'000; ++i) {
        game::integrate(registry, world, 1.0f / 120.0f);
        game::resolveBounds(registry, world);
    }

    const auto& position = registry.get<game::Transform>(entity).position;
    CHECK(position.x >= 4.0f);
    CHECK(position.x <= 196.0f);
    CHECK(position.y >= 4.0f);
    CHECK(position.y <= 96.0f);
}
