#include "core/FixedTimestep.hpp"

#include <doctest/doctest.h>

TEST_CASE("FixedTimestep runs one step per full step of elapsed time") {
    core::FixedTimestep timestep{1.0 / 60.0};
    CHECK(timestep.advance(1.0 / 60.0) == 1);
    CHECK(timestep.alpha() == doctest::Approx(0.0));
}

TEST_CASE("FixedTimestep carries the remainder into the next frame") {
    const double step = 1.0 / 60.0;
    core::FixedTimestep timestep{step};

    CHECK(timestep.advance(step * 0.5) == 0);
    CHECK(timestep.alpha() == doctest::Approx(0.5));

    CHECK(timestep.advance(step * 0.5) == 1);
    CHECK(timestep.alpha() == doctest::Approx(0.0));
}

TEST_CASE("FixedTimestep runs several steps after a slow frame") {
    const double step = 1.0 / 120.0;
    core::FixedTimestep timestep{step};
    CHECK(timestep.advance(step * 3.25) == 3);
    CHECK(timestep.alpha() == doctest::Approx(0.25));
}

TEST_CASE("FixedTimestep caps steps and drops the backlog after a stall") {
    core::FixedTimestep timestep{1.0 / 60.0, 4};
    CHECK(timestep.advance(5.0) == 4);
    CHECK(timestep.alpha() == doctest::Approx(0.0));
    CHECK(timestep.advance(0.0) == 0);
}

TEST_CASE("FixedTimestep ignores negative frame times") {
    core::FixedTimestep timestep{1.0 / 60.0};
    CHECK(timestep.advance(-1.0) == 0);
    CHECK(timestep.alpha() == doctest::Approx(0.0));
}
