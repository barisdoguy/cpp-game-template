#pragma once

#include <algorithm>

namespace core {

// Fixed-timestep accumulator (see Glenn Fiedler, "Fix Your Timestep!").
//
// The simulation always advances in steps of the same length, whatever the
// render frame rate. Physics then behaves the same at 30 FPS and at 240 FPS,
// and a replay from the same inputs gives the same result.
class FixedTimestep {
public:
    explicit FixedTimestep(double stepSeconds, int maxStepsPerFrame = 8) noexcept
        : m_step(stepSeconds), m_maxSteps(maxStepsPerFrame) {}

    // Feeds elapsed wall-clock time and returns how many simulation steps to
    // run this frame. Negative input (clock jumps) is ignored.
    [[nodiscard]] int advance(double frameSeconds) noexcept {
        m_accumulator += std::max(0.0, frameSeconds);
        int steps = static_cast<int>(m_accumulator / m_step);
        if (steps > m_maxSteps) {
            // "Spiral of death" guard: after a long stall (debugger break,
            // window drag) drop the backlog instead of falling further behind.
            steps = m_maxSteps;
            m_accumulator = 0.0;
        } else {
            m_accumulator -= static_cast<double>(steps) * m_step;
        }
        return steps;
    }

    // Interpolation factor in [0, 1) for blending the last two simulation
    // states when rendering.
    [[nodiscard]] double alpha() const noexcept { return m_accumulator / m_step; }
    [[nodiscard]] double step() const noexcept { return m_step; }

private:
    double m_step;
    int m_maxSteps;
    double m_accumulator = 0.0;
};

} // namespace core
